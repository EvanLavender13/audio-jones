# Physarum Species Reintroduction

Reintroduce hue-based implicit species behavior to the physarum simulation. Agents with similar hues cluster together by sensing and following trails of similar color.

## Goal

Enable emergent species behavior where agents self-organize by color. Based on the ComputeShader reference implementation, agents compare their hue to sensed trail hue and turn toward similar colors. This creates natural separation without explicit species parameters.

## Current State

The physarum simulation was simplified to intensity-only in phase 1:

- `src/render/physarum.h:7-12` defines agent as `{x, y, heading, _pad}` with no color
- `src/render/physarum.cpp:19-27` initializes agents with random position/heading only
- `shaders/physarum_agents.glsl:39-43` samples trail as single float intensity
- `shaders/physarum_agents.glsl:71-80` steers based on intensity gradient only
- `shaders/physarum_trail.glsl:5-6` uses R32F format for diffusion
- `shaders/physarum_boost.fs:18` reads trail as single-channel intensity

The reference implementation (`ComputeShader/app/src/main/resources/compute.glsl:127-134`) uses hue difference for sensing:
```glsl
float sensor_front_v = abs(color_hsv.x - sensor_front_hsv.x);
```
Agents turn toward lower hue difference (similar colors).

## Algorithm

### Hue-Based Sensing

Each agent stores a hue value (0-1). When sensing:

1. Sample trail color at sensor positions (front, left, right)
2. Convert trail RGB to HSV, extract hue
3. Compute hue difference: `abs(agent_hue - trail_hue)`
4. Handle hue wraparound: `min(diff, 1.0 - diff)` since hue is circular
5. Turn toward sensor with **lowest** hue difference (most similar color)

### Agent Initialization

Distribute hue across agents using existing `ColorConfig`:

```cpp
float hue = (config.color.rainbowHue + (i / (float)count) * config.color.rainbowRange) / 360.0f;
hue = fmodf(hue, 1.0f);
```

| Parameter | Source | Purpose |
|-----------|--------|---------|
| `rainbowHue` | ColorConfig | Starting hue offset (0-360) |
| `rainbowRange` | ColorConfig | Hue span across agents (0-360) |
| `rainbowSat` | ColorConfig | Saturation for deposition |
| `rainbowVal` | ColorConfig | Value/brightness for deposition |

### Trail Deposition

Convert agent hue to RGB using constant saturation/value from config:

```glsl
vec3 depositColor = hsv2rgb(vec3(agent.hue, saturation, value));
vec4 current = imageLoad(trailMap, coord);
vec4 deposited = current + vec4(depositColor * depositAmount, 0.0);
imageStore(trailMap, coord, deposited);
```

### Boost Integration

Keep existing boost effect. Use trail luminance as intensity multiplier:

```glsl
vec3 trailColor = texture(trailMap, fragTexCoord).rgb;
float trail = dot(trailColor, vec3(0.299, 0.587, 0.114));  // Luminance
```

This maintains the headroom-limited boost behavior while allowing color trails.

## Integration

### PhysarumConfig Changes

Add `ColorConfig color` to `PhysarumConfig` (`src/render/physarum.h:14-26`):

```cpp
typedef struct PhysarumConfig {
    // ... existing fields ...
    ColorConfig color;  // Hue distribution for species
} PhysarumConfig;
```

### Agent Struct Changes

Add `hue` field to `PhysarumAgent` (`src/render/physarum.h:7-12`):

```cpp
typedef struct PhysarumAgent {
    float x;
    float y;
    float heading;
    float hue;  // Agent's hue identity (0-1), replaces _pad
} PhysarumAgent;
```

Note: `hue` replaces `_pad` - struct remains 16 bytes, GPU-aligned.

### Shader Uniform Additions

Add to `physarum_agents.glsl`:
- `uniform float saturation;` - From ColorConfig
- `uniform float value;` - From ColorConfig

Add HSV conversion functions (from reference `compute.glsl:51-66`).

### Trail Map Format Change

Change from R32F to RGBA32F:
- `src/render/physarum.cpp:37` - `RL_PIXELFORMAT_UNCOMPRESSED_R32` to `RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32`
- `shaders/physarum_trail.glsl:5-6` - `r32f` to `rgba32f`
- `shaders/physarum_agents.glsl:18` - `r32f` to `rgba32f`

### UI Integration

Add color controls after physarum checkbox in `src/ui/ui_panel_effects.cpp:42-54`:

```cpp
if (effects->physarum.enabled) {
    // ... existing sliders ...
    dropdowns.physarumColor = UIDrawColorControls(l, state, &effects->physarum.color, &state->physarumHueDragging);
}
```

## File Changes

| File | Change |
|------|--------|
| `src/render/physarum.h` | Add `hue` to agent struct, add `ColorConfig color` to config, add uniform locations |
| `src/render/physarum.cpp` | Initialize hue per agent, create RGBA32F trail maps, pass saturation/value uniforms |
| `shaders/physarum_agents.glsl` | Add HSV functions, hue-based sensing, color deposition |
| `shaders/physarum_trail.glsl` | Change to RGBA32F format, diffuse all 4 channels |
| `shaders/physarum_debug.fs` | Display trail color directly (remove grayscale conversion) |
| `shaders/physarum_boost.fs` | Compute luminance from RGB trail for intensity |
| `src/ui/ui_panel_effects.cpp` | Add color controls dropdown for physarum |
| `src/ui/ui_panel_effects.h` | Add `physarumColor` to `EffectsPanelDropdowns` |
| `src/config/preset.cpp` | Serialize/deserialize physarum color config |

## Phases

### Phase 1: Agent Hue + Initialization

**Scope**: CPU-side changes only. Shader still uses intensity-only.

1. Add `hue` field to `PhysarumAgent` (replaces `_pad`)
2. Add `ColorConfig color` to `PhysarumConfig`
3. Update `InitializeAgents()` to distribute hue based on ColorConfig
4. Add `saturationLoc`, `valueLoc` uniform locations to `Physarum` struct
5. Pass saturation/value uniforms in `PhysarumUpdate()`

**Validation**: Build succeeds, simulation runs unchanged (hue field ignored by shader).

### Phase 2: RGBA Trail Map

**Scope**: Change trail storage format. Diffusion works on color.

1. Change `CreateTrailMap()` to use `RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32`
2. Update `physarum_trail.glsl` to use `rgba32f` and diffuse vec4
3. Update image bindings in `PhysarumUpdate()` and `PhysarumProcessTrails()`
4. Update `physarum_debug.fs` to display RGB color directly

**Validation**: Trail map displays as black (no color deposited yet), diffusion still works.

### Phase 3: Hue-Based Sensing + Deposition

**Scope**: Core algorithm change in agent shader.

1. Add HSV conversion functions to `physarum_agents.glsl`
2. Add `hue` field to shader Agent struct
3. Add `saturation`, `value` uniforms
4. Change `sampleTrail()` to return vec4 color
5. Implement hue-based sensing with wraparound handling
6. Implement color deposition using agent hue + config sat/val

**Validation**: Agents cluster by color. Debug overlay shows colored trails.

### Phase 4: Boost + UI Integration

**Scope**: Polish and user controls.

1. Update `physarum_boost.fs` to compute luminance from RGB trail
2. Add `physarumColor` rectangle to `EffectsPanelDropdowns`
3. Add `physarumHueDragging` to `PanelState`
4. Call `UIDrawColorControls()` in `UIDrawEffectsPanel()`
5. Update `preset.cpp` to serialize/deserialize `physarum.color`

**Validation**: Color controls visible, boost effect works with colored trails, presets save/load color.

## Validation

- [ ] Agents with similar hues cluster together
- [ ] Agents with different hues form separate networks
- [ ] Debug overlay shows colored trails matching agent distribution
- [ ] Rainbow mode (360 range) produces full color spectrum
- [ ] Narrow range (e.g., 60) produces single-color clustering
- [ ] Solid mode works (all agents same hue = original intensity behavior)
- [ ] Boost effect brightens based on trail luminance
- [ ] Color controls appear in UI when physarum enabled
- [ ] Presets save and restore physarum color settings
- [ ] Build succeeds with no errors or warnings

## References

- Reference implementation: `ComputeShader/app/src/main/resources/compute.glsl`
- Agent initialization patterns: `ComputeShader/app/src/main/java/computeshader/slime/AgentUtil.java`
- Original simplification plan: `docs/plans/physarum-simplify-phase1.md`
- Existing color system: `src/render/color_config.h`
