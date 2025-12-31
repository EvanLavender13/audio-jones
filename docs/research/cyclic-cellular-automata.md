# Cyclic Cellular Automata (CCA)

Background research for CCA-based visual effects. Cyclic CA produces perpetual spiraling "demon" patterns that never reach steady state—ideal for continuous audio-reactive visualization.

## Core Concept

Each cell holds one of N discrete states (0 to N-1). A cell in state `i` advances to state `(i+1) mod N` when it detects sufficient neighbors already in that next state. This creates a predator-prey dynamic: state 2 "eats" state 1, state 3 eats state 2, and state 0 wraps around to eat state N-1.

The result: perpetual rotating waves called "demons" that spawn outward spirals. Unlike reaction-diffusion or Lenia, CCA never settles to equilibrium—patterns continuously flow and reorganize.

## Comparison with Other CA Systems

| Aspect | Game of Life | Reaction-Diffusion | Lenia | Cyclic CA |
|--------|--------------|-------------------|-------|-----------|
| States | Binary (0/1) | Continuous (0-1) | Continuous (0-1) | Discrete (0 to N-1) |
| Dynamics | Gliders, still lifes | Spots, stripes, mazes | Soliton "creatures" | Perpetual spirals |
| Steady state | Yes (often) | Yes (slowly) | Quasi-stable | Never |
| Food/source | No | No | Optional | No |
| Parameters | 1 rule set | 4 (F, k, Dᵤ, Dᵥ) | 10+ (kernel, growth) | 3-4 (N, threshold, range) |
| GPU cost | Low | Medium | High | Low-Medium |

CCA excels at continuous motion without external energy sources. Parameter changes produce immediate, dramatic visual shifts.

## Algorithm

### State Transition Rule

The fundamental update rule:

```
if count(neighbors in state (i+1) mod N) >= threshold:
    cell_next = (i + 1) mod N
else:
    cell_next = i
```

Where:
- `i`: current cell state (0 to N-1)
- `N`: total number of states
- `threshold`: minimum neighbors required to trigger advance
- `neighbors`: cells within the defined neighborhood

### Neighborhood Definitions

**Moore Neighborhood** (8 cells, radius 1):
```
[1 1 1]
[1 X 1]
[1 1 1]
```

**Von Neumann Neighborhood** (4 cells, radius 1):
```
[0 1 0]
[1 X 1]
[0 1 0]
```

**Extended Moore** (radius R):
All cells within Chebyshev distance R. Radius 2 = 24 neighbors, radius 3 = 48 neighbors.

**Circular/Euclidean** (radius R):
All cells within Euclidean distance R. Produces smoother, more organic spirals.

```python
# Circular neighborhood kernel
for dy in range(-R, R+1):
    for dx in range(-R, R+1):
        if sqrt(dx*dx + dy*dy) <= R:
            kernel[dy + R][dx + R] = 1
```

### Update Logic (Complete)

```python
def update_cca(state, N, threshold, neighborhood):
    next_state = state.copy()
    height, width = state.shape

    for y in range(height):
        for x in range(width):
            current = state[y, x]
            target = (current + 1) % N

            # Count neighbors in target state
            count = 0
            for dy, dx in neighborhood:
                ny = (y + dy) % height  # Periodic boundary
                nx = (x + dx) % width
                if state[ny, nx] == target:
                    count += 1

            if count >= threshold:
                next_state[y, x] = target

    return next_state
```

## Pattern Evolution Phases

CCA evolves through distinct visual phases from random initialization:

### Phase 1: Random Field
Initial noise. No coherent structure. Duration: 10-50 iterations.

### Phase 2: Consumption
Color blocks form and advance against remaining randomness. Larger regions "eat" smaller ones. Duration: 50-200 iterations.

### Phase 3: Demon Formation
Critical structures called "demons" emerge—cycles of adjacent cells in consecutive states that rotate continuously. Each demon acts as a wave source.

### Phase 4: Spiral Regime (Perpetual)
Demons emit outward-propagating spiral waves. Multiple demons create interference patterns. This phase continues indefinitely without reaching steady state.

### Turbulence Mode
At intermediate threshold values, demons fail to fully stabilize. The system enters a chaotic regime with fragmented spirals, merging/splitting wave fronts, and continuous reorganization. This "turbulent" mode produces the most visually dynamic results.

## Parameter Effects

### Number of States (N)

| N | Visual Effect |
|---|---------------|
| 3-4 | Fast rotation, bold color bands, aggressive spirals |
| 6-8 | Balanced speed, distinct color gradation |
| 12-16 | Slow, smooth rotation, fine color bands |
| 20+ | Very slow evolution, subtle gradients |

Lower N = faster visual tempo. Higher N = more colors but slower dynamics.

### Threshold

| Threshold | Effect |
|-----------|--------|
| 1 | Maximum aggression. Spirals form quickly, strong wave propagation |
| 2 | Slightly slower formation, cleaner spiral edges |
| 3+ | Turbulent regime. Spirals fragment, chaotic mixing |
| High (>N/2) | System freezes into static color blocks |

Threshold 1 with Moore neighborhood (8 cells) guarantees spiral formation. Higher thresholds create the visually interesting turbulent regime.

### Neighborhood Range

| Range | Effect |
|-------|--------|
| 1 (8 neighbors) | Sharp, pixelated spirals. Fast computation |
| 2 (24 neighbors) | Smoother curves, wider spiral arms |
| 3-5 (48-120 neighbors) | Organic, flowing patterns. Higher GPU cost |
| 5+ | Very smooth but computationally expensive |

Larger ranges produce more organic, less pixelated visuals but increase texture fetches quadratically.

### Circular vs Square Neighborhoods

Circular (Euclidean) neighborhoods eliminate the axis-aligned artifacts visible with Moore neighborhoods. Spirals appear rounder and more natural. Cost: slightly more complex kernel computation.

## GPU Implementation

### Computational Cost

CCA requires fewer operations per cell than reaction-diffusion or MNCA:

| Grid Size | Range 1 | Range 2 | Range 3 |
|-----------|---------|---------|---------|
| 512×512 | 2M ops | 6M ops | 13M ops |
| 1024×1024 | 8M ops | 25M ops | 50M ops |
| 1920×1080 | 16M ops | 50M ops | 100M ops |

Operations = grid_cells × neighbors_per_cell. Fragment shader handles range 1-2 easily at 60fps. Range 3+ benefits from compute shaders.

### Data Layout

Single-channel texture stores state index:

| Format | Precision | States | Notes |
|--------|-----------|--------|-------|
| R8 | 8-bit | 0-255 | Sufficient for most cases |
| R16F | 16-bit float | 0-65535 | Overkill but compatible |
| R32F | 32-bit float | Unlimited | Maximum precision |

R8 format minimizes bandwidth. 256 possible states far exceeds typical N values (3-16).

### Fragment Shader Implementation

```glsl
#version 330 core

uniform sampler2D uState;
uniform vec2 uResolution;
uniform int uNumStates;
uniform int uThreshold;
uniform int uRange;

out vec4 fragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 texel = 1.0 / uResolution;

    // Current state (0 to N-1)
    int current = int(texture(uState, uv).r * 255.0);
    int target = (current + 1) % uNumStates;

    // Count neighbors in target state
    int count = 0;

    // Moore neighborhood (range 1)
    for (int dy = -uRange; dy <= uRange; dy++) {
        for (int dx = -uRange; dx <= uRange; dx++) {
            if (dx == 0 && dy == 0) continue;

            vec2 neighbor = uv + vec2(dx, dy) * texel;
            neighbor = fract(neighbor); // Periodic boundary

            int neighborState = int(texture(uState, neighbor).r * 255.0);
            if (neighborState == target) {
                count++;
            }
        }
    }

    // Apply transition rule
    int nextState = current;
    if (count >= uThreshold) {
        nextState = target;
    }

    fragColor = vec4(float(nextState) / 255.0, 0.0, 0.0, 1.0);
}
```

### Compute Shader Implementation

For larger neighborhoods (range 3+), compute shaders with shared memory provide ~5x speedup:

```glsl
#version 430
layout(local_size_x = 16, local_size_y = 16) in;

layout(r8, binding = 0) uniform readonly image2D stateIn;
layout(r8, binding = 1) uniform writeonly image2D stateOut;

uniform int uNumStates;
uniform int uThreshold;
uniform int uRange;
uniform ivec2 uSize;

// Shared memory tile with halo
shared int tile[48][48]; // 16 + 2*16 for range up to 16

void main() {
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    ivec2 lid = ivec2(gl_LocalInvocationID.xy);
    ivec2 wid = ivec2(gl_WorkGroupID.xy);

    // Cooperative tile loading (simplified - full version handles edges)
    int halo = uRange;
    ivec2 tileBase = wid * ivec2(16, 16) - ivec2(halo);

    // Each thread loads multiple cells into shared memory
    for (int ty = lid.y; ty < 16 + 2 * halo; ty += 16) {
        for (int tx = lid.x; tx < 16 + 2 * halo; tx += 16) {
            ivec2 loadPos = tileBase + ivec2(tx, ty);
            loadPos = (loadPos + uSize) % uSize; // Wrap
            tile[ty][tx] = int(imageLoad(stateIn, loadPos).r * 255.0);
        }
    }

    barrier();

    // Read current state from shared memory
    int current = tile[lid.y + halo][lid.x + halo];
    int target = (current + 1) % uNumStates;

    // Count neighbors from shared memory
    int count = 0;
    for (int dy = -uRange; dy <= uRange; dy++) {
        for (int dx = -uRange; dx <= uRange; dx++) {
            if (dx == 0 && dy == 0) continue;
            if (tile[lid.y + halo + dy][lid.x + halo + dx] == target) {
                count++;
            }
        }
    }

    int nextState = (count >= uThreshold) ? target : current;
    imageStore(stateOut, gid, vec4(float(nextState) / 255.0));
}
```

### Circular Neighborhood Optimization

Pre-compute circular kernel as uniform array or texture:

```glsl
// Pre-computed offsets for circular neighborhood radius 3
// Generated once at init, passed as uniform
uniform ivec2 uNeighborOffsets[28]; // Cells within radius 3
uniform int uNeighborCount;

// In shader:
for (int i = 0; i < uNeighborCount; i++) {
    vec2 neighbor = uv + vec2(uNeighborOffsets[i]) * texel;
    // ... sample and count
}
```

This eliminates the inner loop distance check, reducing ALU operations.

## Color Mapping

### Direct State-to-Hue

Map state index directly to hue:

```glsl
vec3 stateToColor(int state, int numStates) {
    float hue = float(state) / float(numStates);
    return hsv2rgb(vec3(hue, 1.0, 1.0));
}
```

Produces rainbow gradient cycling through all states. Simple and effective.

### Gradient LUT

Sample from a 1D gradient texture for custom color schemes:

```glsl
uniform sampler1D uGradient;

vec3 stateToColor(int state, int numStates) {
    float t = float(state) / float(numStates);
    return texture(uGradient, t).rgb;
}
```

Allows preset-defined color palettes.

### Edge Enhancement

Highlight state boundaries with gradient detection:

```glsl
// Sobel-like edge detection on state field
float edge = 0.0;
for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
        int neighborState = sampleState(uv + vec2(dx, dy) * texel);
        if (neighborState != currentState) {
            edge += 1.0;
        }
    }
}
edge /= 8.0;

// Blend edge glow with base color
vec3 color = mix(baseColor, vec3(1.0), edge * edgeIntensity);
```

## Integration with AudioJones

### Pipeline Position

```
CCA Simulation (fragment or compute shader)
         ↓
State Texture (ping-pong, R8 format)
         ↓
Color Mapping Shader (state → RGB via gradient)
         ↓
Blend with accumulation buffer
         ↓
Post-effects (kaleidoscope, feedback, bloom)
```

CCA output feeds into the existing post-effect chain. The colored output blends with waveforms, shapes, and physarum trails.

### Drawable Integration

Extend the drawable system with a CCA type:

```cpp
// In drawable_config.h
enum DrawableType {
    DRAWABLE_WAVEFORM,
    DRAWABLE_SPECTRUM,
    DRAWABLE_SHAPE,
    DRAWABLE_CCA  // New type
};

struct CCAConfig {
    int numStates = 8;
    int threshold = 1;
    int range = 1;
    bool circularNeighborhood = false;
    float simulationSpeed = 1.0f;  // Steps per frame
    ColorConfig colorConfig;
};
```

### Audio Reactivity Options

| Parameter | Audio Source | Effect |
|-----------|--------------|--------|
| numStates | Slowly varying (envelope) | More states = finer color bands, slower rotation |
| threshold | Bass energy | Low bass = aggressive spirals, high bass = turbulence |
| range | Mid energy | Larger range = smoother, more organic flow |
| simulationSpeed | Beat/tempo | Sync rotation speed to music tempo |
| colorConfig.hueShift | Continuous LFO | Rotating color palette |
| seedInjection | Beat detection | Inject new demons on beat hits |

### Beat-Synchronized Seeding

Inject new random seeds on beat detection to create fresh spiral sources:

```cpp
void CCAInjectSeed(CCAState* state, int x, int y, int radius) {
    // Set random states in circular region
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= radius*radius) {
                int px = (x + dx + state->width) % state->width;
                int py = (y + dy + state->height) % state->height;
                state->cells[py * state->width + px] = rand() % state->numStates;
            }
        }
    }
}

// In update loop:
if (beatDetected) {
    int x = rand() % width;
    int y = rand() % height;
    CCAInjectSeed(&ccaState, x, y, 20);
}
```

This creates localized chaos that spawns new demons, synchronized to musical events.

### ModEngine Integration

Register CCA parameters with the modulation system:

```cpp
void CCARegisterModulation(CCAConfig* config, ModEngine* engine) {
    ModEngineRegisterParam(PARAM_CCA_THRESHOLD, &config->threshold, 1, 5);
    ModEngineRegisterParam(PARAM_CCA_RANGE, &config->range, 1, 5);
    ModEngineRegisterParam(PARAM_CCA_SPEED, &config->simulationSpeed, 0.1f, 4.0f);
    ModEngineRegisterParam(PARAM_CCA_HUE_SHIFT, &config->colorConfig.hueShift, 0.0f, 1.0f);
}
```

### Resource Requirements

| Component | Size | Notes |
|-----------|------|-------|
| State texture (×2) | 2 MB (1920×1080 R8) | Ping-pong pair |
| Color gradient | 1 KB (256×1 RGB8) | 1D LUT |
| Shader | ~50-100 lines GLSL | Fragment or compute |
| Neighbor kernel | 512 bytes | Pre-computed offsets |

Total VRAM: ~4 MB. Minimal compared to physarum (~12 MB for agent buffers).

### Performance Estimate

| Configuration | Est. Frame Time | Notes |
|---------------|-----------------|-------|
| 1080p, range 1, fragment | 0.3 ms | Trivial |
| 1080p, range 2, fragment | 1.2 ms | Still fast |
| 1080p, range 3, compute | 2.0 ms | Shared memory helps |
| 1080p, range 5, compute | 4.0 ms | Budget ~6ms for 60fps headroom |

CCA is significantly cheaper than physarum (agent simulation + trail diffusion) and reaction-diffusion (9-point stencil × 2 species × substeps).

## Variations and Extensions

### Greenberg-Hastings Model

Modified CCA where cells have an "excited" and "refractory" period:

```
State 0: Resting (can be excited)
State 1: Excited (will become refractory)
States 2-N: Refractory (counting down to resting)
```

Only state 0 cells can become excited. Produces cleaner, more biological-looking waves.

### Probabilistic CCA

Add randomness to transition rule:

```glsl
if (count >= threshold && random() < probability) {
    nextState = target;
}
```

Creates softer wave edges and occasional "mutations" that disrupt patterns.

### Multi-Species CCA

Two or more independent CCA systems with different parameters, blended via color channels:

```glsl
// Red channel: fast 4-state CCA
// Green channel: slow 12-state CCA
// Blue channel: mid 8-state CCA with high threshold
```

Creates complex color mixing as independent wave systems interfere.

### Flow-Field Advection

Move the state field with a flow vector before applying CCA rules:

```glsl
vec2 flow = texture(uFlowField, uv).xy;
vec2 advectedUV = uv - flow * deltaTime;
int current = sampleState(advectedUV);
// Then apply normal CCA rule
```

Flow field could derive from audio (bass = radial push) or LFO (rotating field). Adds directionality to spiral motion.

## Open Questions

- **TBD:** Optimal state count for visual appeal vs parameter responsiveness
- **TBD:** Threshold modulation range—how far can it push into turbulence before visual coherence breaks?
- **Needs investigation:** Interaction with kaleidoscope effect (radial symmetry may create interesting demon arrangements)
- **TBD:** Seed injection strategy—random placement vs audio-reactive positioning (e.g., bass seeds at center, treble at edges)
- **Needs investigation:** Blending strategy with physarum trails (additive? multiplicative? separate layers?)

## Sources

### Primary
- [Wikipedia - Cyclic Cellular Automaton](https://en.wikipedia.org/wiki/Cyclic_cellular_automaton)
- [Fisch, Gravner, Griffeath - Cyclic Cellular Automata in Two Dimensions (1991)](https://link.springer.com/chapter/10.1007/978-1-4612-0451-0_8)

### Implementations
- [Ne0nWinds - Interactive Cyclic Cellular Automata (D3D11 Compute)](https://github.com/Ne0nWinds/Interactive-Cyclic-Cellular-Automata/)
- [Chaitanya Shah - Cyclic Cellular Automata](https://chaitanyashah.com/cyclic-cellular-automata)
- [Arne Vogel - Cyclic Cellular Automaton](https://www.arnevogel.com/cyclic-cellular-automaton/)

### Tutorials
- [Alan Zucconi - Cellular Automata with Shaders](https://www.alanzucconi.com/2016/03/16/cellular-automata-with-shaders/)
- [Vectrx - Parallelizing Cellular Automata with WebGPU](https://vectrx.substack.com/p/webgpu-cellular-automata)

### Audio-Reactive Examples
- [Algoritmarte - CyclicCA (VCV Rack module)](https://www.algoritmarte.com/cyclic-cellular-automata/)
- [VCV Community - CyclicCA Announcement](https://community.vcvrack.com/t/algoritmarte-cyclicca-cyclic-cellular-automata-simulator/12300)

### Related Systems
- [Greenberg-Hastings Model](https://en.wikipedia.org/wiki/Greenberg%E2%80%93Hastings_cellular_automaton)
- [Belousov-Zhabotinsky Reaction](https://en.wikipedia.org/wiki/Belousov%E2%80%93Zhabotinsky_reaction) (chemical analog)
