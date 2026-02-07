# Muons

Raymarched volumetric generator producing glowing particle trails curving through 3D space. Thin luminous filaments spiral and fork like charged particles in a bubble chamber. Trails appear at varying depths with parallax, colored via a gradient LUT. Composited onto the scene via BlendCompositor.

**Research**: `docs/research/muons.md`

## Design

### Types

```
MuonsConfig {
  bool enabled = false;

  // Animation
  float speed = 0.3f;            // Turbulence evolution rate (0.0-2.0)

  // Raymarching
  int marchSteps = 10;           // Trail density — more steps reveal more filaments (4-40)
  int turbulenceOctaves = 9;     // Path complexity — fewer = smooth, more = chaotic (1-12)
  float turbulenceStrength = 1.0f; // FBM displacement amplitude (0.0-2.0)
  float ringThickness = 0.03f;   // Wire gauge of trails (0.005-0.1)
  float cameraDistance = 9.0f;   // Depth into volume (3.0-20.0)

  // Color
  float colorFreq = 33.0f;      // Color cycles along ray depth (0.5-50.0)
  float colorSpeed = 0.5f;      // LUT scroll rate over time (0.0-2.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Tonemap
  float brightness = 1.0f;      // Intensity multiplier before tonemap (0.1-5.0)
  float exposure = 3000.0f;     // Tonemap divisor — lower = brighter (500-10000)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;  // (0.0-5.0)
}

MuonsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;                    // CPU-accumulated animation phase
  int resolutionLoc;
  int timeLoc;
  int marchStepsLoc;
  int turbulenceOctavesLoc;
  int turbulenceStrengthLoc;
  int ringThicknessLoc;
  int cameraDistanceLoc;
  int colorFreqLoc;
  int colorSpeedLoc;
  int brightnessLoc;
  int exposureLoc;
  int gradientLUTLoc;
}
```

### Algorithm

The shader raymarches from each pixel into a 3D volume. Reference: `docs/research/muons.md` sections "Ray Setup", "March Loop", "Color Accumulation", "Tonemapping".

**Ray setup**: Per-pixel ray direction from screen UV (aspect-corrected). Camera sits at `z = -cameraDistance` looking forward.

**March loop** (marchSteps iterations):
1. Compute sample point `p = z * rayDir`
2. Time-varying rotation axis `a = normalize(cos(vec3(7,1,0) + time - s))`
3. Rodrigues rotation: `a = a * dot(a, p) - cross(a, p)`
4. FBM turbulence: N octaves of `a += sin(a * d + time).yzx / d * turbulenceStrength`
5. Shell distance: `s = length(a)`, step distance `d = ringThickness * abs(sin(s))`
6. Adaptive step: `z += d`

**Color accumulation**: At each step, sample gradient LUT at `fract(z * colorFreq + time * colorSpeed)`, accumulate `color += sampleColor / d / s`.

**Tonemapping**: `tanh(accumulated * brightness / exposure)` — brightness scales the final accumulation before the soft rolloff.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| speed | float | 0.0–2.0 | 0.3 | Yes | Speed |
| marchSteps | int | 4–40 | 10 | No | March Steps |
| turbulenceOctaves | int | 1–12 | 9 | No | Octaves |
| turbulenceStrength | float | 0.0–2.0 | 1.0 | Yes | Turbulence |
| ringThickness | float | 0.005–0.1 | 0.03 | Yes | Ring Thickness |
| cameraDistance | float | 3.0–20.0 | 9.0 | Yes | Camera Distance |
| colorFreq | float | 0.5–50.0 | 33.0 | Yes | Color Freq |
| colorSpeed | float | 0.0–2.0 | 0.5 | Yes | Color Speed |
| brightness | float | 0.1–5.0 | 1.0 | Yes | Brightness |
| exposure | float | 500–10000 | 3000 | Yes | Exposure |
| gradient | ColorConfig | — | GRADIENT | No | (gradient editor) |
| blendMode | EffectBlendMode | — | SCREEN | No | Blend Mode |
| blendIntensity | float | 0.0–5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_MUONS_BLEND`
- Display name: `"Muons Blend"`
- Category: `"GEN"`, section 10
- Config field name: `muons` (on `EffectConfig`)
- Effect field name: `muons` (on `PostEffect`)
- Active flag: `muonsBlendActive` (on `PostEffect`)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create muons effect header

**Files**: `src/effects/muons.h` (create)
**Creates**: `MuonsConfig`, `MuonsEffect` types and function declarations

**Do**: Follow `plasma.h` as pattern. Define `MuonsConfig` and `MuonsEffect` structs per the Design section. Include `render/blend_mode.h` and `render/color_config.h`. Declare `MuonsEffectInit`, `MuonsEffectSetup`, `MuonsEffectUninit`, `MuonsConfigDefault`, `MuonsRegisterParams`. Init takes `(MuonsEffect*, const MuonsConfig*)`. Setup takes `(MuonsEffect*, const MuonsConfig*, float deltaTime)`.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

All tasks depend on Wave 1 complete. No file overlap between tasks.

#### Task 2.1: Create muons effect implementation

**Files**: `src/effects/muons.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `plasma.cpp` as pattern. Load shader `shaders/muons.fs`. Cache all uniform locations listed in `MuonsEffect`. Init gradient LUT via `ColorLUTInit`. In Setup: accumulate `time += speed * deltaTime`, call `ColorLUTUpdate`, bind all uniforms via `SetShaderValue`. Bind gradient LUT texture via `SetShaderValueTexture`. RegisterParams: register all modulatable float params per the parameter table. Add `CMakeLists.txt` entry in `EFFECTS_SOURCES` (alphabetical near other m-names).

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Create muons fragment shader

**Files**: `shaders/muons.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the raymarching algorithm from the Design section and `docs/research/muons.md`. Accept all uniforms matching the `MuonsEffect` loc fields. Output to `finalColor`. Use aspect-corrected UV. The march loop, turbulence FBM, shell distance, and color accumulation follow the research doc exactly. Apply `tanh(accumulated / exposure)` tonemapping. Sample gradient LUT at `fract(z * colorFreq + time * colorSpeed)`.

**Verify**: Shader file exists with correct uniform declarations.

#### Task 2.3: Register muons in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/muons.h"` (alphabetical among includes)
- Add `TRANSFORM_MUONS_BLEND,` before `TRANSFORM_EFFECT_COUNT` in the enum
- Add `"Muons Blend"` entry in `TRANSFORM_EFFECT_NAMES` at matching position
- Add `MuonsConfig muons;` member to `EffectConfig` with comment
- Add `case TRANSFORM_MUONS_BLEND: return e->muons.enabled && e->muons.blendIntensity > 0.0f;` in `IsTransformEnabled`

Note: The `TransformOrderConfig::order` array auto-initializes from enum indices, so no manual entry needed.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Integrate into PostEffect

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `post_effect.h`: Add `#include "effects/muons.h"`. Add `MuonsEffect muons;` member. Add `bool muonsBlendActive;` flag.
- `post_effect.cpp` Init: Add `MuonsEffectInit(&pe->muons, &pe->effects.muons)` call. On failure: log error, free, return NULL. Follow `SpectralArcsEffectInit` call as pattern for placement.
- `post_effect.cpp` Uninit: Add `MuonsEffectUninit(&pe->muons);`
- `post_effect.cpp` RegisterParams: Add `MuonsRegisterParams(&pe->effects.muons);`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Shader setup and dispatch

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `shader_setup_generators.h`: Declare `SetupMuons(PostEffect*)` and `SetupMuonsBlend(PostEffect*)`.
- `shader_setup_generators.cpp`: Add `#include "effects/muons.h"`. Implement `SetupMuons` — delegates to `MuonsEffectSetup(&pe->muons, &pe->effects.muons, pe->currentDeltaTime)`. Implement `SetupMuonsBlend` — calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.muons.blendIntensity, pe->effects.muons.blendMode)`. Add case in `GetGeneratorScratchPass` for `TRANSFORM_MUONS_BLEND` returning `{pe->muons.shader, SetupMuons}`.
- `shader_setup.cpp` `GetTransformEffect`: Add case `TRANSFORM_MUONS_BLEND` returning `{&pe->blendCompositor->shader, SetupMuonsBlend, &pe->muonsBlendActive}`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `type == TRANSFORM_MUONS_BLEND` to `IsGeneratorBlendEffect()`.
- Add `pe->muonsBlendActive = (pe->effects.muons.enabled && pe->effects.muons.blendIntensity > 0.0f);` in the generator blend active flags section.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.7: UI panel

**Files**: `src/ui/imgui_effects.cpp` (modify), `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `imgui_effects.cpp`: Add `case TRANSFORM_MUONS_BLEND:` to `GetTransformCategory` returning `{"GEN", 10}`.
- `imgui_effects_generators.cpp`: Add `#include "effects/muons.h"`. Add `static bool sectionMuons = false;`. Create `DrawGeneratorsMuons` helper following `DrawGeneratorsPlasma` as pattern. Layout:
  - Enabled checkbox (moves to end on enable)
  - Animation: Speed slider (modulatable)
  - Raymarching section: March Steps (int slider), Octaves (int slider), Turbulence (modulatable), Ring Thickness (modulatable), Camera Distance (modulatable)
  - Color section: Color Freq (modulatable), Color Speed (modulatable), gradient editor
  - Tonemap section: Brightness (modulatable), Exposure (modulatable)
  - Output section: Blend Intensity (modulatable), Blend Mode combo
- Add call in `DrawGeneratorsCategory` before `DrawGeneratorsSolidColor`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.8: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `MuonsConfig` listing all fields: `enabled, speed, marchSteps, turbulenceOctaves, turbulenceStrength, ringThickness, cameraDistance, colorFreq, colorSpeed, brightness, exposure, gradient, blendMode, blendIntensity`.
- Add `to_json` entry: `if (e.muons.enabled) { j["muons"] = e.muons; }`
- Add `from_json` entry: `e.muons = j.value("muons", e.muons);`

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline with "GEN" badge
- [ ] Enabling adds muons to the pipeline list
- [ ] UI controls modify the effect in real-time
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
- [ ] marchSteps and turbulenceOctaves at max (40/12) remain interactive on target GPU
