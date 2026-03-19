# Voxel March

Fly-through raymarcher rendering voxelized 3D space where rounded sphere shells repeat through domain-folded space, with sin-boundary voxel density variation, boundary seam highlights, and depth-dependent FFT audio reactivity. Based on "Refactoring" by OldEclipse.

**Research**: `docs/research/voxel_march.md`

## Design

### Types

**VoxelMarchConfig** (`src/effects/voxel_march.h`):
```
bool enabled = false

// Geometry
int marchSteps = 60          // Raymarching iterations (30-80)
float stepSize = 0.3f        // March step multiplier (0.1-0.5)
float voxelScale = 16.0f     // Base voxel grid density (4.0-32.0)
float voxelVariation = 1.0f  // Sin-boundary variation blend (0.0-1.0)
float cellSize = 4.0f        // Domain repetition period (1.0-8.0)
float shellRadius = 2.8f     // Sphere shell surface radius (0.5-4.0)
float surfaceCount = 2.0f    // Number of layered shell surfaces (1-3), float for modulation
float highlightIntensity = 0.06f // Boundary seam highlight strength (0.0-0.5)
float positionTint = 0.5f    // Position color influence (0.0-1.0)
float tonemapGain = 0.0005f  // Tonemap exposure (0.0001-0.002)

// Speed (CPU-accumulated)
float flySpeed = 1.0f        // Forward travel speed (0.0-5.0)
float gridAnimSpeed = 1.0f   // Sin-boundary animation rate (0.0-3.0)

// Camera
DualLissajousConfig lissajous = {.amplitude = 0.8f, .motionSpeed = 1.0f, .freqX1 = 0.6f, .freqY1 = 0.3f}

// Audio
float baseFreq = 55.0f       // Low frequency bound Hz (27.5-440)
float maxFreq = 14000.0f     // High frequency bound Hz (1000-16000)
float gain = 2.0f            // FFT amplitude multiplier (0.1-10)
float curve = 1.5f           // FFT response curve exponent (0.1-3.0)
float baseBright = 0.15f     // Minimum brightness when silent (0.0-1.0)

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}

// Blend
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**VoxelMarchEffect** (`src/effects/voxel_march.h`):
```
Shader shader
ColorLUT *gradientLUT

// CPU-accumulated phases
float flyPhase
float gridPhase

// Uniform locations
int resolutionLoc
int fftTextureLoc
int sampleRateLoc
int flyPhaseLoc
int gridPhaseLoc
int marchStepsLoc
int stepSizeLoc
int voxelScaleLoc
int voxelVariationLoc
int cellSizeLoc
int shellRadiusLoc
int surfaceCountLoc
int highlightIntensityLoc
int positionTintLoc
int tonemapGainLoc
int panLoc
int baseFreqLoc
int maxFreqLoc
int gainLoc
int curveLoc
int baseBrightLoc
int gradientLUTLoc
```

### Algorithm

The shader is built by applying the substitution table from the research doc to the primary reference code ("Refactoring" by OldEclipse) line by line. Each substitution row maps one original element to its AudioJones equivalent.

#### Shader Uniforms

```glsl
uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float flyPhase;
uniform float gridPhase;
uniform int marchSteps;
uniform float stepSize;
uniform float voxelScale;
uniform float voxelVariation;
uniform float cellSize;
uniform float shellRadius;
uniform int surfaceCount;
uniform float highlightIntensity;
uniform float positionTint;
uniform float tonemapGain;
uniform vec2 pan;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform sampler2D gradientLUT;
```

#### Main Loop Structure

The reference code's entire computation happens inside a for-loop with comma-operator chaining. Translated to readable GLSL:

```glsl
void main() {
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;
    vec3 rd = normalize(vec3(uv, 1.0));

    vec3 color = vec3(0.0);
    float t = 0.1;

    for (int i = 0; i < marchSteps; i++) {
        // Position: ray origin at (pan.x, pan.y, flyPhase), march along rd
        vec3 p = vec3(pan, flyPhase) + t * rd;

        // Voxel quantization with sin-boundary density variation
        float sinCount = step(0.0, sin(p.x + gridPhase))
                       + step(0.0, sin(p.y + 1.0 + gridPhase))
                       + step(0.0, sin(p.z + 2.0 + gridPhase));
        float s = voxelScale - voxelScale * 0.25 * voxelVariation * sinCount;
        p = round(p * s) / s;

        // Boundary highlight distance
        float boundaryDist = min(min(
            abs(sin(p.x + gridPhase)),
            abs(sin(p.y + 1.0 + gridPhase))),
            abs(sin(p.z + 2.0 + gridPhase))) + 0.01;

        // FFT brightness for this march step (depth-dependent audio)
        float t0 = float(i) / float(marchSteps - 1);
        float t1 = float(i + 1) / float(marchSteps);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);
        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int b = 0; b < BAND_SAMPLES; b++) {
            float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // Surface loop: each iteration folds p via mod, creating mirrored geometry
        float minDist = 1e9;
        for (int surf = 0; surf < surfaceCount; surf++) {
            p = mod(p, cellSize) - cellSize * 0.5;
            float dist = abs(length(p) - shellRadius) + 0.01;

            float gradientT = float(surf) / float(max(surfaceCount - 1, 1));
            vec3 surfColor = texture(gradientLUT, vec2(gradientT, 0.5)).rgb;

            color += (surfColor - p * positionTint + highlightIntensity / boundaryDist) / dist * brightness;

            minDist = min(minDist, dist);
        }

        // March forward
        t += stepSize * minDist;
    }

    finalColor = vec4(tanh(color * tonemapGain), 1.0);
}
```

#### Key Correspondences to Reference

| Reference Line | Shader Line | Notes |
|---|---|---|
| `vec3(1.,1.,iTime)` | `vec3(pan, flyPhase)` | pan.xy from DualLissajous, flyPhase CPU-accumulated |
| `sin(p + iTime)` | `sin(p.x + gridPhase)` etc. | gridPhase CPU-accumulated separately |
| `vec3(I+I,0)-iResolution.xyy` | `normalize(vec3(uv, 1.0))` | Standard UV-to-ray-direction |
| `16. - 4.*(step+step+step)` | `voxelScale - voxelScale * 0.25 * voxelVariation * sinCount` | voxelVariation=0 gives uniform grid |
| `round(p*s)/s` | `round(p * s) / s` | Kept verbatim |
| `min(abs(sin), abs(sin), abs(sin)) + .01` | `min(min(abs(sin(...)), abs(sin(...))), abs(sin(...))) + 0.01` | Kept verbatim |
| `A(v,0)` + `A(w,2)` macro calls | Surface loop 0..surfaceCount-1 | Each iteration folds p via mod |
| `mod(p,4.)-2.` | `mod(p, cellSize) - cellSize * 0.5` | Domain fold, cellSize uniform |
| `abs(length(p) - 2.8) + .01` | `abs(length(p) - shellRadius) + 0.01` | Shell SDF |
| `vec3(i,1,2-i)` color | `texture(gradientLUT, vec2(gradientT, 0.5)).rgb` | Gradient LUT sampling |
| `p*.5` position tint | `p * positionTint` | Configurable |
| `.06/s` highlight | `highlightIntensity / boundaryDist` | Configurable |
| `t += .3*min(v,w)` | `t += stepSize * minDist` | stepSize uniform, minDist across all surfaces |
| `tanh(c/2e3)` | `tanh(color * tonemapGain)` | tonemapGain = 0.0005 matches 1/2000 |

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| marchSteps | int | 30-80 | 60 | No | March Steps |
| stepSize | float | 0.1-0.5 | 0.3 | Yes | Step Size |
| voxelScale | float | 4.0-32.0 | 16.0 | Yes | Voxel Scale |
| voxelVariation | float | 0.0-1.0 | 1.0 | Yes | Voxel Variation |
| cellSize | float | 1.0-8.0 | 4.0 | Yes | Cell Size |
| shellRadius | float | 0.5-4.0 | 2.8 | Yes | Shell Radius |
| surfaceCount | float | 1-3 | 2.0 | Yes | Surfaces |
| highlightIntensity | float | 0.0-0.5 | 0.06 | Yes | Highlight |
| positionTint | float | 0.0-1.0 | 0.5 | Yes | Position Tint |
| tonemapGain | float | 0.0001-0.002 | 0.0005 | Yes | Tonemap Gain |
| flySpeed | float | 0.0-5.0 | 1.0 | Yes | Fly Speed |
| gridAnimSpeed | float | 0.0-3.0 | 1.0 | Yes | Grid Speed |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |

### Constants

- Enum name: `TRANSFORM_VOXEL_MARCH_BLEND`
- Display name: `"Voxel March"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `13` (Field)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create VoxelMarchConfig and VoxelMarchEffect

**Files**: `src/effects/voxel_march.h` (create)
**Creates**: Config struct, Effect struct, function declarations needed by all other tasks

**Do**: Create header following the Design > Types section above. Follow `light_medley.h` as template:
- Include `config/dual_lissajous_config.h`, `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`
- `VoxelMarchConfig` struct with all fields from Design section
- `VOXEL_MARCH_CONFIG_FIELDS` macro listing all serializable fields (exclude internal state)
- Forward declare `ColorLUT`
- `VoxelMarchEffect` typedef struct with all loc fields from Design section
- Function declarations: `VoxelMarchEffectInit(VoxelMarchEffect*, const VoxelMarchConfig*)`, `VoxelMarchEffectSetup(VoxelMarchEffect*, VoxelMarchConfig*, float, const Texture2D&)` (non-const config because DualLissajousUpdate mutates phase), `VoxelMarchEffectUninit(VoxelMarchEffect*)`, `VoxelMarchRegisterParams(VoxelMarchConfig*)`

**Verify**: `cmake.exe --build build` compiles (header-only, no link errors expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect Module Implementation

**Files**: `src/effects/voxel_march.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement effect module following `light_medley.cpp` as template. Includes: own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`.

Functions:
- `VoxelMarchEffectInit`: Load shader `shaders/voxel_march.fs`, cache all uniform locations, init `ColorLUT`, zero phase accumulators. Return false if shader or LUT fails (cascade cleanup).
- `VoxelMarchEffectSetup`: Accumulate `flyPhase += flySpeed * dt`, `gridPhase += gridAnimSpeed * dt`. Update ColorLUT. Compute DualLissajous pan. Bind all uniforms. `marchSteps` passed as int, `surfaceCount` cast to int for shader. fftTexture and gradientLUT bound as textures.
- `VoxelMarchEffectUninit`: Unload shader, free ColorLUT.
- `VoxelMarchRegisterParams`: Register all modulatable params from Parameters table (all except `marchSteps`). Use `"voxelMarch."` prefix. Lissajous: register `amplitude` and `motionSpeed`. Include `blendIntensity`.
- `SetupVoxelMarch(PostEffect*)`: Bridge function (NOT static). Calls `VoxelMarchEffectSetup`.
- `SetupVoxelMarchBlend(PostEffect*)`: Bridge function (NOT static). Calls `BlendCompositorApply`.

UI section (`// === UI ===`):
- `static void DrawVoxelMarchParams(EffectConfig*, const ModSources*, ImU32)`:
  - Audio section: Base Freq, Max Freq, Gain, Contrast, Base Bright (standard order and formats)
  - Geometry section: March Steps (`ImGui::SliderInt`), Step Size, Voxel Scale, Voxel Variation, Cell Size, Shell Radius, Surfaces (`ModulatableSliderInt`), Highlight (`ModulatableSliderLog`), Position Tint, Tonemap Gain (`ModulatableSliderLog`)
  - Motion section: Fly Speed, Grid Speed
  - Camera section: `DrawLissajousControls(&cfg->lissajous, "vm_cam", "voxelMarch.lissajous", modSources, 2.0f)`
- `STANDARD_GENERATOR_OUTPUT(voxelMarch)` macro
- `REGISTER_GENERATOR(TRANSFORM_VOXEL_MARCH_BLEND, VoxelMarch, voxelMarch, "Voxel March", SetupVoxelMarchBlend, SetupVoxelMarch, 13, DrawVoxelMarchParams, DrawOutput_voxelMarch)` wrapped in clang-format off/on

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Fragment Shader

**Files**: `shaders/voxel_march.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from this plan. Attribution header required:
```
// Based on "Refactoring" by OldEclipse
// https://www.shadertoy.com/view/WcycRw
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT audio reactivity, gradient LUT coloring
```

Declare all uniforms from the Algorithm > Shader Uniforms list. Implement `main()` exactly as specified in the Algorithm > Main Loop Structure. Key points:
- UV: `fragTexCoord * 2.0 - 1.0`, aspect-corrected
- Ray direction: `normalize(vec3(uv, 1.0))`
- Ray origin: `vec3(pan, flyPhase)`
- Voxel quantization: `round(p * s) / s` with sin-boundary density variation controlled by `voxelVariation`
- Boundary highlight: `min(min(abs(sin(...)), abs(sin(...))), abs(sin(...))) + 0.01`
- FFT: Standard frequency spread pattern mapping march step index to frequency band
- Surface loop: `for (int surf = 0; surf < surfaceCount; surf++)` with `mod` fold, shell SDF, gradient LUT color, position tint, highlight term
- March step: `t += stepSize * minDist`
- Tonemap: `tanh(color * tonemapGain)`

**Verify**: File exists, all uniforms match the .cpp uniform binding.

---

#### Task 2.3: Config Registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/voxel_march.h"` with the other effect includes
2. Add `TRANSFORM_VOXEL_MARCH_BLEND,` to `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE`
3. Add `VoxelMarchConfig voxelMarch;` to `EffectConfig` struct (near the other generator configs)

The order array auto-populates from enum index in the `TransformOrderConfig` constructor.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: PostEffect Struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/voxel_march.h"` with the other effect includes
2. Add `VoxelMarchEffect voxelMarch;` member to `PostEffect` struct (near `lightMedley` member)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Build System and Serialization

**Files**: `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. `CMakeLists.txt`: Add `src/effects/voxel_march.cpp` to `EFFECTS_SOURCES`
2. `effect_serialization.cpp`:
   - Add `#include "effects/voxel_march.h"` with the other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VoxelMarchConfig, VOXEL_MARCH_CONFIG_FIELDS)` with the other per-config macros
   - Add `X(voxelMarch) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Field generator section (section 13)
- [ ] Shader loads without errors
- [ ] FFT audio reactivity responds to music
- [ ] Gradient LUT colors the surfaces
- [ ] DualLissajous camera pan works
- [ ] Fly speed and grid animation speed accumulate smoothly on CPU
- [ ] surfaceCount 1-3 produces distinct visual layering
- [ ] voxelVariation 0 produces uniform grid, 1 produces sin-boundary variation
- [ ] Preset save/load round-trips all parameters
- [ ] Modulation routes to registered parameters
