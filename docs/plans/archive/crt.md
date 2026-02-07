# CRT

Standalone CRT monitor simulation: phosphor mask (shadow mask or aperture grille), scanlines, barrel distortion, vignette, and pulsing electron beam glow. Extracts CRT-related fields from Glitch into a dedicated retro transform effect with full modulation support.

**Research**: `docs/research/crt.md`

## Design

### Types

```
CrtConfig {
  bool enabled = false;

  // Phosphor mask
  int maskMode = 0;             // 0 = shadow mask, 1 = aperture grille
  float maskSize = 8.0f;        // Cell pixel size (2.0-24.0)
  float maskIntensity = 0.7f;   // Blend strength (0.0-1.0)
  float maskBorder = 0.8f;      // Dark gap width (0.0-1.0)

  // Scanlines
  float scanlineIntensity = 0.3f;    // Darkness (0.0-1.0)
  float scanlineSpacing = 2.0f;      // Pixels between lines (1.0-8.0)
  float scanlineSharpness = 1.5f;    // Transition sharpness (0.5-4.0)
  float scanlineBrightBoost = 0.5f;  // Bright pixel resistance (0.0-1.0)

  // Barrel distortion
  bool curvatureEnabled = true;
  float curvatureAmount = 0.06f;     // Distortion strength (0.0-0.15)

  // Vignette
  bool vignetteEnabled = true;
  float vignetteExponent = 0.4f;     // Edge falloff curve (0.1-1.0)

  // Pulsing glow
  bool pulseEnabled = false;
  float pulseIntensity = 0.03f;      // Brightness ripple (0.0-0.1)
  float pulseWidth = 60.0f;          // Wavelength in pixels (20.0-200.0)
  float pulseSpeed = 20.0f;          // Scroll speed (1.0-40.0)
}

CrtEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int maskModeLoc;
  int maskSizeLoc;
  int maskIntensityLoc;
  int maskBorderLoc;
  int scanlineIntensityLoc;
  int scanlineSpacingLoc;
  int scanlineSharpnessLoc;
  int scanlineBrightBoostLoc;
  int curvatureEnabledLoc;
  int curvatureAmountLoc;
  int vignetteEnabledLoc;
  int vignetteExponentLoc;
  int pulseEnabledLoc;
  int pulseIntensityLoc;
  int pulseWidthLoc;
  int pulseSpeedLoc;
  float time;  // Animation accumulator for pulse
}
```

### Algorithm

The shader implements seven stages in fixed order. Reference `docs/research/crt.md` for full GLSL.

1. **Barrel distortion** — warp UVs via squared radial distance. Store whether UV is out-of-bounds for bezel clamp later.
2. **Sample texture** at warped UV. In shadow mask mode, quantize UV to mask cell centers (one sample per cell, reducing effective resolution). Aperture grille samples per-pixel.
3. **Scanlines** — sin-based horizontal darkening modulated by image brightness (bright pixels resist darkening).
4. **Phosphor mask** — shadow mask mode (staggered rectangular R/G/B subcells) or aperture grille mode (continuous vertical stripes). Both use `*3.0` brightness compensation.
5. **Pulsing glow** — vertical brightness bands (constant along y) scrolling horizontally via `cos(pixel.x / pulseWidth + time * pulseSpeed)`.
6. **Vignette** — separable x/y edge darkening.
7. **Bezel clamp** — zero out pixels where warped UV fell outside [0,1] after barrel distortion.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| maskMode | int | 0–1 | 0 | No | Mask Mode |
| maskSize | float | 2.0–24.0 | 8.0 | Yes | Mask Size |
| maskIntensity | float | 0.0–1.0 | 0.7 | Yes | Mask Intensity |
| maskBorder | float | 0.0–1.0 | 0.8 | No | Mask Border |
| scanlineIntensity | float | 0.0–1.0 | 0.3 | Yes | Scanline Intensity |
| scanlineSpacing | float | 1.0–8.0 | 2.0 | No | Scanline Spacing |
| scanlineSharpness | float | 0.5–4.0 | 1.5 | No | Scanline Sharpness |
| scanlineBrightBoost | float | 0.0–1.0 | 0.5 | No | Bright Boost |
| curvatureEnabled | bool | — | true | No | Curvature |
| curvatureAmount | float | 0.0–0.15 | 0.06 | Yes | Curvature Amount |
| vignetteEnabled | bool | — | true | No | Vignette |
| vignetteExponent | float | 0.1–1.0 | 0.4 | No | Vignette Exponent |
| pulseEnabled | bool | — | false | No | Pulse |
| pulseIntensity | float | 0.0–0.1 | 0.03 | Yes | Pulse Intensity |
| pulseWidth | float | 20.0–200.0 | 60.0 | No | Pulse Width |
| pulseSpeed | float | 1.0–40.0 | 20.0 | Yes | Pulse Speed |

### Constants

- Enum name: `TRANSFORM_CRT`
- Display name: `"CRT"`
- Category: `{"RET", 6}` (Retro)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create CRT effect header

**Files**: `src/effects/crt.h` (create)
**Creates**: `CrtConfig` and `CrtEffect` types that all other tasks depend on

**Do**: Define `CrtConfig` and `CrtEffect` structs per the Design section. Expose `CrtEffectInit`, `CrtEffectSetup`, `CrtEffectUninit`, `CrtConfigDefault`, `CrtRegisterParams`. Follow `glitch.h` structure — include guard `CRT_H`, include `raylib.h` and `<stdbool.h>`.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

All tasks in this wave touch different files — no overlap.

#### Task 2.1: Effect module implementation

**Files**: `src/effects/crt.cpp` (create)
**Depends on**: Wave 1

**Do**: Implement `CrtEffectInit` (load `shaders/crt.fs`, cache all uniform locations), `CrtEffectSetup` (accumulate time, bind all uniforms — follow `GlitchEffectSetup` pattern with static helpers for each sub-feature group), `CrtEffectUninit`, `CrtConfigDefault`, `CrtRegisterParams` (register 6 modulatable params: `crt.maskSize`, `crt.maskIntensity`, `crt.scanlineIntensity`, `crt.curvatureAmount`, `crt.pulseIntensity`, `crt.pulseSpeed`). Follow `glitch.cpp` conventions: `#include "crt.h"`, `#include "automation/modulation_engine.h"`, `#include <stddef.h>`.

**Verify**: Compiles.

#### Task 2.2: Fragment shader

**Files**: `shaders/crt.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the 6-stage shader per the Algorithm section and `docs/research/crt.md` GLSL snippets. Uniforms: `resolution` (vec2), `time` (float), plus all 16 config params. Use `#version 330`, `in vec2 fragTexCoord`, `out vec4 finalColor`, `uniform sampler2D texture0`. Apply stages in order: barrel distortion → sample → scanlines → phosphor mask → pulse → vignette → bezel clamp.

**Verify**: File exists and is valid GLSL (verified by build + runtime).

#### Task 2.3: Effect registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/crt.h"` (alphabetical, after `#include "effects/cross_hatching.h"`)
2. Add `TRANSFORM_CRT,` before `TRANSFORM_EFFECT_COUNT`
3. Add `"CRT"` to `TRANSFORM_EFFECT_NAMES` array at matching index
4. Add `CrtConfig crt;` to `EffectConfig` struct (after `GlitchConfig glitch` comment block — CRT is a retro effect)
5. Add `case TRANSFORM_CRT: return e->crt.enabled;` to `IsTransformEnabled`

**Verify**: Compiles. `TRANSFORM_EFFECT_COUNT` incremented by 1.

#### Task 2.4: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1

**Do**:
- `post_effect.h`: Add `#include "effects/crt.h"` and `CrtEffect crt;` member (near `GlitchEffect glitch`)
- `post_effect.cpp`: Add `CrtEffectInit(&pe->crt)` in `PostEffectInit` (with failure logging), `CrtEffectUninit(&pe->crt)` in `PostEffectUninit`, `CrtRegisterParams(&pe->effects.crt)` in `PostEffectRegisterParams`

**Verify**: Compiles.

#### Task 2.5: Shader setup (retro category)

**Files**: `src/render/shader_setup_retro.h` (modify), `src/render/shader_setup_retro.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1

**Do**:
- `shader_setup_retro.h`: Add `void SetupCrt(PostEffect *pe);` declaration
- `shader_setup_retro.cpp`: Add `#include "effects/crt.h"`, implement `SetupCrt` that delegates to `CrtEffectSetup(&pe->crt, &pe->effects.crt, pe->currentDeltaTime)`
- `shader_setup.cpp`: Add `case TRANSFORM_CRT: return {&pe->crt.shader, SetupCrt, &pe->effects.crt.enabled};` in `GetTransformEffect`

Note: The static helper in `glitch.cpp` is also named `SetupCrt` — the new public function in `shader_setup_retro.cpp` won't conflict because the glitch one is `static` (file-local).

**Verify**: Compiles.

#### Task 2.6: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/crt.cpp` to `EFFECTS_SOURCES` list (alphabetical, after `cross_hatching.cpp`).

**Verify**: Compiles.

#### Task 2.7: UI panel

**Files**: `src/ui/imgui_effects_retro.cpp` (modify), `src/ui/imgui_effects.cpp` (modify)
**Depends on**: Wave 1

**Do**:
- `imgui_effects_retro.cpp`:
  1. Add `static bool sectionCrt = false;` with other section bools
  2. Add `DrawRetroCrt()` helper before `DrawRetroCategory()`. Structure: `DrawSectionBegin("CRT", ...)`, checkbox for enabled (with `MoveTransformToEnd`), then group controls under tree nodes — "Phosphor Mask" (combo for maskMode with items "Shadow Mask"/"Aperture Grille", `ModulatableSlider` for maskSize/maskIntensity, slider for maskBorder), "Scanlines" (ModulatableSlider for scanlineIntensity, sliders for spacing/sharpness/brightBoost), "Curvature" (checkbox curvatureEnabled, ModulatableSlider curvatureAmount), "Vignette" (checkbox vignetteEnabled, slider vignetteExponent), "Pulse" (checkbox pulseEnabled, ModulatableSlider pulseIntensity/pulseSpeed, slider pulseWidth). Use `##crt` suffixes.
  3. Call `DrawRetroCrt()` in `DrawRetroCategory()` with `ImGui::Spacing()` — place after Glitch, before ASCII Art
- `imgui_effects.cpp`: Add `case TRANSFORM_CRT:` to `GetTransformCategory` returning `{"RET", 6}`

**Verify**: Compiles.

#### Task 2.8: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CrtConfig, enabled, maskMode, maskSize, maskIntensity, maskBorder, scanlineIntensity, scanlineSpacing, scanlineSharpness, scanlineBrightBoost, curvatureEnabled, curvatureAmount, vignetteEnabled, vignetteExponent, pulseEnabled, pulseIntensity, pulseWidth, pulseSpeed)` — place alphabetically near other macros
2. Add `if (e.crt.enabled) { j["crt"] = e.crt; }` in `to_json`
3. Add `e.crt = j.value("crt", e.crt);` in `from_json`

**Verify**: Compiles.

---

### Wave 3: Glitch Extraction

#### Task 3.1: Remove CRT from Glitch

**Files**: `src/effects/glitch.h` (modify), `src/effects/glitch.cpp` (modify), `shaders/glitch.fs` (modify), `src/ui/imgui_effects_retro.cpp` (modify), `src/config/preset.cpp` (modify)
**Depends on**: Wave 2

**Do**:
- `glitch.h`: Remove `crtEnabled`, `curvature`, `vignetteEnabled` fields from `GlitchConfig`. Remove `crtEnabledLoc`, `curvatureLoc`, `vignetteEnabledLoc` from `GlitchEffect`.
- `glitch.cpp`: Remove `CacheLocations` entries for crt/curvature/vignette. Remove the `static SetupCrt` helper. Remove the `SetupCrt(e, cfg)` call from `GlitchEffectSetup`.
- `glitch.fs`: Remove `crtEnabled`, `curvature`, `vignetteEnabled` uniforms. Remove the `crt()` function. Remove the `if (crtEnabled)` barrel distortion block. Remove the `if (crtEnabled && vignetteEnabled)` vignette block.
- `imgui_effects_retro.cpp`: Remove the `TreeNodeAccented("CRT##glitch", ...)` block from `DrawRetroGlitch`.
- `preset.cpp`: Remove `crtEnabled`, `curvature`, `vignetteEnabled` from the `GlitchConfig` NLOHMANN macro.

**Verify**: Compiles. Glitch effect still works without CRT sub-features.

#### Task 3.2: Update presets manually

**Files**: `presets/BUZZBUZZ.json` (modify), `presets/CYMATICBOB.json` (modify), `presets/GLITCHYBOB.json` (modify)
**Depends on**: Wave 2

**Do**: For each preset that had `glitch.crtEnabled: true`:
1. Remove `crtEnabled`, `curvature`, `vignetteEnabled` from the `"glitch"` object
2. Add a new `"crt"` object at the top level of the effects with `enabled: true` and corresponding barrel/vignette values mapped from the old glitch fields:
   - `glitch.curvature` → `crt.curvatureAmount` (note: Glitch used different barrel math — use `curvatureAmount: 0.06` as default since the algorithms differ)
   - `glitch.vignetteEnabled` → `crt.vignetteEnabled`
   - All other CRT fields use defaults

**Verify**: JSON is valid. Loading each preset enables the CRT effect.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] CRT effect appears in transform order pipeline with "RET" badge
- [ ] Enabling CRT adds it to pipeline list and can be reordered
- [ ] Shadow mask and aperture grille modes produce distinct phosphor patterns
- [ ] Scanlines darken the image with brightness-dependent resistance
- [ ] Barrel distortion curves the image with black bezel outside bounds
- [ ] Vignette darkens edges
- [ ] Pulse produces vertical brightness bands scrolling horizontally when enabled
- [ ] All 6 modulatable params respond to LFO routing
- [ ] Preset save/load round-trips all 16 CRT parameters
- [ ] Glitch effect works without CRT sub-features
- [ ] BUZZBUZZ, CYMATICBOB, GLITCHYBOB presets load with CRT effect active
