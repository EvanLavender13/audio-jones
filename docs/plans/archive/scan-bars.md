# Scan Bars

Variable-width colored bars that scroll and converge toward a focal point via `tan()` distortion, with independent color cycling through a ColorLUT. Supports Linear (vertical/angled), Spoke (radial fan), and Ring (concentric circle) layouts. A generator effect rendered to scratch texture and blended into the pipeline.

**Research**: `docs/research/scan-bars.md`

## Design

### Types

**ScanBarsConfig** (in `src/effects/scan_bars.h`):

```
enabled          bool             false
mode             int              0          // 0=Linear, 1=Spokes, 2=Rings
angle            float            0.0f       // Bar orientation in radians (linear mode)
barDensity       float            10.0f      // Number of bars across viewport
convergence      float            0.5f       // tan() bunching strength
convergenceFreq  float            5.0f       // Spatial frequency of convergence warping
convergenceOffset float           0.0f       // Focal point offset from center
sharpness        float            0.1f       // Bar edge hardness (smoothstep width)
scrollSpeed      float            0.2f       // Bar position scroll rate
colorSpeed       float            1.0f       // LUT index drift rate
chaosFreq        float            10.0f      // Frequency multiplier for color chaos math
chaosIntensity   float            1.0f       // How wildly adjacent bars jump across palette
snapAmount       float            0.0f       // Time quantization (0=smooth, higher=stutter)
gradient         ColorConfig      {COLOR_MODE_GRADIENT}  // Palette via LUT
blendMode        EffectBlendMode  EFFECT_BLEND_SCREEN
blendIntensity   float            1.0f
```

**ScanBarsEffect** (in `src/effects/scan_bars.h`):

```
shader           Shader
gradientLUT      ColorLUT*
scrollPhase      float            // Bar position accumulator
colorPhase       float            // LUT index drift accumulator
resolutionLoc    int
modeLoc          int
angleLoc         int
barDensityLoc    int
convergenceLoc   int
convergenceFreqLoc int
convergenceOffsetLoc int
sharpnessLoc     int
scrollPhaseLoc   int
colorPhaseLoc    int
chaosFreqLoc     int
chaosIntensityLoc int
snapAmountLoc    int
gradientLUTLoc   int
```

### Algorithm

Reference `docs/research/scan-bars.md` for all formulas.

**Shader** (`shaders/scan_bars.fs`):

Three coordinate modes branch on a `mode` uniform:
- **Linear (0)**: Rotate UV by `angle`, then apply `tan(abs(x - offset) * convergenceFreq) * convergence` distortion to the rotated x-coordinate.
- **Spokes (1)**: Replace linear coordinate with `atan(uv.y, uv.x) / TAU + 0.5` (angular position). Convergence distortion bunches spokes toward an angular focal point.
- **Rings (2)**: Replace linear coordinate with `length(uv)` (radial distance). Convergence bunches rings toward a specific radius.

Bar mask from the distorted coordinate:
```glsl
d = fract(barDensity * coord + scrollPhase)
mask = smoothstep(0.5 - sharpness, 0.5, d) * smoothstep(0.5 + sharpness, 0.5, d)
```

Color via LUT:
```glsl
chaos = tan(coord * chaosFreq + colorPhase)
t = fract(chaos * chaosIntensity)
color = texture(gradientLUT, vec2(t, 0.5))
```

Snap/lurch via quantized phase (applied to both scrollPhase and colorPhase on CPU before sending to shader):
```
snapPhase = floor(phase) + pow(fract(phase), snapAmount + 1.0)
```
When `snapAmount = 0`, `pow(x, 1.0) = x` so scrolling is smooth. Higher values create lurching.

**Safety**: Clamp the `tan()` result to prevent NaN/Inf. Spoke mode wraps `atan()` discontinuity at +/-PI. Ring mode clamps radius to avoid degenerate bunching at origin.

**CPU side** (`src/effects/scan_bars.cpp`):

`ScanBarsEffectSetup` accumulates two independent phase accumulators:
- `scrollPhase += scrollSpeed * deltaTime`
- `colorPhase += colorSpeed * deltaTime`

Applies snap quantization to both phases before sending to shader. Updates ColorLUT, binds all uniforms.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| mode | int | 0-2 | 0 | No | Mode (combo: Linear/Spokes/Rings) |
| angle | float | 0 - 2PI | 0 | Yes | Angle (AngleDeg slider) |
| barDensity | float | 1 - 100 | 10 | Yes | Bar Density |
| convergence | float | 0 - 2 | 0.5 | Yes | Convergence |
| convergenceFreq | float | 1 - 20 | 5 | Yes | Conv. Frequency |
| convergenceOffset | float | -1 - 1 | 0 | Yes | Conv. Offset |
| sharpness | float | 0.01 - 0.5 | 0.1 | Yes | Sharpness |
| scrollSpeed | float | 0 - 5 | 0.2 | Yes | Scroll Speed |
| colorSpeed | float | 0 - 5 | 1.0 | Yes | Color Speed |
| chaosFreq | float | 1 - 50 | 10 | Yes | Chaos Frequency |
| chaosIntensity | float | 0 - 5 | 1.0 | Yes | Chaos Intensity |
| snapAmount | float | 0 - 2 | 0 | Yes | Snap Amount |
| gradient | ColorConfig | - | Rainbow | No | (color mode widget) |
| blendMode | EffectBlendMode | - | Screen | No | Blend Mode (combo) |
| blendIntensity | float | 0 - 5 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_SCAN_BARS_BLEND` (placed before `TRANSFORM_EFFECT_COUNT`, after `TRANSFORM_SOLID_COLOR`)
- Display name: `"Scan Bars Blend"`
- Category: `TRANSFORM_CATEGORY_GENERATORS` → `{"GEN", 10}` in `GetTransformCategory()`
- Config member name: `scanBars`
- Effect member name: `scanBars`
- Blend active flag: `scanBarsBlendActive`

---

## Tasks

### Wave 1: Config & Effect Module

#### Task 1.1: Create scan_bars.h and scan_bars.cpp

**Files**: `src/effects/scan_bars.h` (create), `src/effects/scan_bars.cpp` (create)
**Creates**: ScanBarsConfig struct, ScanBarsEffect struct, lifecycle functions

**Do**:
- Follow `src/effects/plasma.h` / `src/effects/plasma.cpp` as the pattern
- Config struct with all fields from the Design section
- Effect struct with Shader, ColorLUT*, two phase accumulators, and uniform location ints
- `ScanBarsEffectInit`: load `shaders/scan_bars.fs`, get all uniform locations, init ColorLUT, zero phases
- `ScanBarsEffectSetup`: accumulate phases (`scrollPhase += scrollSpeed * dt`, `colorPhase += colorSpeed * dt`), apply snap quantization, call `ColorLUTUpdate`, bind all uniforms via `SetShaderValue`
- `ScanBarsEffectUninit`: unload shader, free LUT
- `ScanBarsConfigDefault`: return default-constructed config
- `ScanBarsRegisterParams`: register all modulatable params (angle, barDensity, convergence, convergenceFreq, convergenceOffset, sharpness, scrollSpeed, colorSpeed, chaosFreq, chaosIntensity, snapAmount, blendIntensity) via `ModEngineRegisterParam`
- For the angle param, use `ROTATION_OFFSET_MAX` from `ui_units.h` as the max bound

**Verify**: `cmake.exe --build build` compiles (will fail until Wave 2 integrates, but the files should be syntactically valid).

---

### Wave 2: Shader, Integration, UI (parallel tasks — no file overlap)

#### Task 2.1: Create scan_bars.fs shader

**Files**: `shaders/scan_bars.fs` (create)
**Depends on**: Wave 1 complete (needs to know uniform names)

**Do**:
- Reference `docs/research/scan-bars.md` for all formulas
- Uniforms: `resolution`, `mode`, `angle`, `barDensity`, `convergence`, `convergenceFreq`, `convergenceOffset`, `sharpness`, `scrollPhase`, `colorPhase`, `chaosFreq`, `chaosIntensity`, `snapAmount`, `gradientLUT`
- Implement three coordinate modes branching on `mode` uniform (Linear/Spoke/Ring)
- Linear mode: rotate UV by angle, apply tan() convergence distortion
- Spoke mode: use atan(y,x)/TAU+0.5, apply convergence
- Ring mode: use length(uv), apply convergence
- Bar mask via fract + dual smoothstep
- Color via tan(coord * chaosFreq + colorPhase) → fract → LUT sample
- Clamp tan() outputs to safe range (e.g. -10 to 10) to prevent NaN
- Handle atan() discontinuity in spoke mode (fract wrapping)
- Output `finalColor = mask * color`

**Verify**: File exists, has valid GLSL syntax (verified at build time).

---

#### Task 2.2: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/scan_bars.h"` in the alphabetical include list (after `#include "effects/relativistic_doppler.h"`)
- Add `TRANSFORM_SCAN_BARS_BLEND` enum entry before `TRANSFORM_EFFECT_COUNT` (after `TRANSFORM_SOLID_COLOR`)
- Add `"Scan Bars Blend"` to `TRANSFORM_EFFECT_NAMES` array at matching index
- Add `ScanBarsConfig scanBars;` member to `EffectConfig` struct with comment
- Add `IsTransformEnabled` case: `case TRANSFORM_SCAN_BARS_BLEND: return e->scanBars.enabled && e->scanBars.blendIntensity > 0.0f;` (same pattern as TRANSFORM_PLASMA_BLEND)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `post_effect.h`: Add `#include "effects/scan_bars.h"` in alphabetical position. Add `ScanBarsEffect scanBars;` member after other generator effects. Add `bool scanBarsBlendActive;` in the generator blend active flags section.
- `post_effect.cpp`: Add `ScanBarsEffectInit(&pe->scanBars, &pe->effects.scanBars)` in `PostEffectInit()` after other generator inits (with error handling pattern). Add `ScanBarsEffectUninit(&pe->scanBars)` in `PostEffectUninit()`. Add `ScanBarsRegisterParams(&pe->effects.scanBars)` in `PostEffectRegisterParams()` after other generator registrations.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Shader setup (generators category)

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `shader_setup_generators.h`: Declare `void SetupScanBars(PostEffect *pe);` and `void SetupScanBarsBlend(PostEffect *pe);`
- `shader_setup_generators.cpp`: Implement `SetupScanBars` — delegates to `ScanBarsEffectSetup(&pe->scanBars, &pe->effects.scanBars, pe->currentDeltaTime)`. Implement `SetupScanBarsBlend` — calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.scanBars.blendIntensity, pe->effects.scanBars.blendMode)`. Add `TRANSFORM_SCAN_BARS_BLEND` case to `GetGeneratorScratchPass()` returning `{pe->scanBars.shader, SetupScanBars}`.
- `shader_setup.cpp`: Add `TRANSFORM_SCAN_BARS_BLEND` case in `GetTransformEffect()` returning `{&pe->blendCompositor->shader, SetupScanBarsBlend, &pe->scanBarsBlendActive}` (same pattern as `TRANSFORM_PLASMA_BLEND`).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `TRANSFORM_SCAN_BARS_BLEND` to the `IsGeneratorBlendEffect()` function's return expression (after `TRANSFORM_SOLID_COLOR`).
- Add `pe->scanBarsBlendActive = (pe->effects.scanBars.enabled && pe->effects.scanBars.blendIntensity > 0.0f);` in the "Generator blend active flags" section of `RenderPipelineApplyOutput()`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/scan_bars.h"` to includes
- Add `static bool sectionScanBars = false;` with other section bools
- Add `DrawGeneratorsScanBars()` helper function following `DrawGeneratorsPlasma` pattern:
  - `DrawSectionBegin("Scan Bars", categoryGlow, &sectionScanBars)`
  - Checkbox "Enabled##scanbars" → `MoveTransformToEnd(..., TRANSFORM_SCAN_BARS_BLEND)` on enable
  - `ImGui::Combo("Mode##scanbars", &cfg->mode, "Linear\0Spokes\0Rings\0")`
  - `ModulatableSliderAngleDeg("Angle##scanbars", ...)` for angle (only visible when mode==0)
  - `ModulatableSlider` for all float params (barDensity, convergence, convergenceFreq, convergenceOffset, sharpness, scrollSpeed, colorSpeed, chaosFreq, chaosIntensity, snapAmount)
  - `ImGuiDrawColorMode(&cfg->gradient)` for gradient
  - Output section: blend intensity slider + blend mode combo
- Add `DrawGeneratorsScanBars()` call in `DrawGeneratorsCategory()` with spacing

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.7: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ScanBarsConfig, enabled, mode, angle, barDensity, convergence, convergenceFreq, convergenceOffset, sharpness, scrollSpeed, colorSpeed, chaosFreq, chaosIntensity, snapAmount, gradient, blendMode, blendIntensity)` near other generator config macros
- Add `if (e.scanBars.enabled) { j["scanBars"] = e.scanBars; }` in `to_json`
- Add `e.scanBars = j.value("scanBars", e.scanBars);` in `from_json`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.8: Build system and category mapping

**Files**: `CMakeLists.txt` (modify), `src/ui/imgui_effects.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `CMakeLists.txt`: Add `src/effects/scan_bars.cpp` to `EFFECTS_SOURCES` list (alphabetical position)
- `src/ui/imgui_effects.cpp`: Add `case TRANSFORM_SCAN_BARS_BLEND:` to `GetTransformCategory()` in the generators section (with the other `TRANSFORM_*_BLEND` cases) returning `{"GEN", 10}`

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Generators category in UI
- [ ] Effect shows "GEN" badge in pipeline list
- [ ] All three modes (Linear, Spokes, Rings) produce distinct visuals
- [ ] Scroll and color phases animate independently
- [ ] Convergence distortion bunches bars toward focal point
- [ ] Snap amount creates lurching motion at non-zero values
- [ ] Blend mode and intensity control compositing
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
