# Log-Polar Spiral Effect

Transform effect that converts to log-polar coordinates, applies spiral distortion, and blends multiple zoom layers for an infinite tunnel/vortex aesthetic.

**Reference**: See [docs/research/heavy-texture-transforms.md](../research/heavy-texture-transforms.md#technique-5-log-polar-spiral-blur) for algorithm details and GLSL implementation.

## Current State

Existing infrastructure to hook into:

- `src/config/infinite_zoom_config.h` - template for config struct
- `src/config/effect_config.h:13-29` - TransformEffectType enum and name switch
- `src/render/post_effect.h:28,77-82,109-110` - shader handle, uniform locs, time accum pattern
- `src/render/post_effect.cpp:38,99-104,148,212` - shader load/unload/uniforms pattern
- `src/render/render_pipeline.cpp:92-103,265-279` - GetTransformEffect switch and setup callback
- `src/ui/imgui_effects.cpp:173-185` - Infinite Zoom UI section pattern
- `src/ui/ui_units.h:31-35` - ModulatableSliderAngleDeg pattern (need TurnsDeg variant)
- `src/config/preset.cpp:98-99,119,142` - JSON serialization pattern

---

## Phase 1: Config and Shader

**Goal**: Create config struct and core shader.

**Create**:
- `src/config/log_polar_spiral_config.h` - config struct:
  - `enabled` (bool, default false)
  - `speed` (float, 0.1-2.0, default 1.0) - animation speed
  - `zoomDepth` (float, 1.0-5.0, default 3.0) - zoom range in powers of 2
  - `focalAmplitude` (float, 0.0-0.2, default 0.0) - Lissajous center offset
  - `focalFreqX` (float, 0.1-5.0, default 1.0)
  - `focalFreqY` (float, 0.1-5.0, default 1.5)
  - `layers` (int, 2-8, default 6)
  - `spiralTwist` (float, radians, default 0.0) - angle varies with log-radius
  - `spiralTurns` (float, turns, default 0.0) - rotation per zoom cycle

- `shaders/log_polar_spiral.fs` - fragment shader implementing:
  - `toLogPolar()` / `fromLogPolar()` coordinate transforms
  - Multi-layer loop with exponential scale
  - Spiral twist: `lp.y += lp.x * spiralTwist`
  - Per-layer rotation: `lp.y += phase * spiralTurns * TWO_PI`
  - Cosine alpha weighting, edge softening, weighted accumulation

**Done when**: Config compiles and shader file exists.

---

## Phase 2: Effect Registration

**Goal**: Register as transform effect in the chain.

**Modify**:
- `src/config/effect_config.h`:
  - Add `#include "log_polar_spiral_config.h"`
  - Add `TRANSFORM_LOG_POLAR_SPIRAL` to enum (before `TRANSFORM_EFFECT_COUNT`)
  - Add case to `TransformEffectName()` switch
  - Add `LogPolarSpiralConfig logPolarSpiral;` member to `EffectConfig`
  - Add to default `transformOrder[]` array

**Done when**: Build succeeds with new enum value.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Modify**:
- `src/render/post_effect.h`:
  - Add `Shader logPolarSpiralShader;`
  - Add uniform locations: `logPolarSpiralTimeLoc`, `logPolarSpiralSpeedLoc`, `logPolarSpiralZoomDepthLoc`, `logPolarSpiralFocalLoc`, `logPolarSpiralLayersLoc`, `logPolarSpiralSpiralTwistLoc`, `logPolarSpiralSpiralTurnsLoc`
  - Add `float logPolarSpiralTime;` and `float logPolarSpiralFocal[2];`

- `src/render/post_effect.cpp`:
  - Add `LoadShader(0, "shaders/log_polar_spiral.fs")` in shader loading
  - Add shader ID check to success validation
  - Add `GetShaderLocation()` calls for all uniforms
  - Initialize `logPolarSpiralTime = 0.0f`
  - Add `UnloadShader(pe->logPolarSpiralShader)` in cleanup

**Done when**: Build succeeds with shader loaded.

---

## Phase 4: Pipeline Integration

**Goal**: Execute shader pass via transform effect dispatch.

**Modify**:
- `src/render/render_pipeline.cpp`:
  - Add `static void SetupLogPolarSpiral(PostEffect* pe);` forward declaration
  - Add `TRANSFORM_LOG_POLAR_SPIRAL` case to `GetTransformEffect()` switch
  - Implement `SetupLogPolarSpiral()` callback - set all uniforms from config
  - Add time accumulation: `pe->logPolarSpiralTime += deltaTime;` in feedback loop
  - Add Lissajous focal computation in `RenderPipelineApplyOutput()`

**Done when**: Effect renders when enabled.

---

## Phase 5: UI Controls

**Goal**: Add user-facing controls with audio modulation.

**Modify**:
- `src/ui/ui_units.h`:
  - Add `ModulatableSliderTurnsDeg()` helper (parallel to `ModulatableSliderAngleDeg` but using `TURNS_TO_DEG`)

- `src/ui/imgui_effects.cpp`:
  - Add `static bool sectionLogPolarSpiral = false;`
  - Add `TRANSFORM_LOG_POLAR_SPIRAL` case to effect order enabled-check switch
  - Add collapsible section after Infinite Zoom:
    - Checkbox for `enabled`
    - SliderFloat for `speed` (0.1-2.0)
    - ModulatableSlider for `zoomDepth` (1.0-5.0) - bass modulation target
    - SliderFloat for `focalAmplitude`, `focalFreqX`, `focalFreqY`
    - SliderInt for `layers` (2-8)
    - ModulatableSliderAngleDeg for `spiralTwist` - mids modulation target
    - ModulatableSliderTurnsDeg for `spiralTurns` - treble modulation target

**Done when**: All controls visible and functional.

---

## Phase 6: Preset Serialization

**Goal**: Save/load effect settings in presets.

**Modify**:
- `src/config/preset.cpp`:
  - Add `#include "config/log_polar_spiral_config.h"`
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LogPolarSpiralConfig, ...)`
  - Add `logPolarSpiral` to `EffectConfig` to_json/from_json

**Done when**: Presets persist settings across save/load.

---

## Phase 7: Consolidate into Infinite Zoom

**Goal**: Add radius-dependent twist (`spiralTwist`) to infinite_zoom alongside existing uniform twist (`spiralTurns`). Remove redundant log-polar spiral effect.

**Two twist modes** (can be combined):
- `spiralTurns`: uniform rotation per layer (existing)
- `spiralTwist`: radius-dependent rotation via `log(r)` (tighter near center, looser at edges)

**Delete**:
- `src/config/log_polar_spiral_config.h`
- `shaders/log_polar_spiral.fs`

**Modify**:
- `src/config/infinite_zoom_config.h`:
  - Add `float spiralTwist = 0.0f;` - radius-dependent twist (radians)

- `shaders/infinite_zoom.fs`:
  - Add `uniform float spiralTwist;`
  - Change alpha calculation to include scale weighting:
    ```glsl
    float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5 / scale;
    ```
  - Reorder operations: scale BEFORE twist (twist uses scaled radius):
    ```glsl
    vec2 uv = fragTexCoord - center;

    // Uniform rotation (spiralTurns) - can stay before or after scale
    if (spiralTurns != 0.0) {
        float angle = phase * spiralTurns * TWO_PI;
        float cosA = cos(angle), sinA = sin(angle);
        uv = vec2(uv.x * cosA - uv.y * sinA, uv.x * sinA + uv.y * cosA);
    }

    // Scale (zoom out)
    uv = uv / scale;

    // Radius-dependent twist - MUST be after scale
    // log(length(uv)) now equals log(original_r) - phase*zoomDepth*LOG2
    if (spiralTwist != 0.0) {
        float logR = log(length(uv) + 0.001);
        float twistAngle = logR * spiralTwist;
        float cosT = cos(twistAngle), sinT = sin(twistAngle);
        uv = vec2(uv.x * cosT - uv.y * sinT, uv.x * sinT + uv.y * cosT);
    }

    // Recenter
    uv = uv + center;
    ```

- `src/config/effect_config.h`:
  - Remove `TRANSFORM_LOG_POLAR_SPIRAL` enum value
  - Remove `LogPolarSpiralConfig logPolarSpiral;` member
  - Remove from `transformOrder[]` array

- `src/render/post_effect.h`:
  - Remove all `logPolarSpiral*` shader/uniform/state members
  - Add `int infiniteZoomSpiralTwistLoc;`

- `src/render/post_effect.cpp`:
  - Remove `logPolarSpiralShader` load/unload
  - Remove `logPolarSpiral*` uniform location fetches
  - Add `GetShaderLocation()` for `spiralTwist`

- `src/render/render_pipeline.cpp`:
  - Remove `TRANSFORM_LOG_POLAR_SPIRAL` case from `GetTransformEffect()`
  - Remove `SetupLogPolarSpiral()` function
  - Add `SetShaderValue()` for `spiralTwist` in `SetupInfiniteZoom()`

- `src/ui/imgui_effects.cpp`:
  - Remove log-polar spiral UI section entirely
  - Add `ModulatableSliderAngleDeg` for `spiralTwist` in Infinite Zoom section

- `src/config/preset.cpp`:
  - Remove `LogPolarSpiralConfig` macro and serialization
  - Add `spiralTwist` to `InfiniteZoomConfig` serialization

**Done when**: Infinite Zoom has both `spiralTurns` (uniform) and `spiralTwist` (radius-dependent), log-polar spiral code deleted.

---

## Verification

After completing all phases:

1. **Visual**: Enable effect - creates spiraling tunnel/vortex into texture
2. **Spiral Twist**: Increase spiralTwist - spiral tightens with radius
3. **Spiral Turns**: Increase spiralTurns - layers rotate as they zoom
4. **Modulation**: Route bass→zoomDepth, mids→spiralTwist, treble→spiralTurns - parameters react to audio
5. **Chain Order**: Reorder in Effect Order list - effect executes in new position
6. **Presets**: Save preset, restart, load - settings preserved
7. **Performance**: 6 layers at 1080p stays under 4ms overhead
