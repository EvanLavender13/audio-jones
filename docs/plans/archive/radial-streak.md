# Radial Streak Accumulation

Post-processing effect that samples along radial or spiral paths from a center point, creating motion-blur-like streaking. Supports mode toggle (radial/spiral), Lissajous focal animation, and audio-modulatable spiral parameters.

## Current State

Relevant files to modify or reference:
- `src/config/turbulence_config.h` - simple config pattern to follow
- `src/config/infinite_zoom_config.h` - focal animation params pattern
- `shaders/infinite_zoom.fs` - accumulation + edge fade pattern
- `src/render/post_effect.h:28-93` - shader + uniform location declarations
- `src/render/post_effect.cpp:25-116` - shader loading + uniform caching
- `src/render/render_pipeline.cpp:87-106` - transform effect dispatch table
- `src/config/effect_config.h:13-29` - TransformEffectType enum
- `src/ui/imgui_effects.cpp:109-188` - effect UI sections pattern
- `src/config/preset.cpp:98-103` - JSON serialization macros
- `src/automation/param_registry.cpp:12-42` - param table + registration

## Phase 1: Config and Shader

**Goal**: Create the config struct and GLSL shader.

**Build**:
- Create `src/config/radial_streak_config.h` with:
  - `enabled` (bool)
  - `mode` (int: 0=radial, 1=spiral)
  - `samples` (int: 4-16)
  - `streakLength` (float: 0.1-1.0)
  - `spiralTwist` (float: radians)
  - `spiralTurns` (float: radians)
  - `focalAmplitude`, `focalFreqX`, `focalFreqY` (float: Lissajous params)
- Create `shaders/radial_streak.fs` implementing:
  - Radial mode: sample along `uv - center` direction
  - Spiral mode: sample along spiral path (angle increases as radius decreases)
  - Weighted accumulation (inner samples contribute more)
  - Edge fade near texture boundaries
  - Uniforms: time, mode, samples, streakLength, spiralTwist, spiralTurns, focalOffset

**Done when**: Config compiles, shader file exists with both modes implemented.

---

## Phase 2: Pipeline Integration

**Goal**: Wire shader into render pipeline as 5th transform effect.

**Build**:
- `src/config/effect_config.h`:
  - Add `TRANSFORM_RADIAL_STREAK` to enum (before `TRANSFORM_EFFECT_COUNT`)
  - Add `#include "radial_streak_config.h"`
  - Add `RadialStreakConfig radialStreak;` to EffectConfig
  - Update `TransformEffectName()` switch
  - Add to default `transformOrder[]` array
- `src/render/post_effect.h`:
  - Add `Shader radialStreakShader;`
  - Add uniform location ints for all params
  - Add `float radialStreakTime;` and `float radialStreakFocal[2];`
- `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Cache uniform locations in `GetShaderUniformLocations()`
  - Unload shader in `PostEffectUninit()`
  - Initialize time to 0 in `PostEffectInit()`
- `src/render/render_pipeline.cpp`:
  - Add `SetupRadialStreak()` function
  - Add case to `GetTransformEffect()` dispatch
  - Increment `radialStreakTime` in `RenderPipelineApplyFeedback()`
  - Compute Lissajous focal in `RenderPipelineApplyOutput()`

**Done when**: Effect toggles on/off, renders with default params.

---

## Phase 3: UI, Serialization, and Modulation

**Goal**: Expose controls, persist to presets, enable audio modulation.

**Build**:
- `src/ui/imgui_effects.cpp`:
  - Add `static bool sectionRadialStreak = false;`
  - Add section after Turbulence with:
    - Enabled checkbox
    - Mode combo (Radial/Spiral)
    - Samples slider (int, 4-16)
    - Streak Length slider (float)
    - Spiral Twist modulatable slider (angle)
    - Spiral Turns modulatable slider (angle)
    - Focal Amplitude, FreqX, FreqY sliders
  - Update Effect Order list switch for enabled dimming
- `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for RadialStreakConfig
  - Add `j["radialStreak"]` in EffectConfig `to_json`/`from_json`
- `src/automation/param_registry.cpp`:
  - Add entries to `PARAM_TABLE[]`:
    - `{"radialStreak.spiralTwist", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}}`
    - `{"radialStreak.spiralTurns", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}}`
  - Add corresponding pointers in `ParamRegistryInit()`

**Done when**: UI controls work, presets save/load correctly, spiral params respond to modulation.
