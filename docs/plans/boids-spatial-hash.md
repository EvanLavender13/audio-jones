# Boids Spatial Hashing

Replace O(N²) brute-force neighbor search with O(N×K) spatial hash lookup. At 50k boids, reduces distance checks from 2.5B to ~11M per frame. Also fuses three separate neighbor loops into one.

## Current State

- `src/simulation/boids.cpp:152-191` - BoidsUpdate dispatches single compute shader
- `src/simulation/boids.h:40-67` - Boids struct with single agentBuffer SSBO
- `shaders/boids_agents.glsl:59-145` - Three functions (cohesion, separation, alignment) each loop all N boids independently

**Problem**: Each boid executes 3 × N iterations. GPU threads diverge waiting for full loops.

## Technical Implementation

### Spatial Hash Algorithm

**Grid setup**:
```
cellSize = perceptionRadius
gridWidth = ceil(resolution.x / cellSize)
gridHeight = ceil(resolution.y / cellSize)
totalCells = gridWidth * gridHeight
```

**Hash function** (position → cell index):
```glsl
ivec2 cellCoord = ivec2(floor(position / cellSize));
cellCoord = cellCoord % ivec2(gridWidth, gridHeight);  // Toroidal wrap
int cellIndex = cellCoord.y * gridWidth + cellCoord.x;
```

**Build phase** (two sub-passes):

1. **Count**: Each agent atomically increments its cell's count
```glsl
int cell = positionToCell(agents[id].position);
atomicAdd(cellCounts[cell], 1);
```

2. **Prefix sum + scatter**: Compute offsets, then each agent writes its index
```glsl
// After prefix sum computes cellOffsets[cell] = sum of counts before cell
int cell = positionToCell(agents[id].position);
int slot = atomicAdd(cellOffsets[cell], 1);  // Returns old value, then increments
sortedIndices[slot] = id;
```

**Query phase** (in boids steering shader):
```glsl
ivec2 myCell = ivec2(floor(position / cellSize));

for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
        ivec2 neighborCell = (myCell + ivec2(dx, dy) + ivec2(gridWidth, gridHeight)) % ivec2(gridWidth, gridHeight);
        int cellIdx = neighborCell.y * gridWidth + neighborCell.x;

        int start = cellIdx == 0 ? 0 : cellOffsets[cellIdx - 1];
        int end = cellOffsets[cellIdx];

        for (int i = start; i < end; i++) {
            int otherId = sortedIndices[i];
            // ... process neighbor
        }
    }
}
```

### Fused Steering Loop

Current code runs three separate N-loops. Replace with single neighbor loop:

```glsl
vec2 cohesionSum = vec2(0.0);
vec2 separationSum = vec2(0.0);
vec2 alignmentSum = vec2(0.0);
float cohesionWeight = 0.0;
float alignmentWeight = 0.0;

// Single loop over 3x3 cells
for each neighbor in spatial hash query {
    if (neighborId == selfId) continue;

    vec2 delta = wrapDelta(selfPos, neighborPos);
    float dist = length(delta);

    // Cohesion (within perception radius)
    if (dist < perceptionRadius) {
        float affinity = 1.0 - hueDistance(neighborHue, selfHue) * hueAffinity;
        cohesionSum += delta * affinity;
        cohesionWeight += affinity;

        // Alignment (same radius)
        alignmentSum += neighborVel * affinity;
        alignmentWeight += affinity;
    }

    // Separation (within separation radius)
    if (dist < separationRadius && dist > 0.0) {
        float repulsion = 1.0 + hueDistance(neighborHue, selfHue) * hueAffinity * 2.0;
        separationSum -= delta / (dist * dist) * repulsion;
    }
}

// Apply weights after loop
vec2 cohesion = cohesionWeight > 0.001 ? (cohesionSum / cohesionWeight) * 0.01 : vec2(0.0);
vec2 alignment = alignmentWeight > 0.001 ? (alignmentSum / alignmentWeight - selfVel) * 0.125 : vec2(0.0);
vec2 separation = separationSum;
```

### Buffers

| Buffer | Type | Size | Purpose |
|--------|------|------|---------|
| cellCounts | uint | totalCells | Boids per cell (reset each frame) |
| cellOffsets | uint | totalCells | Prefix sum result (also used as insertion counters in scatter) |
| sortedIndices | uint | maxAgents | Agent indices sorted by cell |

### Prefix Sum Strategy

Use simple serial scan on GPU. For typical grid (1920×1080 at 50px cells = 38×21 = 798 cells), serial scan is faster than parallel scan overhead. Single thread iterates cells:

```glsl
// Run with 1 thread
uint sum = 0;
for (int i = 0; i < totalCells; i++) {
    sum += cellCounts[i];
    cellOffsets[i] = sum;
}
```

For grids >10k cells, could upgrade to parallel Hillis-Steele scan.

---

## Phase 1: Spatial Hash Module

**Goal**: Create reusable spatial hash infrastructure.

**Build**:
- `src/simulation/spatial_hash.h` - SpatialHash struct, init/uninit/build/resize API
- `src/simulation/spatial_hash.cpp` - Buffer allocation, compute dispatch, grid dimension calculation
- `shaders/spatial_hash_build.glsl` - Three kernels in one file: clear counts, count agents, prefix sum + scatter

**API**:
```c
typedef struct SpatialHash SpatialHash;

SpatialHash* SpatialHashInit(int maxAgents, float cellSize, int width, int height);
void SpatialHashUninit(SpatialHash* sh);
void SpatialHashResize(SpatialHash* sh, int width, int height);
void SpatialHashBuild(SpatialHash* sh, unsigned int positionBuffer, int agentCount, int agentStride, int positionOffset);
void SpatialHashGetGrid(const SpatialHash* sh, int* outWidth, int* outHeight, float* outCellSize);
unsigned int SpatialHashGetOffsetsBuffer(const SpatialHash* sh);
unsigned int SpatialHashGetIndicesBuffer(const SpatialHash* sh);
```

**Shader uniforms**:
- `resolution` (vec2) - world bounds
- `cellSize` (float) - grid cell dimension
- `gridSize` (ivec2) - grid dimensions in cells
- `agentCount` (int) - number of agents to process
- `agentStride` (int) - bytes between agents in position buffer
- `positionOffset` (int) - byte offset to position within agent struct

**Done when**: Module compiles, buffers allocate correctly.

---

## Phase 2: Integrate with Boids

**Goal**: Boids uses spatial hash for neighbor queries with fused steering.

**Build**:
- Modify `src/simulation/boids.h` - Add SpatialHash* member to Boids struct
- Modify `src/simulation/boids.cpp`:
  - `BoidsInit`: Call `SpatialHashInit` with perceptionRadius as cell size
  - `BoidsUpdate`: Call `SpatialHashBuild` before agent dispatch, bind hash buffers
  - `BoidsApplyConfig`: Recreate spatial hash if perceptionRadius changes significantly
  - `BoidsUninit`: Call `SpatialHashUninit`
  - `BoidsResize`: Call `SpatialHashResize`
- Rewrite `shaders/boids_agents.glsl`:
  - Remove `cohesion()`, `separation()`, `alignment()` functions
  - Add spatial hash buffer bindings and grid uniforms
  - Implement 3×3 cell iteration with fused steering accumulation

**Uniform additions to boids shader**:
- `gridSize` (ivec2)
- `cellSize` (float)

**Buffer bindings**:
- binding 0: agentBuffer (existing)
- binding 1: trailMap image (existing)
- binding 2: cellOffsets (new)
- binding 3: sortedIndices (new)

**Done when**: Boids simulation runs with spatial hashing, visually identical behavior to brute force.

---

## Phase 3: Validation and Cleanup

**Goal**: Verify correctness and performance gains.

**Build**:
- Add temporary debug logging for cell distribution (optional, remove after validation)
- Test at various agent counts: 10k, 25k, 50k, 100k
- Profile GPU frame time before/after
- Verify toroidal wrap works correctly at screen edges
- Remove old brute-force code paths

**Done when**: 50k boids runs at 60fps, no visual artifacts at boundaries.

---

## Post-Implementation Notes

### Grid Artifact Resolution

Initial implementation showed visible gridlines at cell boundaries. Root causes and fixes:

1. **Fine grid at low perception**: Small `perceptionRadius` created thousands of tiny cells. Fixed by calculating cell size from `GCD(width, height)` divisors in range [40, 80], ensuring grid tiles resolution perfectly.

2. **Edge compression**: `clamp()` in `positionToCell()` compressed edge positions. Removed clamp since `mod(pos, resolution)` already constrains positions.

3. **Insufficient scan radius**: Fixed 3×3 neighbor scan missed neighbors when `perceptionRadius > cellSize`. Changed to dynamic `max(2, ceil(perceptionRadius / cellSize))`, ensuring minimum 5×5 scan.

4. **Resolution mismatch on resize**: `BoidsResize` kept old cell size. Now recreates spatial hash with new resolution-based cell size.

### Performance Tuning

- Max agents reduced to 125k (performance degrades >150k at max perception)
- Profiler EMA smoothing reduced (0.15 → 0.05) for stable UI display
- 5×5 minimum scan adds negligible overhead vs 3×3 (25 vs 9 cell lookups, but neighbor counts similar)
