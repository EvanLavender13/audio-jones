# Physarum Accumulation Texture Sensing

Physarum agents sense the waveform accumulation buffer using hue-based affinity, enabling them to follow or avoid audio-reactive content matching their species color.

## Goal

Make physarum respond to visualized audio by sensing hue in the accumulated waveform texture. Agents follow waveforms matching their hue species and turn away from different hues, creating reactive behavior where colored waveforms attract matching physarum populations.

## Current State

- `shaders/physarum_agents.glsl:71-83` implements `sampleTrailAffinity()` using hue-based sensing
- `shaders/physarum_agents.glsl:104-107` samples affinity at three sensor positions (front/left/right)
- `shaders/physarum_agents.glsl:112-124` steers agents toward lowest affinity (most similar hue)
- `src/render/physarum.cpp:317-318` binds agentBuffer at 0, trailMap at binding 1
- `src/render/post_effect.cpp:294` calls `ApplyPhysarumPass()` before waveforms are drawn, so agents sense previous frame's accumTexture (enables smooth following)
- Binding 2 is available for accumTexture

## Algorithm

Blend between trail sensing and accumTexture sensing. Both use combined hue+intensity affinity: agents prefer dense areas of similar hue. The intensity component (weighted 0.3) ensures solid color mode produces coherent patterns instead of diffuse clouds.

### Shader Implementation

```glsl
// New bindings and uniforms
layout(binding = 2) uniform sampler2D accumMap;
uniform float accumSenseBlend;  // 0 = trail only, 1 = accum only

// Modified trail sampling: blend hue similarity with intensity
float sampleTrailAffinity(vec2 pos, float agentHue)
{
    ivec2 coord = ivec2(mod(pos, resolution));
    vec3 trailColor = imageLoad(trailMap, coord).rgb;
    float trailIntensity = dot(trailColor, LUMA_WEIGHTS);

    if (trailIntensity < 0.001) {
        return 1.0;  // No trail = least attractive
    }

    vec3 trailHSV = rgb2hsv(trailColor);
    float hueDiff = hueDifference(agentHue, trailHSV.x);

    // Blend hue similarity with intensity attraction
    return hueDiff + (1.0 - trailIntensity) * 0.3;
}

// New function: sample accumTexture with same blended logic
float sampleAccumAffinity(vec2 pos, float agentHue)
{
    vec2 uv = pos / resolution;
    vec3 accumColor = texture(accumMap, uv).rgb;
    float intensity = dot(accumColor, LUMA_WEIGHTS);

    if (intensity < 0.001) {
        return 1.0;  // No content = least attractive
    }

    vec3 accumHSV = rgb2hsv(accumColor);
    float hueDiff = hueDifference(agentHue, accumHSV.x);

    return hueDiff + (1.0 - intensity) * 0.3;
}

// Modified sensing in main():
float trailFront = sampleTrailAffinity(frontPos, agent.hue);
float accumFront = sampleAccumAffinity(frontPos, agent.hue);
float front = mix(trailFront, accumFront, accumSenseBlend);

float trailLeft = sampleTrailAffinity(leftPos, agent.hue);
float accumLeft = sampleAccumAffinity(leftPos, agent.hue);
float left = mix(trailLeft, accumLeft, accumSenseBlend);

float trailRight = sampleTrailAffinity(rightPos, agent.hue);
float accumRight = sampleAccumAffinity(rightPos, agent.hue);
float right = mix(trailRight, accumRight, accumSenseBlend);
```

### Parameter Behavior

| accumSenseBlend | Behavior |
|-----------------|----------|
| 0.0 | Original behavior (sense only own trails) |
| 0.5 | Balanced sensing of both sources |
| 1.0 | Sense only waveform content (ignore trails) |

### Sensing Logic

Affinity blends hue similarity with intensity attraction:

```glsl
return hueDiff + (1.0 - intensity) * 0.3;
```

| Condition | Affinity | Agent Response |
|-----------|----------|----------------|
| Same hue, high intensity | ~0.0 | Maximum attraction |
| Same hue, low intensity | ~0.3 | Moderate attraction |
| Different hue, high intensity | ~0.2-0.5 | Weak attraction |
| Different hue, low intensity | ~0.5-0.8 | Turn away |
| No content | 1.0 | Neutral - no preference |

This blended approach fixes solid color mode where pure hue-based sensing fails (all agents have identical hue, so all trails look equally attractive). With intensity blending, agents in solid color mode follow intensity gradients toward denser trail areas, producing coherent patterns instead of diffuse clouds.

## Integration

1. **PhysarumConfig** (`src/render/physarum.h:15-28`): Add `accumSenseBlend` field with default 0.0
2. **Physarum struct** (`src/render/physarum.h:30-57`): Add `accumSenseBlendLoc` uniform location
3. **PhysarumInit** (`src/render/physarum.cpp`): Get uniform location for `accumSenseBlend`
4. **PhysarumUpdate signature** (`src/render/physarum.h:70`): Add `Texture2D accumTexture` parameter
5. **PhysarumUpdate** (`src/render/physarum.cpp:286-328`): Set uniform, bind accumTexture as sampler2D at binding 2
6. **ApplyPhysarumPass** (`src/render/post_effect.cpp:127-144`): Pass `pe->accumTexture.texture` to PhysarumUpdate
7. **UI** (`src/ui/ui_panel_effects.cpp:39-57`): Add slider inside physarum enabled block
8. **Preset serialization** (`src/config/preset.cpp:16-19`): Add field to NLOHMANN macro

## File Changes

| File | Change |
|------|--------|
| `shaders/physarum_agents.glsl` | Add accumMap sampler (binding 2), accumSenseBlend uniform, sampleAccumAffinity function, modify sampleTrailAffinity to blend hue+intensity, modify sensing to blend both sources |
| `src/render/physarum.h` | Add `accumSenseBlend` to PhysarumConfig (default 0.0), add `accumSenseBlendLoc` to Physarum struct, update PhysarumUpdate signature |
| `src/render/physarum.cpp` | Get accumSenseBlend uniform location in init, set uniform and bind accumTexture in PhysarumUpdate |
| `src/render/post_effect.cpp` | Pass accumTexture to PhysarumUpdate call |
| `src/ui/ui_panel_effects.cpp` | Add accumSenseBlend slider (0.0-1.0) inside physarum enabled block |
| `src/config/preset.cpp` | Add accumSenseBlend to PhysarumConfig serialization macro |

## Validation

- [ ] With accumSenseBlend = 0, behavior identical to current (agents follow only their trails)
- [ ] With accumSenseBlend = 1, agents visibly cluster around waveform content matching their hue
- [ ] Agents turn away from waveform content with different hues
- [ ] Smooth transition when adjusting blend slider
- [ ] Parameter persists across preset save/load
- [ ] No visual artifacts when accumTexture is empty (agents behave as if sensing trails only)
- [ ] No performance regression (accumTexture already exists, just binding it)
- [ ] Solid color mode produces coherent trail patterns (intensity-based clustering)
