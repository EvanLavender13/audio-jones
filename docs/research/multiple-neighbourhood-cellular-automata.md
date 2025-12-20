# Multiple Neighbourhood Cellular Automata (MNCA)

Background research for MNCA-based visual effects. Technique developed by Slackermanz, featured in Sebastian Lague's "Complex Behaviour from Simple Rules" video.

## Core Concept

Traditional cellular automata (Conway's Game of Life) use a single neighborhood to determine cell state. MNCA extend this by evaluating 2-4 distinct neighborhoods simultaneously, each with independent threshold rules. Later rules overwrite earlier ones when triggered.

This produces soliton-like structures: resilient, individualized units that interact without mutual destruction. Emergent patterns include amoebas with cell walls, organisms undergoing mitosis, worms, and multicellular structures.

The term "Multiple Neighbourhood Cellular Automata" was coined by Softology's Blog. Slackermanz did not name the technique when developing it.

## Comparison with Traditional CA

| Aspect | Game of Life | Larger Than Life | MNCA |
|--------|--------------|------------------|------|
| Neighborhoods | 1 (8 cells) | 1 (large radius) | 2-4 (concentric rings) |
| Neighborhood size | 8 cells | Up to 500 cells | Thousands of cells |
| State | Binary | Binary | Binary or continuous |
| Rules | Single birth/death | Single threshold | Multiple ordered rules |
| Patterns | Gliders, still lifes | "Bugs" | Robust solitons, mitosis |

MNCA structures exhibit greater resilience than other CA families. Patterns recover from perturbations that destroy equivalent structures in simpler models.

## Algorithm

### Neighborhood Definition

Neighborhoods form concentric rings around each cell. A typical configuration:

```
Neighborhood 0: Inner disc (radius 0-5)
Neighborhood 1: First ring (radius 6-12)
Neighborhood 2: Second ring (radius 13-20)
Neighborhood 3: Outer ring (radius 21-30)
```

Each neighborhood tallies live cells within its region, producing sums `sum_0`, `sum_1`, `sum_2`, `sum_3`.

### Neighborhood Kernel Shapes

Kernels are 2D arrays marking which cells belong to each neighborhood:

```python
# Example: ring neighborhood between radius 6-12
for dy in range(-maxRadius, maxRadius+1):
    for dx in range(-maxRadius, maxRadius+1):
        r = sqrt(dx*dx + dy*dy)
        if 6 <= r <= 12:
            kernel[dy + center][dx + center] = 1
```

Kernel shapes include:
- **Filled circles**: All cells within radius R
- **Rings/annuli**: Cells between inner radius r and outer radius R
- **Toroidal shapes**: 3D extension with concentric shells

### Rule Structure

Each neighborhood has threshold rules defining birth and death conditions:

```
nhThresh = [
    [[0, 17, False], [40, 42, True]],   # sum_0: die if 0-17, live if 40-42
    [[10, 13, True]],                     # sum_1: live if 10-13
    [[9, 21, False]],                     # sum_2: die if 9-21
    [[78, 89, False], [109, 500, False]]  # sum_3: die if 78-89 or 109+
]
```

Rule tuple format: `[min, max, isBirth]`
- `isBirth = True`: Dead cells in range become alive
- `isBirth = False`: Alive cells in range die

### Update Logic

Rules apply sequentially. Later rules overwrite earlier results:

```python
for n in range(len(neighborhoods)):
    for rule in rules[n]:
        min_val, max_val, is_birth = rule

        if is_birth:
            # Birth: dead cells become alive if sum in range
            mask = (sums[n] >= min_val) & (sums[n] <= max_val) & (state == 0)
            state = state | mask
        else:
            # Death: alive cells die if sum in range
            mask = (sums[n] >= min_val) & (sums[n] <= max_val)
            state = state & ~mask
```

Order matters: a cell may be born by one rule and killed by a subsequent rule in the same timestep.

### Convolution-Based Implementation

Efficient implementations compute neighborhood sums via convolution:

```python
from scipy.ndimage import correlate

# neighborhood_kernels: list of 2D binary arrays
sums = [correlate(state, kernel, mode="wrap") for kernel in neighborhood_kernels]
```

Wraparound boundaries (torus topology) prevent edge artifacts.

## Discrete vs Continuous MNCA

### Discrete MNCA

State is binary (0 or 1). Rules set state directly:

- Birth rule: `if sum in range and cell dead: cell = 1`
- Death rule: `if sum in range and cell alive: cell = 0`

Last triggered rule determines final state.

### Continuous MNCA

State ranges from 0.0 to 1.0. Rules increment or decrement:

```glsl
float delta = 0.0;

for each rule {
    if (sum >= rule.min && sum <= rule.max) {
        delta += rule.isGrowth ? +0.1 : -0.1;
    }
}

state = clamp(state + delta, 0.0, 1.0);
```

Continuous MNCA produce smoother, more organic patterns resembling SmoothLife and Lenia.

## Relationship to Lenia

MNCA and Lenia share conceptual overlap:

| Aspect | MNCA | Lenia |
|--------|------|-------|
| Developer | Slackermanz | Bert Chan |
| State | Discrete or continuous | Continuous (0-1) |
| Neighborhoods | Multiple discrete rings | Single smooth kernel |
| Growth function | Threshold ranges | Gaussian curve |
| Time | Discrete steps | Continuous (dt integration) |

Both produce soliton-like "lifeforms" but derive from independent research. MNCA use hard threshold boundaries; Lenia uses smooth Gaussian growth functions.

### Lenia Kernel and Growth

For reference, Lenia's update formula:

```
A(t+dt) = A(t) + dt * G(K * A(t))
```

Where:
- `K`: Donut-shaped convolution kernel
- `G`: Growth function (Gaussian centered at μ with width σ)
- `dt`: Time step (typically 0.1-0.2)

The growth function returns values in [-1, 1]:
- `G > 0`: Cell grows
- `G < 0`: Cell decays
- `G ≈ 0`: Cell stable

## GPU Implementation

### Computational Cost

MNCA require summing thousands of cells per pixel per frame. CPU implementations cannot achieve real-time performance at reasonable resolutions.

| Grid Size | Cells/Neighborhood | Total Ops/Frame |
|-----------|-------------------|-----------------|
| 256×256 | 500 | 32M |
| 512×512 | 2000 | 524M |
| 1024×1024 | 2000 | 2B |

GPU acceleration is effectively mandatory for real-time visualization.

### Shader Architecture

Two-pass texture ping-pong with compute shader:

```
Pass 1: Compute shader calculates neighborhood sums and applies rules
Pass 2: Swap read/write textures
```

### GLSL Fragment Shader Approach

For smaller neighborhoods, fragment shaders suffice:

```glsl
uniform sampler2D state;
uniform vec2 resolution;

// Neighborhood parameters
uniform float innerRadius0, outerRadius0;
uniform float innerRadius1, outerRadius1;
uniform float threshold0Min, threshold0Max;
uniform float threshold1Min, threshold1Max;

void main() {
    vec2 uv = gl_FragCoord.xy;

    float sum0 = 0.0;
    float sum1 = 0.0;

    // Sample neighborhood (example: radius up to 15)
    for (float dy = -15.0; dy <= 15.0; dy++) {
        for (float dx = -15.0; dx <= 15.0; dx++) {
            float r = length(vec2(dx, dy));
            vec2 sampleUV = mod(uv + vec2(dx, dy), resolution);
            float val = texture(state, sampleUV / resolution).r;

            if (r >= innerRadius0 && r <= outerRadius0) sum0 += val;
            if (r >= innerRadius1 && r <= outerRadius1) sum1 += val;
        }
    }

    // Apply rules (example: 2 neighborhoods)
    float current = texture(state, uv / resolution).r;
    float next = current;

    // Neighborhood 0 rules
    if (sum0 >= threshold0Min && sum0 <= threshold0Max) {
        next = 1.0;  // Birth
    }

    // Neighborhood 1 rules (can overwrite)
    if (sum1 < threshold1Min || sum1 > threshold1Max) {
        next = 0.0;  // Death
    }

    fragColor = vec4(next, 0.0, 0.0, 1.0);
}
```

### Compute Shader Optimization

For large neighborhoods (radius > 20), compute shaders with shared memory provide significant speedup:

1. Workgroup loads tile + halo into shared memory
2. Each thread samples neighborhood from shared memory
3. Reduces global memory bandwidth by ~10x

```glsl
#version 430
layout(local_size_x = 16, local_size_y = 16) in;

shared float tile[48][48];  // 16x16 tile + 16 pixel halo

layout(rgba32f, binding = 0) uniform image2D stateIn;
layout(rgba32f, binding = 1) uniform image2D stateOut;

void main() {
    // Load tile + halo into shared memory
    // ... (cooperative loading)

    barrier();

    // Compute sums from shared memory
    float sum0 = 0.0, sum1 = 0.0;
    // ... (neighborhood sampling)

    // Apply rules
    // ... (threshold logic)

    imageStore(stateOut, ivec2(gl_GlobalInvocationID.xy), vec4(result));
}
```

## Example Rule Sets

### Slackermanz "Worms"

```
Neighborhood 0: radius 5-10
Neighborhood 1: radius 12-20
Neighborhood 2: radius 22-30

Rules:
  sum_0 in [15, 25]: birth
  sum_1 in [40, 60]: survive
  sum_2 in [80, 120]: death
```

Produces worm-like structures that navigate the grid.

### Mitosis Pattern

```
Neighborhood 0: radius 0-8
Neighborhood 1: radius 10-18

Rules:
  sum_0 in [20, 35]: birth
  sum_0 in [0, 10]: death
  sum_1 in [50, 80]: survive
  sum_1 in [100, 200]: death (overpopulation split)
```

Cells grow, split into two smaller cells when reaching threshold size.

## Integration with AudioJones

### Visual Characteristics

MNCA produce:
- Organic, biological patterns
- Self-organizing structures
- Continuous evolution (never reaches stable state with most rules)
- Scale-dependent behavior (pattern character changes with grid resolution)

### Pipeline Position

```
MNCA Simulation (compute shader)
         ↓
State Texture (ping-pong)
         ↓
Color Mapping (gradient LUT)
         ↓
Blend with existing effects (voronoi, waveforms)
```

### Audio Reactivity Options

| Parameter | Audio Source | Effect |
|-----------|--------------|--------|
| Birth threshold range | Bass energy | Expansion on kicks |
| Death threshold range | High-frequency energy | Treble causes contraction |
| Neighborhood radii | Mid-frequency energy | Pattern scale modulation |
| Rule selection | Beat detection | Switch rule sets on drops |
| Time scale | Tempo (BPM) | Simulation speed follows music |

### Resource Requirements

| Component | Size | Notes |
|-----------|------|-------|
| State texture | 4 MB (1024×1024 RGBA32F) | Ping-pong pair = 8 MB |
| Neighborhood kernels | Pre-computed or generated | Stored as uniforms or textures |
| Compute shader | ~100-200 lines GLSL | Depends on neighborhood count |

### Performance Considerations

Fragment shader approach:
- Radius 15: ~900 texture fetches/pixel → 1920×1080 @ 60fps achievable
- Radius 30: ~3600 fetches/pixel → May require half resolution

Compute shader with shared memory:
- Radius 30+: Achievable at full resolution
- Requires OpenGL 4.3

## Open Questions

- **TBD:** Optimal neighborhood radii for audio visualization vs pure simulation
- **TBD:** Rule switching strategy (crossfade states? hard switch?)
- **Needs investigation:** Color mapping strategies beyond simple gradient (consider multiple state channels)
- **TBD:** Interaction with kaleidoscope effect (circular symmetry may reinforce MNCA ring structures)

## Sources

### Primary
- [Slackermanz - Understanding Multiple Neighborhood Cellular Automata](https://slackermanz.com/understanding-multiple-neighborhood-cellular-automata/)
- [Slackermanz VulkanAutomata GitHub](https://github.com/Slackermanz/VulkanAutomata)

### Community Documentation
- [Softology's Blog - Multiple Neighborhoods Cellular Automata](https://softologyblog.wordpress.com/2018/03/09/multiple-neighborhoods-cellular-automata/)
- [Softology's Blog - More Explorations](https://softologyblog.wordpress.com/2018/03/31/more-explorations-with-multiple-neighborhood-cellular-automata/)
- [Softology's Blog - Even More Explorations](https://softologyblog.wordpress.com/2021/02/14/even-more-explorations-with-multiple-neighborhoods-cellular-automata/)

### Implementations
- [Sebastian Lague - MN-Cellular-Automata (Unity/C#)](https://github.com/SebLague/MN-Cellular-Automata)
- [saraqael-m/MNCA (Python)](https://github.com/saraqael-m/MNCA)
- [Josh Gracie - Multi Neighborhood Cellular Automata](https://www.joshgracie.com/blog/mnca/)

### Related Systems
- [Lenia - Wikipedia](https://en.wikipedia.org/wiki/Lenia)
- [HEGL - Continuous Cellular Automata](https://hegl.mathi.uni-heidelberg.de/continuous-cellular-automata/)

### Video
- [Sebastian Lague - Complex Behaviour from Simple Rules (YouTube, timestamp 2:50)](https://youtu.be/kzwT3wQWAHE?t=170)
