# Physarum Scene-Sampling Architecture

Redesign physarum from a standalone simulation to a reactive transport effect that carries audio-driven visual content along organic trail networks.

## Design Philosophy

**Current approach**: Physarum generates its own colored trails. Audio waveforms render on top. Two independent visual layers compete for attention.

**Proposed approach**: Audio waveforms and spectrum bars are the primary visual content. Physarum agents sample and transport that content along their paths. The effect amplifies and spreads audio-reactive visuals rather than replacing them.

This aligns with AudioJones's purpose: real-time audio visualization. Effects should enhance the audio-visual connection, not obscure it.

## Architecture Overview

```
                    ┌─────────────────────────────────────┐
                    │           Render Pipeline           │
                    └─────────────────────────────────────┘
                                      │
         ┌────────────────────────────┼────────────────────────────┐
         ▼                            ▼                            ▼
┌─────────────────┐          ┌─────────────────┐          ┌─────────────────┐
│   trailMap      │          │  accumTexture   │          │   tempTexture   │
│   (R32F)        │          │  (RGBA32F)      │          │   (RGBA32F)     │
│   Grayscale     │          │  RGB Scene      │          │   Ping-pong     │
└────────┬────────┘          └────────┬────────┘          └─────────────────┘
         │                            │
         │ sense intensity            │ sample color
         │                            │ deposit color
         ▼                            ▼
    ┌─────────────────────────────────────┐
    │         Physarum Compute            │
    │                                     │
    │  1. Sense trailMap (3 sensors)      │
    │  2. Steer toward same-species trails│
    │  3. Move forward                    │
    │  4. Sample RGB from accumTexture    │
    │  5. Deposit sampled RGB back        │
    │  6. Deposit intensity to trailMap   │
    └─────────────────────────────────────┘
         │
         ▼
    ┌─────────────────────────────────────┐
    │      Physarum Diffuse/Decay         │
    │      (Dedicated pass on trailMap)   │
    │                                     │
    │  - Box blur or Gaussian             │
    │  - Configurable decay rate          │
    │  - Independent of main pipeline     │
    └─────────────────────────────────────┘
```

## Key Changes from Current Implementation

### 1. Separate Trail Map for Steering

**Current** (`shaders/physarum_agents.glsl:21`): Agents sense and deposit to the same `accumTexture`.

**Proposed**:
- `trailMap` (R32F): Grayscale intensity for steering. Dedicated diffuse/decay.
- `accumTexture` (RGBA32F): RGB scene content. Agents sample and deposit colors here.

Rationale: Decouples steering dynamics from visual output. Trail persistence and diffusion tune agent behavior without affecting scene color processing.

### 2. Integer Species Instead of Hue-Based

**Current** (`shaders/physarum_agents.glsl:118-128`): Circular hue distance computes attraction/repulsion.

**Proposed**:
```glsl
struct Agent {
    float x, y;
    float heading;
    uint species;  // 0 to speciesCount-1
};

// Species stored in trailMap alpha or separate channel
// Same species: attract (positive score)
// Different species: repel (negative score)
```

UI exposes single integer slider: "Species Count" (1-8 typical).

Rationale: Simpler configuration. Discrete species create cleaner domain boundaries. Standard approach in physarum literature.

### 3. Scene Color Sampling for Deposit

**Current** (`shaders/physarum_agents.glsl:183`): Deposit color computed from agent hue via HSV→RGB.

**Proposed**:
```glsl
// Sample whatever color exists at agent position
vec3 sceneColor = imageLoad(accumTexture, ivec2(pos)).rgb;

// Deposit with intensity boost
vec3 deposited = current.rgb + sceneColor * depositAmount;
imageStore(accumTexture, ivec2(pos), vec4(deposited, 1.0));
```

Rationale: Agents "carry" waveform colors, spectrum bar colors, feedback artifacts. Visual output reflects audio content.

### 4. Decoupled Physarum Diffuse/Decay

**Current** (`src/render/post_effect.cpp:269-272`): Physarum shares blur/decay with feedback and voronoi.

**Proposed**: Dedicated diffuse/decay pass on `trailMap` only.

```cpp
void PostEffectBeginAccum(...) {
    // 1. Physarum steering/movement (reads trailMap, writes trailMap + accumTexture)
    ApplyPhysarumPass(pe, deltaTime);

    // 2. Physarum trail diffuse/decay (trailMap only, separate shader)
    ApplyPhysarumDiffusePass(pe, deltaTime);

    // 3. Standard pipeline continues (accumTexture only)
    ApplyVoronoiPass(pe);
    ApplyFeedbackPass(pe, effectiveRotation);
    ApplyBlurPass(pe, blurScale, deltaTime);  // accumTexture blur/decay
}
```

Rationale: Physarum trail dynamics tune independently. Main pipeline blur/decay affects visual output without altering agent steering signals.

## Agent Behavior with Scene Sampling

| Scene State | Agent Behavior |
|-------------|----------------|
| Waveform draws bright arc | Agents near arc sample bright color, deposit along paths, spread the arc |
| Spectrum bar pulses red | Red bleeds outward along agent trails toward trail-dense regions |
| Black empty region | Agents deposit black (invisible), but still leave trail intensity for steering |
| Feedback creates spiral | Agents carry spiral colors, organic mixing along species boundaries |

## Pipeline Order (Revised)

```
1. PostEffectBeginAccum()
   a. ApplyPhysarumPass()           # Agents sense trailMap, deposit to accumTexture + trailMap
   b. ApplyPhysarumDiffusePass()    # Blur/decay trailMap only
   c. ApplyVoronoiPass()            # Optional overlay on accumTexture
   d. ApplyFeedbackPass()           # Zoom/rotate/desaturate accumTexture
   e. ApplyBlurPass()               # Blur/decay accumTexture

2. Draw waveforms/spectrum to accumTexture

3. PostEffectEndAccum()
   a. ApplyKaleidoPass()            # Optional

4. PostEffectToScreen()
   a. ApplyChromatic()              # Optional
   b. Draw accumTexture to screen
```

Waveforms draw *after* physarum samples, so agents pick up colors from the *previous* frame's waveform. This creates a one-frame delay but ensures agents always have content to sample.

## Data Structures

### PhysarumAgent (Revised)

```cpp
struct PhysarumAgent {
    float x;
    float y;
    float heading;
    uint32_t species;  // 0 to speciesCount-1
};
```

16 bytes per agent (unchanged size, different layout).

### PhysarumConfig (Revised)

```cpp
struct PhysarumConfig {
    bool enabled = false;
    int agentCount = 100000;
    int speciesCount = 3;           // NEW: replaces ColorConfig
    float sensorDistance = 20.0f;
    float sensorAngle = 0.4f;
    float turningAngle = 0.3f;
    float stepSize = 1.0f;
    float depositAmount = 0.5f;
    float trailDecay = 0.1f;        // NEW: dedicated trail decay
    float trailDiffuse = 1.0f;      // NEW: dedicated trail blur scale
};
```

Removes `ColorConfig color` field entirely.

### Physarum Struct (Revised)

```cpp
struct Physarum {
    // ... existing fields ...
    RenderTexture2D trailMap;       // NEW: R32F grayscale
    Shader trailDiffuseShader;      // NEW: dedicated blur/decay
    // Remove: color-related uniforms
    int speciesCountLoc;            // NEW
    int trailDecayLoc;              // NEW
};
```

## Shader Changes

### physarum_agents.glsl (Revised)

```glsl
layout(std430, binding = 0) buffer AgentBuffer {
    Agent agents[];
};

layout(rgba32f, binding = 1) uniform image2D accumTexture;  // RGB scene
layout(r32f, binding = 2) uniform image2D trailMap;         // Grayscale intensity

uniform int speciesCount;

// Sense trail intensity at position, weighted by species match
float sampleTrail(vec2 pos, uint agentSpecies) {
    ivec2 coord = ivec2(mod(pos, resolution));
    float intensity = imageLoad(trailMap, coord).r;

    // Species encoding TBD (separate channel or SSBO lookup)
    // Same species: +intensity
    // Different species: -intensity * 0.5

    return intensity;  // Simplified for now
}

void main() {
    Agent agent = agents[id];
    vec2 oldPos = vec2(agent.x, agent.y);

    // Sample scene color BEFORE moving (for directional transport)
    vec3 sceneColor = imageLoad(accumTexture, ivec2(oldPos)).rgb;

    // Sense (steering from trailMap at sensor positions)
    vec2 frontDir = vec2(cos(agent.heading), sin(agent.heading));
    vec2 leftDir = vec2(cos(agent.heading + sensorAngle), sin(agent.heading + sensorAngle));
    vec2 rightDir = vec2(cos(agent.heading - sensorAngle), sin(agent.heading - sensorAngle));

    float front = sampleTrail(oldPos + frontDir * sensorDistance, agent.species);
    float left = sampleTrail(oldPos + leftDir * sensorDistance, agent.species);
    float right = sampleTrail(oldPos + rightDir * sensorDistance, agent.species);

    // Turn (standard Jones algorithm)
    // ... existing turning logic ...

    // Move to new position
    vec2 moveDir = vec2(cos(agent.heading), sin(agent.heading));
    vec2 newPos = oldPos + moveDir * stepSize;
    newPos = mod(newPos, resolution);

    // Deposit sampled color at NEW position (color transport)
    ivec2 newCoord = ivec2(newPos);
    vec4 current = imageLoad(accumTexture, newCoord);
    vec3 deposited = current.rgb + sceneColor * depositAmount;
    imageStore(accumTexture, newCoord, vec4(deposited, 1.0));

    // Deposit trail intensity at new position (for future sensing)
    float currentTrail = imageLoad(trailMap, newCoord).r;
    imageStore(trailMap, newCoord, vec4(currentTrail + depositAmount, 0, 0, 1));

    agent.x = newPos.x;
    agent.y = newPos.y;
    agents[id] = agent;
}
```

**Sampling order options** (test both):

| Approach | Sample Position | Deposit Position | Effect |
|----------|-----------------|------------------|--------|
| **Transport** (above) | Old position | New position | Colors flow along agent direction |
| **Reinforce** | New position | New position | Colors intensify in place along trails |

Transport creates directional smearing. Reinforce creates localized brightening. Test both to determine which integrates better with audio-reactive content.

### physarum_trail_diffuse.fs (New)

```glsl
#version 330

in vec2 fragTexCoord;
uniform sampler2D trailMap;
uniform vec2 resolution;
uniform float decay;
uniform int blurScale;

out vec4 finalColor;

void main() {
    // Box blur on grayscale trail
    float result = 0.0;
    vec2 texelSize = (1.0 / resolution) * float(blurScale);

    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(trailMap, fract(fragTexCoord + offset)).r;
        }
    }
    result /= 25.0;

    // Apply decay
    result *= (1.0 - decay);

    finalColor = vec4(result, 0, 0, 1);
}
```

## UI Changes

Remove from physarum panel (`src/ui/ui_panel_effects.cpp`):
- Color mode selector
- Hue/saturation/value sliders
- Rainbow range controls

Add to physarum panel:
- "Species" integer slider (1-8)
- "Trail Decay" float slider (0.01-0.5)
- "Trail Diffuse" float slider (0-5)

## Migration Path

### Phase 1: Add Trail Map Infrastructure

1. Add `trailMap` (R32F) to `Physarum` struct
2. Add `physarum_trail_diffuse.fs` shader
3. Add `ApplyPhysarumDiffusePass()` function
4. Existing behavior unchanged (agents still use accumTexture for sensing)

### Phase 2: Decouple Sensing to Trail Map

1. Modify `physarum_agents.glsl` to sense from `trailMap`
2. Deposit intensity to `trailMap` in addition to color deposit
3. Test: agents should steer based on trail intensity, not scene color

### Phase 3: Scene Color Sampling

1. Remove hue field from `PhysarumAgent`
2. Sample `accumTexture` for deposit color instead of HSV conversion
3. Remove `ColorConfig` from `PhysarumConfig`
4. Update UI

### Phase 4: Integer Species

1. Add `species` field to `PhysarumAgent`
2. Implement species-aware trail sensing
3. Add species count slider to UI

## Open Questions

- **Species encoding**: Store in separate texture channel, trail map alpha, or SSBO lookup?
- **Trail map resolution**: Same as screen, or lower for performance?
- **Interaction with feedback**: Should feedback also transform trailMap, or only accumTexture?
- **Sampling order**: Transport (sample old, deposit new) vs Reinforce (sample new, deposit new) - which integrates better with audio content?

## Validation

- [ ] Waveform colors spread along agent paths
- [ ] Species boundaries visible as trail density variations
- [ ] Trail decay and main pipeline decay tune independently
- [ ] No visible artifacts from separate trail map
- [ ] Frame time impact <2ms at 1080p with 100K agents

## References

- `docs/research/physarum-simulation.md` - Algorithm background
- `docs/research/physarum-audiojones-vs-computeshader.md` - Implementation comparison
- `docs/plans/physarum-exploration.md` - Related experiments
