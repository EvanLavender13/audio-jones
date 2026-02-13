# Phi Blur

Standalone blur transform with two phi-based sampling modes: a rectangular kernel with free angle and aspect ratio, and a circular disc kernel. Both distribute samples using golden-ratio low-discrepancy sequences for artifact-free coverage at any sample count. Gamma-correct blending linearizes before averaging to prevent midtone darkening. Fits in the Optical category alongside Bloom, Bokeh, and Heightfield Relief.

**Research**: `docs/research/phi_blur.md`

## Design

### Types

**PhiBlurConfig** (user-facing params):
```
bool enabled = false
int mode = 0              // 0 = Rect, 1 = Disc
float radius = 5.0f       // Blur extent in pixels (0.0-50.0)
float angle = 0.0f        // Rect kernel rotation in radians (0-2pi, displayed as degrees)
float aspectRatio = 1.0f  // Rect kernel width/height ratio (0.1-10.0)
int samples = 32          // Sample count (8-128)
float gamma = 2.2f        // Blending gamma (1.0-6.0)
```

**PhiBlurEffect** (runtime state):
```
Shader shader
int resolutionLoc
int modeLoc
int radiusLoc
int angleLoc
int aspectRatioLoc
int samplesLoc
int gammaLoc
```

No animation accumulator — this is a static effect like Bokeh.

### Algorithm

Single fragment shader with mode switch. Both modes share the gamma-correct accumulation loop.

**Rect mode** (mode == 0):
Distribute N samples across a rectangular area. X coordinate is `i / samples` (uniform spacing). Y coordinate is `fract(i * PHI)` (golden-ratio low-discrepancy). Both shifted to [-0.5, 0.5]. Then rotate by `angle` via 2x2 matrix and scale by `(radius * aspectRatio, radius)` in pixel space, divided by resolution for UV offset.

```glsl
#define PHI 1.618034

vec2 offset = vec2(i / samples, fract(i * PHI)) - 0.5;
// Rotate: mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * offset
// Scale: offset * vec2(radius * aspectRatio, radius) / resolution
```

**Disc mode** (mode == 1):
Golden-angle rotation with sqrt-radius for uniform area coverage (identical to existing Bokeh's sampling, but without brightness weighting):

```glsl
#define GOLDEN_ANGLE 2.399963
#define HALF_PI 1.570796

vec2 dir = cos(i * GOLDEN_ANGLE + vec2(0.0, HALF_PI));
float r = sqrt(i / samples) * radius;
vec2 offset = dir * r / resolution;
```

**Gamma-correct blending** (both modes):
Linearize each sample before accumulating, then re-gamma after dividing by count:

```glsl
blur += pow(textureSample, vec4(gamma));
// after loop:
result = pow(blur / samples, vec4(1.0 / gamma));
```

Default gamma = 2.2. At gamma = 1.0, reverts to plain linear average. Higher values (4-6) preserve bright regions.

**Edge handling**: Clamp UV to [0, 1] before texture fetch.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| mode | int | 0-1 | 0 | No | "Mode" (Combo: "Rect", "Disc") |
| radius | float | 0.0-50.0 | 5.0 | Yes | "Radius" |
| angle | float | 0-2pi | 0.0 | Yes | "Angle" (ModulatableSliderAngleDeg) |
| aspectRatio | float | 0.1-10.0 | 1.0 | Yes | "Aspect Ratio" |
| samples | int | 8-128 | 32 | No | "Samples" (SliderInt) |
| gamma | float | 1.0-6.0 | 2.2 | Yes | "Gamma" |

- `angle` and `aspectRatio` are hidden in UI when mode == 1 (Disc)
- `radius` is in pixel units, converted to UV space in shader by dividing by resolution

### Constants

- Enum: `TRANSFORM_PHI_BLUR`
- Display name: `"Phi Blur"`
- Category: `"OPT"`, section index 7
- Flags: `EFFECT_FLAG_NONE`
- Config member: `phiBlur`

---

## Tasks

### Wave 1: Config & Header

#### Task 1.1: Create effect module header and source

**Files**: `src/effects/phi_blur.h` (create), `src/effects/phi_blur.cpp` (create)

**Creates**: PhiBlurConfig struct, PhiBlurEffect struct, lifecycle function declarations

**Do**: Follow `src/effects/bokeh.h` / `src/effects/bokeh.cpp` as pattern. Header declares PhiBlurConfig (fields per Design section), PhiBlurEffect (shader + uniform locs), and functions: `PhiBlurEffectInit`, `PhiBlurEffectSetup`, `PhiBlurEffectUninit`, `PhiBlurConfigDefault`, `PhiBlurRegisterParams`. Source implements all five. Setup takes `(PhiBlurEffect* e, const PhiBlurConfig* cfg)` — no deltaTime since no animation. Bind all 7 uniforms (resolution, mode, radius, angle, aspectRatio, samples, gamma). RegisterParams registers radius (0-50), angle (0-ROTATION_OFFSET_MAX), aspectRatio (0.1-10), gamma (1-6).

**Verify**: Header compiles when included.

#### Task 1.2: Register enum, order, and config member

**Files**: `src/config/effect_config.h` (modify)

**Creates**: TRANSFORM_PHI_BLUR enum entry, order array entry, PhiBlurConfig member in EffectConfig

**Do**: Add `#include "effects/phi_blur.h"` with other effect includes. Add `TRANSFORM_PHI_BLUR` before `TRANSFORM_EFFECT_COUNT`. Add to `TransformOrderConfig::order` array at end. Add `PhiBlurConfig phiBlur;` to EffectConfig struct.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Shader, Integration, UI, Serialization

#### Task 2.1: Create fragment shader

**Files**: `shaders/phi_blur.fs` (create)

**Do**: Implement the Algorithm section above. `#version 330`. Uniforms: `texture0`, `resolution` (vec2), `mode` (int), `radius` (float), `angle` (float), `aspectRatio` (float), `samples` (int), `gamma` (float). Use `if (mode == 0)` branch for rect, else disc. Clamp UV to [0,1] before each texture fetch. Output to `finalColor`.

**Verify**: Shader file exists and has correct uniform declarations.

#### Task 2.2: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/phi_blur.h"` and `PhiBlurEffect phiBlur;` member in PostEffect struct. In `PostEffectInit()`, add init call after other optical effects. In `PostEffectUninit()`, add uninit call. In `PostEffectRegisterParams()`, add `PhiBlurRegisterParams(&pe->effects.phiBlur);`.

**Verify**: Compiles.

#### Task 2.3: Shader setup dispatch

**Files**: `src/render/shader_setup_optical.h` (modify), `src/render/shader_setup_optical.cpp` (modify), `src/render/shader_setup.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Declare `void SetupPhiBlur(PostEffect* pe);` in header. Implement in .cpp — delegates to `PhiBlurEffectSetup(&pe->phiBlur, &pe->effects.phiBlur)`. Add dispatch case in `shader_setup.cpp` `GetTransformEffect()`: `case TRANSFORM_PHI_BLUR: return { &pe->phiBlur.shader, SetupPhiBlur, &pe->effects.phiBlur.enabled };`.

**Verify**: Compiles.

#### Task 2.4: Descriptor table row

**Files**: `src/config/effect_descriptor.h` (modify)

**Depends on**: Wave 1 complete

**Do**: Add row to `EFFECT_DESCRIPTORS[]` before the closing `};`:
```
{
    TRANSFORM_PHI_BLUR, "Phi Blur", "OPT", 7,
    offsetof(EffectConfig, phiBlur.enabled), EFFECT_FLAG_NONE
},
```

**Verify**: Compiles.

#### Task 2.5: UI panel

**Files**: `src/ui/imgui_effects_optical.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/phi_blur.h"` if needed. Add `static bool sectionPhiBlur = false;`. Create `DrawOpticalPhiBlur()` following the Bokeh pattern. Mode selector: `ImGui::Combo("Mode##phiBlur", &e->phiBlur.mode, "Rect\0Disc\0")`. Show radius (ModulatableSlider, "%.1f"), samples (SliderInt, 8-128), gamma (ModulatableSlider, "%.1f"). When mode == 0 (Rect): also show angle (ModulatableSliderAngleDeg) and aspectRatio (ModulatableSlider, "%.1f"). Call from `DrawOpticalCategory()` with spacing after Anamorphic Streak.

**Verify**: Compiles.

#### Task 2.6: Preset serialization

**Files**: `src/config/preset.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhiBlurConfig, enabled, mode, radius, angle, aspectRatio, samples, gamma)`. Add `if (e.phiBlur.enabled) { j["phiBlur"] = e.phiBlur; }` in `to_json`. Add `e.phiBlur = j.value("phiBlur", e.phiBlur);` in `from_json`.

**Verify**: Compiles.

#### Task 2.7: Build system

**Files**: `CMakeLists.txt` (modify)

**Depends on**: Wave 1 complete

**Do**: Add `src/effects/phi_blur.cpp` to `EFFECTS_SOURCES`.

**Verify**: Full build succeeds: `cmake.exe --build build`.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform reorder list with "OPT" badge
- [ ] Combo switches between Rect and Disc mode
- [ ] Angle and Aspect Ratio sliders hidden in Disc mode
- [ ] Radius, angle, aspectRatio, gamma respond to LFO modulation
- [ ] Preset save/load preserves all settings
- [ ] Visual: Rect mode produces directional blur, Disc mode produces uniform circular blur
- [ ] Visual: Gamma > 2 preserves bright regions more than gamma = 1
