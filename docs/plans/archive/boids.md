# Boids

GPU-accelerated flocking simulation using Craig Reynolds' algorithm. Agents follow three steering rules (separation, alignment, cohesion) with hue-weighted affinity so like-colored boids flock together.

## Post-Implementation Fixes

- **Toroidal wrapping**: Added `wrapDelta()` for correct neighbor detection across screen edges
- **Removed spectrumPos**: Unused FFT lookup field removed from agent struct
- **Hue-weighted separation**: Different-colored boids repel each other more strongly
- **Hue-weighted alignment**: Boids align velocity primarily with same-hued neighbors
- **Removed texture reaction**: Single-point sampling ineffective; Phase 8 removed the feature

## Current State

Hook points for integration:

- `src/simulation/physarum.h/.cpp` - Reference implementation for agent struct, SSBO, compute dispatch
- `src/simulation/trail_map.h/.cpp` - Shared diffusion/decay processing (reuse directly)
- `src/simulation/shader_utils.h/.cpp` - Shared shader loading (reuse directly)
- `src/config/effect_config.h:169` - Add `BoidsConfig boids;` alongside other simulations
- `src/config/preset.cpp:87` - Add JSON serialization macro after PhysarumConfig
- `src/render/post_effect.h:218` - Add `Boids* boids;` pointer
- `src/render/post_effect.cpp:330` - Initialize boids in `PostEffectInit()`
- `src/render/render_pipeline.cpp:67` - Add `ApplyBoidsPass()` after Physarum pass
- `src/render/shader_setup.h/.cpp` - Add `SetupBoidsTrailBoost()` for BlendCompositor
- `src/ui/imgui_effects.cpp:130` - Add UI section under SIMULATIONS group
- `src/automation/param_registry.cpp:13` - Register modulatable parameters

## Technical Implementation

**Source**: `docs/research/boids.md`, Craig Reynolds' Boids algorithm with hue affinity extension

### Agent Structure (32 bytes, GPU-aligned)

```glsl
struct BoidAgent {
    vec2 position;    // 8 bytes
    vec2 velocity;    // 8 bytes
    float hue;        // 4 bytes - agent's color identity (0-1)
    float spectrumPos;// 4 bytes - position in population for FFT lookup
    vec2 _pad;        // 8 bytes - pad to 32 bytes
};
```

### Main Update Loop

```glsl
void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= boids.length()) return;

    BoidAgent b = boids[id];

    vec2 v1 = cohesion(b);      // Steer toward flock center (hue-weighted)
    vec2 v2 = separation(b);    // Avoid crowding
    vec2 v3 = alignment(b);     // Match flock velocity
    vec2 v4 = textureReaction(b); // React to feedback texture

    b.velocity += v1 * cohesionWeight
                + v2 * separationWeight
                + v3 * alignmentWeight
                + v4 * textureWeight;

    // Clamp velocity magnitude
    float speed = length(b.velocity);
    if (speed > maxSpeed) b.velocity = (b.velocity / speed) * maxSpeed;
    if (speed < minSpeed) b.velocity = (b.velocity / speed) * minSpeed;

    b.position += b.velocity;
    b.position = mod(b.position, resolution); // Wrap boundaries

    deposit(b);
    boids[id] = b;
}
```

### Rule 1: Cohesion with Hue Affinity

Compute center of mass weighted by hue similarity. Similar-hue neighbors contribute more:

```glsl
vec2 cohesion(BoidAgent bJ) {
    vec2 center = vec2(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < numBoids; i++) {
        if (i == int(gl_GlobalInvocationID.x)) continue;
        BoidAgent b = boids[i];
        float dist = distance(b.position, bJ.position);

        if (dist < perceptionRadius) {
            // Hue affinity: 1.0 when same hue, decays with hue difference
            float hueDiff = min(abs(b.hue - bJ.hue), 1.0 - abs(b.hue - bJ.hue));
            float affinity = 1.0 - hueDiff * hueAffinity; // hueAffinity 0-2 range

            center += b.position * affinity;
            totalWeight += affinity;
        }
    }

    if (totalWeight < 0.001) return vec2(0.0);

    center /= totalWeight;
    return (center - bJ.position) * 0.01; // Gradual steering
}
```

### Rule 2: Separation

Steer away from nearby neighbors (no hue weighting - all boids avoid collision):

```glsl
vec2 separation(BoidAgent bJ) {
    vec2 avoid = vec2(0.0);

    for (int i = 0; i < numBoids; i++) {
        if (i == int(gl_GlobalInvocationID.x)) continue;
        BoidAgent b = boids[i];
        vec2 diff = bJ.position - b.position;
        float dist = length(diff);

        if (dist < separationRadius && dist > 0.0) {
            avoid += diff / dist; // Stronger push when closer
        }
    }

    return avoid;
}
```

### Rule 3: Alignment

Match average velocity of neighbors:

```glsl
vec2 alignment(BoidAgent bJ) {
    vec2 avgVelocity = vec2(0.0);
    int count = 0;

    for (int i = 0; i < numBoids; i++) {
        if (i == int(gl_GlobalInvocationID.x)) continue;
        BoidAgent b = boids[i];
        float dist = distance(b.position, bJ.position);

        if (dist < perceptionRadius) {
            avgVelocity += b.velocity;
            count++;
        }
    }

    if (count == 0) return vec2(0.0);

    avgVelocity /= float(count);
    return (avgVelocity - bJ.velocity) * 0.125;
}
```

### Rule 4: Texture Reaction

Sample feedback texture ahead, steer toward or away from bright areas:

```glsl
vec2 textureReaction(BoidAgent bJ) {
    if (length(bJ.velocity) < 0.001) return vec2(0.0);

    vec2 ahead = bJ.position + normalize(bJ.velocity) * sensorDistance;
    vec2 aheadUV = ahead / resolution;
    float luminance = dot(texture(feedbackTex, aheadUV).rgb, vec3(0.299, 0.587, 0.114));

    // attractMode: 1.0 = attract to bright, -1.0 = avoid bright
    vec2 towardBright = normalize(ahead - bJ.position);
    return towardBright * luminance * attractMode;
}
```

### Deposit

HSV to RGB deposit following Physarum pattern:

```glsl
void deposit(BoidAgent b) {
    ivec2 coord = ivec2(b.position);
    vec3 depositColor = hsv2rgb(vec3(b.hue, saturation, value));
    vec4 current = imageLoad(trailMap, coord);
    vec3 newColor = current.rgb + depositColor * depositAmount;

    // Proportional scaling to prevent overflow while preserving hue
    float maxChan = max(newColor.r, max(newColor.g, newColor.b));
    if (maxChan > 1.0) newColor /= maxChan;

    imageStore(trailMap, coord, vec4(newColor, 0.0));
}
```

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| enabled | bool | - | false | Enable simulation |
| agentCount | int | 1000-50000 | 10000 | Number of boids |
| perceptionRadius | float | 10-100 px | 50 | Neighbor detection range |
| separationRadius | float | 5-50 px | 20 | Crowding avoidance range |
| cohesionWeight | float | 0-2 | 1.0 | Strength of center-seeking |
| separationWeight | float | 0-2 | 1.5 | Strength of avoidance |
| alignmentWeight | float | 0-2 | 1.0 | Strength of velocity matching |
| hueAffinity | float | 0-2 | 1.0 | How strongly like-colors flock (0=ignore hue) |
| textureWeight | float | 0-2 | 0.0 | Strength of texture reaction |
| attractMode | float | -1 to 1 | 0.0 | -1=avoid bright, 1=attract to bright |
| sensorDistance | float | 10-100 px | 30 | How far ahead to sample texture |
| maxSpeed | float | 1-10 | 4.0 | Velocity clamp |
| minSpeed | float | 0-2 | 0.5 | Prevents stalling |
| depositAmount | float | 0.01-0.5 | 0.05 | Trail brightness |
| decayHalfLife | float | 0.1-5.0 s | 0.5 | Trail persistence |
| diffusionScale | int | 0-4 | 1 | Blur kernel size |
| boostIntensity | float | 0-5 | 0.0 | BlendCompositor intensity |
| blendMode | enum | 0-15 | BOOST | BlendCompositor mode |
| color | ColorConfig | - | - | Hue distribution (solid/gradient/rainbow) |

---

## Phase 1: Core Infrastructure

**Goal**: Create Boids module with agent buffer, config struct, and empty compute shader.

**Build**:
- `src/simulation/boids.h` - BoidAgent struct (32 bytes), BoidsConfig struct with all parameters, Boids struct with agentBuffer/computeProgram/trailMap/uniform locations, function declarations (BoidsSupported, BoidsInit, BoidsUninit, BoidsUpdate, BoidsProcessTrails, BoidsApplyConfig, BoidsReset, BoidsResize, BoidsBeginTrailMapDraw, BoidsEndTrailMapDraw)
- `src/simulation/boids.cpp` - Implement Init/Uninit/Reset/Resize, agent initialization with ColorConfig hue assignment (copy from Physarum), compute shader loading skeleton, empty Update that just returns

**Done when**: `BoidsInit()` creates agent SSBO and TrailMap without crashing.

---

## Phase 2: Compute Shader - Steering Behaviors

**Goal**: Implement the three steering rules with hue-weighted cohesion.

**Build**:
- `shaders/boids_agents.glsl` - Full compute shader with:
  - BoidAgent struct matching C struct
  - SSBO binding 0, trailMap image binding 1
  - Uniforms for all steering parameters
  - cohesion() with hue affinity weighting
  - separation() without hue weighting
  - alignment() without hue weighting
  - deposit() with HSV→RGB conversion
  - main() combining all rules, clamping velocity, wrapping position
- `src/simulation/boids.cpp` - Complete BoidsUpdate(): enable shader, set all uniforms, bind SSBO and trailMap, dispatch compute, memory barrier

**Done when**: Boids flock and deposit colored trails. Like-colored boids cluster together.

---

## Phase 3: Texture Reaction

**Goal**: Add texture-reactive steering from feedback buffer.

**Build**:
- `shaders/boids_agents.glsl` - Add:
  - `uniform sampler2D feedbackTex` at binding 2
  - `uniform float textureWeight`, `uniform float attractMode`, `uniform float sensorDistance`
  - textureReaction() function sampling ahead of movement direction
  - Add v4 contribution to main velocity update
- `src/simulation/boids.cpp` - Add uniform locations, bind feedbackTex (accumTexture) in Update

**Done when**: Boids react to bright areas in feedback buffer when textureWeight > 0.

---

## Phase 4: PostEffect Integration

**Goal**: Wire Boids into the render pipeline with BlendCompositor.

**Build**:
- `src/render/post_effect.h` - Add `Boids* boids;` to PostEffect struct
- `src/render/post_effect.cpp` - Initialize in PostEffectInit(), cleanup in PostEffectUninit(), resize in PostEffectResize(), clear in PostEffectClearFeedback()
- `src/render/shader_setup.h` - Declare SetupBoidsTrailBoost()
- `src/render/shader_setup.cpp` - Implement SetupBoidsTrailBoost() using BlendCompositorApply()
- `src/render/render_pipeline.cpp` - Add ApplyBoidsPass() (copy ApplyPhysarumPass pattern), call in RenderPipelineProcess() after Physarum, add trail boost render pass when boostIntensity > 0

**Done when**: Boids trails appear in final output with configurable blend mode.

---

## Phase 5: Config and Serialization

**Goal**: Add BoidsConfig to EffectConfig with JSON serialization.

**Build**:
- `src/config/effect_config.h` - Add `#include "simulation/boids.h"`, add `BoidsConfig boids;` to EffectConfig struct
- `src/config/preset.cpp` - Add NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT for BoidsConfig with all fields

**Done when**: Boids config saves/loads with presets.

---

## Phase 6: UI Panel

**Goal**: Add Boids section to Effects panel.

**Build**:
- `src/ui/imgui_effects.cpp` - Add static bool `sectionBoids`, add Boids section in SIMULATIONS group after Attractor Flow:
  - Checkbox for enabled
  - SliderInt for agentCount
  - SliderFloat for perceptionRadius, separationRadius
  - SliderFloat for cohesionWeight, separationWeight, alignmentWeight (0-2 range)
  - SliderFloat for hueAffinity (0-2 range)
  - SliderFloat for textureWeight, attractMode (-1 to 1), sensorDistance
  - SliderFloat for maxSpeed, minSpeed
  - SliderFloat for depositAmount, decayHalfLife, diffusionScale
  - SliderFloat for boostIntensity
  - Combo for blendMode
  - ImGuiDrawColorMode for color
  - Checkbox for debugOverlay

**Done when**: Full UI control over all Boids parameters.

---

## Phase 7: Modulation

**Goal**: Register steering weights for audio modulation.

**Build**:
- `src/automation/param_registry.cpp` - Add to PARAM_TABLE:
  - `{"boids.cohesionWeight", {0.0f, 2.0f}}`
  - `{"boids.separationWeight", {0.0f, 2.0f}}`
  - `{"boids.alignmentWeight", {0.0f, 2.0f}}`
- Add corresponding entries to GetParamTargets() pointing to `&effects->boids.cohesionWeight`, etc.
- `src/ui/imgui_effects.cpp` - Change steering weight sliders to ModulatableSlider() with param keys

**Done when**: Steering weights respond to audio modulation routes.

---

## Phase 8: Remove Broken Texture Reaction

**Goal**: Remove non-functional texture sampling feature. Single-point sampling cannot steer toward brightness; proper implementation requires multi-sensor approach which duplicates Physarum. Keep boids pure agent-to-agent.

**Build**:
- `shaders/boids_agents.glsl` - Remove textureReaction() function, remove feedbackTex sampler, remove v4 from velocity calculation
- `src/simulation/boids.h` - Remove textureWeight, attractMode, sensorDistance from BoidsConfig
- `src/simulation/boids.cpp` - Remove uniform locations and bindings for removed params, remove feedbackTex binding in BoidsUpdate()
- `src/config/preset.cpp` - Remove textureWeight, attractMode, sensorDistance from serialization macro
- `src/ui/imgui_effects.cpp` - Remove sliders for textureWeight, attractMode, sensorDistance

**Done when**: Boids simulation has no texture-related parameters. Three steering rules (separation, alignment, cohesion) plus hue affinity remain.
