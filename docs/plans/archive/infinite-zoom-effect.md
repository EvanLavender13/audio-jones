# Infinite Zoom Post-Effect

Multi-layer cross-fade zoom that eliminates hard boundaries. Layers at exponentially-spaced scales fade via cosine weighting, producing seamless infinite zoom into any texture.

## Current State

Existing post-effect infrastructure to hook into:

- `src/config/kaleidoscope_config.h` - pattern for dedicated config headers
- `src/config/effect_config.h:20-40` - aggregates all effect configs
- `src/render/post_effect.h:12-88` - shader handles and uniform locations
- `src/render/post_effect.cpp:24-44` - shader loading pattern
- `src/render/render_pipeline.cpp:286-290` - kaleidoscope pass insertion point
- `src/ui/imgui_effects.cpp:36-64` - collapsible section pattern
- `src/config/preset.cpp:86-93` - JSON serialization macros
- `docs/research/infinite-zoom-crossfade.md:120-179` - algorithm and GLSL reference

---

## Phase 1: Config and Shader

**Goal**: Create config struct and core shader.

**Create**:
- `src/config/infinite_zoom_config.h` - config struct with defaults:
  - `enabled` (bool, default false)
  - `speed` (float, 0.1-2.0, default 1.0)
  - `baseScale` (float, 0.5-2.0, default 1.0)
  - `focalAmplitude` (float, 0.0-0.2, default 0.0) - Lissajous orbit radius in UV units
  - `focalFreqX` (float, 0.1-5.0, default 1.0) - Lissajous X frequency
  - `focalFreqY` (float, 0.1-5.0, default 1.5) - Lissajous Y frequency
  - `layers` (int, 4-8, default 6)
  - `spiralTurns` (float, 0.0-4.0, default 0.0)

- `shaders/infinite_zoom.fs` - fragment shader implementing:
  - Multi-layer loop with exponential scale (`exp2(phase * L) * baseScale`)
  - Cosine alpha weighting (`(1.0 - cos(phase * TWO_PI)) * 0.5`)
  - Optional spiral rotation per layer when `spiralTurns > 0`
  - Edge softening via `smoothstep(0.0, 0.1, edgeDist)`
  - Early-out on `alpha < 0.001`
  - Weighted accumulation with normalization

**Modify**:
- `src/config/effect_config.h` - add `#include` and `InfiniteZoomConfig infiniteZoom;` member

**Done when**: Config struct compiles and shader file exists.

---

## Phase 2: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Modify**:
- `src/render/post_effect.h`:
  - Add `Shader infiniteZoomShader;` after line 26
  - Add uniform locations after line 72: `infiniteZoomTimeLoc`, `infiniteZoomSpeedLoc`, `infiniteZoomBaseScaleLoc`, `infiniteZoomFocalLoc`, `infiniteZoomLayersLoc`, `infiniteZoomSpiralTurnsLoc`
  - Add `float infiniteZoomTime;` and `float infiniteZoomFocal[2];` in temporaries section after line 87

- `src/render/post_effect.cpp`:
  - Add `LoadShader(0, "shaders/infinite_zoom.fs")` in `LoadPostEffectShaders()` at line 36
  - Add return check for `infiniteZoomShader.id != 0` at line 38-43
  - Add `GetShaderLocation()` calls in `GetShaderUniformLocations()` at line 93
  - Add `UnloadShader(pe->infiniteZoomShader)` in `PostEffectUninit()` at line 184

**Done when**: Build succeeds with shader loaded and uniform locations cached.

---

## Phase 3: Pipeline Integration

**Goal**: Execute shader pass in render pipeline.

**Modify**:
- `src/render/render_pipeline.cpp`:
  - Add time accumulation in `RenderPipelineApplyFeedback()` after line 230:
    `pe->infiniteZoomTime += deltaTime;`
  - Add Lissajous focal offset computation in `RenderPipelineApplyOutput()`:
    `pe->infiniteZoomFocal[0] = iz->focalAmplitude * sinf(t * iz->focalFreqX);`
    `pe->infiniteZoomFocal[1] = iz->focalAmplitude * cosf(t * iz->focalFreqY);`
  - Add `SetupInfiniteZoom()` callback after `SetupKaleido()` at line 141:
    - Set all uniforms from `pe->effects.infiniteZoom` config
    - Use `pe->infiniteZoomTime` for time uniform
    - Use `pe->infiniteZoomFocal` for focalOffset uniform
  - Insert conditional `RenderPass()` after kaleidoscope (line 290), before `BlitTexture` (line 293):
    ```
    if (pe->effects.infiniteZoom.enabled) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->infiniteZoomShader, SetupInfiniteZoom);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }
    ```

**Done when**: Effect renders when `enabled = true` in config.

---

## Phase 4: UI Controls

**Goal**: Add user-facing controls in Effects panel.

**Modify**:
- `src/ui/imgui_effects.cpp`:
  - Add `static bool sectionInfiniteZoom = false;` at line 15
  - Add collapsible section after Kaleidoscope section (after line 64):
    - `DrawSectionBegin("Infinite Zoom", Theme::GLOW_MAGENTA, &sectionInfiniteZoom)`
    - Checkbox for `enabled`
    - SliderFloat for `speed` (0.1-2.0, format "%.2f")
    - SliderFloat for `baseScale` (0.5-2.0, format "%.2f")
    - SliderFloat for `focalAmplitude` (0.0-0.2, format "%.3f")
    - SliderFloat for `focalFreqX` (0.1-5.0, format "%.2f")
    - SliderFloat for `focalFreqY` (0.1-5.0, format "%.2f")
    - SliderInt for `layers` (4-8)
    - SliderFloat for `spiralTurns` (0.0-4.0, format "%.2f turns")
    - `DrawSectionEnd()`

**Done when**: Effect controllable via UI.

---

## Phase 5: Preset Serialization

**Goal**: Save/load effect settings in presets.

**Modify**:
- `src/config/preset.cpp`:
  - Add `#include "config/infinite_zoom_config.h"` at line 6
  - Add serialization macro after line 89:
    `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InfiniteZoomConfig, enabled, speed, baseScale, centerX, centerY, layers, spiralTurns)`
  - Add `infiniteZoom` to `EffectConfig` macro at line 91-93

**Done when**: Presets persist infinite zoom settings across save/load.

---

## Verification

After completing all phases:

1. **Visual**: Enable effect with kaleidoscope - mandala zooms infinitely without hard edges
2. **Spiral**: Set `spiralTurns > 0` - layers rotate as they zoom
3. **Layers**: Reduce to 4 - still smooth; increase to 8 - even smoother
4. **Presets**: Save preset, restart, load - settings preserved
5. **Performance**: 6 layers at 1080p stays under 4ms overhead
