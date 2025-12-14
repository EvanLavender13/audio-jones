# Physarum Slime Mold Simulation

Background research for physarum-based visual effect as advanced background enhancer.

## Core Concept

Physarum polycephalum (slime mold) forms network structures through emergent agent behavior. Millions of particles sense chemical trails, move toward concentrations, and deposit more chemicals—creating organic, flowing patterns that self-organize into transport networks.

The technique combines two models:
- **Agent-based model**: Particles with position, heading, and sensors
- **Continuum model**: Diffusion/decay on trail map texture

Original research by Jeff Jones (University of the West of England, 2010) demonstrated complex pattern formation from simple local rules.

## Algorithm Overview

### Agent State

Each agent maintains:
- Position (x, y) in texture space
- Heading angle (radians)
- Three sensors: front-left, front-center, front-right

### Per-Frame Update Cycle

```
1. SENSE: Sample trail map at three sensor positions
2. ROTATE: Turn toward highest concentration
3. MOVE: Step forward in heading direction
4. DEPOSIT: Add chemical to trail map at new position
5. DIFFUSE: Blur trail map (3x3 mean filter)
6. DECAY: Multiply trail map by decay factor (0.9-0.99)
```

### Agent Decision Logic

```glsl
float left   = sample(trailMap, pos + sensorOffset(heading - sensorAngle));
float center = sample(trailMap, pos + sensorOffset(heading));
float right  = sample(trailMap, pos + sensorOffset(heading + sensorAngle));

if (center > left && center > right) {
    // Keep heading
} else if (left > right) {
    heading -= rotationAngle;
} else if (right > left) {
    heading += rotationAngle;
} else {
    heading += randomAngle();  // Equal: randomize
}

position += vec2(cos(heading), sin(heading)) * stepSize;
```

### Key Parameters

| Parameter | Typical Range | Effect |
|-----------|---------------|--------|
| Agent count | 100K–4M | Pattern density and performance |
| Sensor distance | 5–50 px | Trail detection range |
| Sensor angle | 20°–60° | Peripheral vision width |
| Rotation angle | 15°–90° | Turn sharpness |
| Step size | 1–5 px | Movement speed |
| Deposit amount | 1–10 | Trail intensity |
| Decay factor | 0.90–0.99 | Trail persistence |
| Diffusion kernel | 3x3–5x5 | Trail spread rate |

Lower decay produces sharp tendrils; higher decay creates smoother, cloudier patterns.

## Implementation Approach

Compute shader handles agent logic only. Existing `blur_v.fs` provides diffusion/decay.

**Architecture:**
```
SSBO Agent Buffer (position, heading, RGB per agent)
         ↓
[Compute Shader: Sense, Rotate, Move, Deposit]
         ↓
Trail Map Texture (drawn to accumTexture)
         ↓
[Existing blur_h.fs + blur_v.fs: Diffusion + Decay]
         ↓
(Existing voronoi, feedback, kaleidoscope passes)
```

No additional shaders beyond the agent compute shader. Trail map renders to `accumTexture` via `DrawFullscreenQuad()`, then existing blur passes handle diffusion/decay through the normal pipeline.

**Requirements:**
- Raylib compiled with `GRAPHICS_API_OPENGL_43`
- SSBO support (`rlLoadShaderBuffer`, `rlBindShaderBuffer`)
- `#version 430` shaders

**Reference:** [arceryz/raylib-gpu-particles](https://github.com/arceryz/raylib-gpu-particles) demonstrates raylib compute shader patterns.

## Hue-Based Attraction (Discrete Species)

Standard physarum uses scalar trail intensity—all agents attract to all trails, converging into unified networks. Hue-based attraction creates discrete species where agents cluster only with matching colors.

### Integration with ColorConfig

Physarum uses the same `ColorConfig` as waveforms. In `COLOR_MODE_RAINBOW`:

- **Per-agent:** Single `hue` value (0-1), uniformly distributed across `[rainbowHue, rainbowHue + rainbowRange]`
- **Shared uniforms:** `rainbowSat` and `rainbowVal` passed to shader

This means agents deposit `HSV(agent.hue, rainbowSat, rainbowVal)` and sense by comparing hue similarity.

In `COLOR_MODE_SOLID`, all agents share the same hue derived from `solid` color, effectively disabling species separation.

### Sensing Algorithm

Agents compare their hue to sensed trail hue. Similar hues attract, dissimilar hues repel:

```glsl
// Extract hue from sensed trail color
vec3 trailRGB = imageLoad(target, sensorPos).rgb;
float trailHue = rgb2hue(trailRGB);

// Circular hue distance (0 = same, 0.5 = opposite on color wheel)
float hueDiff = min(abs(agentHue - trailHue), 1.0 - abs(agentHue - trailHue));

// Convert to attraction/repulsion score:
//   +1.0 = same hue (max attraction)
//    0.0 = 90° apart (neutral)
//   -1.0 = opposite hue (max repulsion)
float score = 1.0 - 4.0 * hueDiff;
```

When choosing direction:
- **Positive score** → turn toward that sensor (attraction)
- **Negative score** → turn away from that sensor (repulsion)

This creates strong species separation—agents don't just ignore different colors, they actively flee from them.

### Deposit Algorithm

Agents deposit their color at new position:

```glsl
vec3 depositColor = hsv2rgb(vec3(agent.hue, saturation, value));
vec4 current = imageLoad(target, coord);
imageStore(target, coord, current + vec4(depositColor * depositAmount, 0.0));
```

### Behavior

- Multiple independent network structures emerge based on hue
- Species with similar hues may partially merge
- Waveforms with matching `ColorConfig` attract physarum agents

### Implementation Notes

**Agent struct (16 bytes, SSBO-aligned):**
```cpp
struct PhysarumAgent {
    float x, y;      // Position
    float heading;   // Direction (radians)
    float hue;       // Species identity (0-1)
};
```

**Shader uniforms:**
```glsl
uniform float saturation;  // From ColorConfig.rainbowSat
uniform float value;       // From ColorConfig.rainbowVal
```

**Agent initialization:**
```cpp
float hueStart = color.rainbowHue / 360.0f;
float hueRange = color.rainbowRange / 360.0f;
agent.hue = hueStart + (i / (float)count) * hueRange;
```

### References

- [EvanLavender13/ComputeShader slime.glsl](https://github.com/EvanLavender13/ComputeShader/blob/main/app/src/main/resources/slime.glsl)
- [Sage Jenson physarum variations](https://cargocollective.com/sagejenson/physarum)

## Integration with AudioJones

### Pipeline Position

Insert before voronoi pass in `PostEffectBeginAccum()`:

```
ApplyPhysarumPass(pe, deltaTime, beatIntensity);  // NEW
ApplyVoronoiPass(pe);                              // Existing
ApplyFeedbackPass(pe, ...);                        // Existing
```

Physarum provides organic background texture. Voronoi cells overlay creates layered depth. Waveforms draw on top as foreground.

### Resource Requirements

| Resource | Type | Size |
|----------|------|------|
| Agent buffer | SSBO | 16 bytes × agentCount |
| Trail map | RenderTexture2D | screenWidth × screenHeight × RGBA32F |
| Compute shader | Shader | ~80 lines GLSL |

At 1920×1080 with 1M agents:
- SSBO: 16 MB
- Trail map: 32 MB (RGBA32F)
- Total VRAM: ~48 MB additional

### Audio-Reactive Parameters

| Parameter | Audio Source | Effect |
|-----------|--------------|--------|
| Deposit amount | `beatIntensity` | Pulse brightness on kicks |
| Decay factor | Low-frequency energy | Bass sustains trails longer |
| Rotation angle | High-frequency energy | Treble creates tighter turns |
| Step size | Tempo (BPM) | Speed follows music pace |
| Sensor angle | Mid-frequency energy | Mids widen peripheral sensing |

**Beat Response Example:**
```cpp
float deposit = baseDeposit + beatIntensity * 5.0f;  // 1-6 range
float decay = 0.95f + lowEnergy * 0.04f;             // 0.95-0.99 range
```

LFO automation (`LFOConfig`) can modulate parameters for non-beat movement (sensor angle sweep, step size oscillation).

### Intensity Control for Background Role

To function as background enhancer (not dominating visual):

1. **Low trail intensity**: Start with deposit amount 1-3, not 5-10
2. **High decay**: Use existing `halfLife` at higher values (0.3-0.5s) for subtle, ghostly trails
3. **Color mapping**: Desaturated blues/purples behind colorful waveforms
4. **Resolution scaling**: Render at 50% resolution, upscale for performance

Trail map draws directly to `accumTexture` via `DrawFullscreenQuad()`, same pattern as voronoi. Existing blur passes then process the combined result.

## Compute Shader Structure

Agent-only compute shader based on Jeff Jones' algorithm. Diffusion/decay handled by existing `blur_v.fs`.

```glsl
#version 430

layout(local_size_x = 1024) in;

struct Agent {
    vec2 position;
    float heading;
    float _pad;
};

layout(std430, binding = 0) buffer AgentBuffer {
    Agent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;

uniform float sensorDistance;
uniform float sensorAngle;
uniform float turningAngle;
uniform float stepSize;
uniform float depositAmount;
uniform vec2 resolution;

// Hash for stochastic behavior (Sage Jenson approach)
uint hash(uint state) {
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}

void main() {
    uint id = gl_GlobalInvocationID.x;
    Agent agent = agents[id];

    // Sensor positions (Jones 2010: three forward-facing sensors)
    vec2 frontDir = vec2(cos(agent.heading), sin(agent.heading));
    vec2 leftDir  = vec2(cos(agent.heading + sensorAngle), sin(agent.heading + sensorAngle));
    vec2 rightDir = vec2(cos(agent.heading - sensorAngle), sin(agent.heading - sensorAngle));

    vec2 frontPos = agent.position + frontDir * sensorDistance;
    vec2 leftPos  = agent.position + leftDir * sensorDistance;
    vec2 rightPos = agent.position + rightDir * sensorDistance;

    // Sample trail intensity at sensor positions
    float front = imageLoad(trailMap, ivec2(mod(frontPos, resolution))).r;
    float left  = imageLoad(trailMap, ivec2(mod(leftPos, resolution))).r;
    float right = imageLoad(trailMap, ivec2(mod(rightPos, resolution))).r;

    // Rotation decision (Jones 2010 algorithm)
    float rnd = float(hash(id)) / 4294967295.0;

    if (front > left && front > right) {
        // Keep heading
    } else if (front < left && front < right) {
        // Both sides stronger: random turn
        agent.heading += (rnd - 0.5) * turningAngle * 2.0;
    } else if (left > right) {
        agent.heading += turningAngle;
    } else if (right > left) {
        agent.heading -= turningAngle;
    }

    // Move forward
    agent.position += frontDir * stepSize;
    agent.position = mod(agent.position, resolution);

    // Deposit trail
    ivec2 coord = ivec2(agent.position);
    vec4 current = imageLoad(trailMap, coord);
    imageStore(trailMap, coord, current + vec4(depositAmount, 0.0, 0.0, 0.0));

    agents[id] = agent;
}
```

Existing pipeline handles trail processing:
- `blur_h.fs` + `blur_v.fs`: 5-tap Gaussian diffusion
- `blur_v.fs:38-43`: Exponential decay via `halfLife` uniform

## Performance Considerations

### Bottlenecks

| Stage | Limiting Factor | Mitigation |
|-------|-----------------|------------|
| Agent update | Memory bandwidth (SSBO reads/writes) | Coalesce agent access patterns |
| Deposit | Atomic writes to trail map | Accept artifacts or use `imageAtomicAdd()` |
| Diffuse/Decay | Texture bandwidth | Already mitigated: existing separable blur passes |

### Benchmarks (from existing implementations)

| Platform | Agent Count | Resolution | FPS |
|----------|-------------|------------|-----|
| RX 6700S | 2M | 1080p | 120 |
| Mid-range GPU | 1M | 1080p | 60 |

For AudioJones targeting 60 fps with room for other effects, 500K–1M agents appears safe on mid-range hardware.

### Optimization Strategies

1. **Half-resolution trail map**: Render at 960×540, upscale when drawing to accumTexture
2. **Agent LOD**: Reduce active agent count when beat intensity low
3. **Temporal spreading**: Update only 50% of agents per frame
4. **Texture format**: RGBA16F instead of RGBA32F if precision sufficient

## Comparison with Voronoi

| Aspect | Physarum | Voronoi |
|--------|----------|---------|
| Pattern type | Organic networks | Geometric cells |
| Animation | Flowing, evolving | Oscillating points |
| Compute cost | High (agents + diffusion) | Low (9 hashes/pixel) |
| Memory | Trail map + agent buffer | None (procedural) |
| Audio response | Natural pulse/flow | Edge thickness, scale |
| Visual weight | Can dominate | Subtle overlay |

Physarum suits slower, ambient sections; voronoi suits rhythmic, structured music.

## Implementation Phases

### Phase 1: Compute Shader Foundation
- Compile raylib with `GRAPHICS_API_OPENGL_43`
- Add compute shader loading to `PostEffect` struct
- Create SSBO for agent buffer via `rlLoadShaderBuffer`
- Implement agent update compute shader
- Verify agent positions wrap correctly

### Phase 2: Trail Map Integration
- Add trail map `RenderTexture2D` (RGBA32F format)
- Draw trail map to `accumTexture` via `DrawFullscreenQuad()`
- Insert `ApplyPhysarumPass()` in `PostEffectBeginAccum()`
- Verify existing blur passes handle diffusion/decay

### Phase 3: Configuration
- Add `PhysarumConfig` to `EffectConfig`
- Wire up UI sliders (agent count, sensor params, deposit amount)
- Preset integration

### Phase 4: Audio Reactivity
- Connect `beatIntensity` to deposit amount
- Map frequency bands to rotation/step size
- Add LFO modulation for sensor angle

### Phase 5: Polish
- Color palette configuration (trail tint, saturation)
- Resolution scaling option
- Agent count presets (subtle/medium/intense)
- Hue-based species variant

## Known Challenges

### Atomic Deposit Collisions

Multiple agents depositing to same pixel causes race conditions. Solutions:
- Accept minor artifacts (often invisible at high agent counts)
- Use `imageAtomicAdd()` with integer textures, convert to float for display

### OpenGL Version Requirements

Compute shaders require OpenGL 4.3. Raylib must compile with `GRAPHICS_API_OPENGL_43`.

**Detection at runtime:**
```cpp
if (!rlGetExtSupported("GL_ARB_compute_shader")) {
    // Disable physarum, show warning in UI
}
```

### Trail Map Clearing

Unlike voronoi (procedural, stateless), physarum maintains persistent trail state. Requires handling:
- Initial agent distribution (random positions, random headings)
- Trail map clear on preset change or resolution change
- Potential "warmup" period before trails become visible

## Sources

### Academic
- [Jeff Jones - Characteristics of Pattern Formation (2010)](https://pubmed.ncbi.nlm.nih.gov/20067403/) - Original algorithm paper
- [Mechanisms Inducing Parallel Computation (2015)](https://arxiv.org/abs/1511.05869) - Extended analysis

### Implementation References
- [Sage Jenson - Physarum Notes](https://cargocollective.com/sagejenson/physarum) - Detailed parameter exploration
- [arceryz/raylib-gpu-particles](https://github.com/arceryz/raylib-gpu-particles) - Raylib compute shader patterns
- [raylib compute shader example](https://github.com/raysan5/raylib/blob/master/examples/others/rlgl_compute_shader.c) - Official SSBO usage
- [EvanLavender13/ComputeShader](https://github.com/EvanLavender13/ComputeShader) - Hue-based species variant

### Video Tutorials
- Sebastian Lague - "Coding Adventure: Ant and Slime Simulations" (Unity/HLSL reference)

## Open Questions

- **TBD:** Whether OpenGL 4.3 requirement acceptable for AudioJones distribution
- **Needs investigation:** Performance impact of RGBA32F vs RGBA16F trail map formats
- **TBD:** Interaction with kaleidoscope effect (does tessellation break organic patterns?)
- **TBD:** Optimal agent count range for "background enhancer" role vs dominating visual
