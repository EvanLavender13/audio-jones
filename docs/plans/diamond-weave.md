# Diamond Weave

Tiled grid of diamond contour rings, slowly twisting under a radial swirl. Per-pixel ring index drives both the gradient LUT color and a BAND_SAMPLES FFT lookup, so brightness tracks bass at the inner contour through treble at the outer corners.

**Research**: `docs/research/diamond_weave.md`

## Design

### Types

`DiamondWeaveConfig` (in `src/effects/diamond_weave.h`):

```cpp
struct DiamondWeaveConfig {
  bool enabled = false;

  // Geometry
  float cellSize = 80.0f;     // Pixel size of each diamond tile (20-200)
  float twistAngle = 0.777f;  // Base rotation of tile grid (-PI to PI)
  float twistRadial = 0.3f;   // Radial twist coefficient (0.0-1.0)

  // Animation
  float phaseSpeed = 0.3f;    // Ringing pattern evolution rate (0.05-1.0)
  float twistRate = 0.0001f;  // Continuous twist rotation rate (0.0-0.001)
  float driftSpeed = 0.1f;    // Gradient hue drift rate (0.0-0.5)

  // Glow
  float glowIntensity = 1.0f; // Output brightness multiplier (0.0-2.0)

  // Audio
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DIAMOND_WEAVE_CONFIG_FIELDS                                            \
  enabled, cellSize, twistAngle, twistRadial, phaseSpeed, twistRate,           \
      driftSpeed, glowIntensity, baseFreq, maxFreq, gain, curve, baseBright,   \
      gradient, blendMode, blendIntensity
```

`DiamondWeaveEffect` (in `src/effects/diamond_weave.h`):

```cpp
typedef struct DiamondWeaveEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated rates (one per *Speed/*Rate config field)
  float phaseAccum;
  float twistAccum;
  float driftAccum;

  // Shader uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int gradientLUTLoc;
  int sampleRateLoc;
  int cellSizeLoc;
  int twistAngleLoc;
  int twistRadialLoc;
  int glowIntensityLoc;
  int phaseAccumLoc;
  int twistAccumLoc;
  int driftAccumLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} DiamondWeaveEffect;
```

### Algorithm

The shader uses a CPU-accumulated phase per rate (sibling pattern from `spiral_nest.cpp`/`shaders/spiral_nest.fs`, `density_wave_spiral.cpp`, `dream_zoom.cpp`): `phaseAccum`, `twistAccum`, `driftAccum` are accumulated each frame as `accum += speed * deltaTime`, then bound as uniforms. The shader applies the `log(1.0 + phaseAccum)` anti-aliasing fix to the phase accumulator and uses the other accumulators raw.

<!-- Intentional deviation: research's substitution table reads `sin(phaseSpeed * log(1 + timeAccum) * shape(...))` (phaseSpeed multiplied OUTSIDE the log). With CPU accumulation, `phaseSpeed * timeAccum` lives INSIDE the log, giving `sin(log(1 + phaseAccum) * shape(...))`. This matches the spiral_nest precedent (which makes the same algebraic shift on `animSpeed`) and provides smooth modulation: changing `phaseSpeed` at runtime affects future growth rate without retroactively rescaling history. The temporal-aliasing fix is preserved. -->

<!-- Intentional deviation: research is silent on default blend mode. EFFECT_BLEND_SCREEN matches the codebase convention for additive-glow texture generators (spiral_nest, density_wave_spiral). -->


Diamond Weave fragment shader (`shaders/diamond_weave.fs`):

```glsl
// Based on "Pixel Weave" by TekF
// https://www.shadertoy.com/view/XdS3RK
// Itself derived from "no title" by gaz (https://www.shadertoy.com/view/Md2Gzy)
// License: CC BY-NC-SA 3.0 Unported
// Modified: log-time fix on phase accumulator, gradient LUT replaces HSV,
// FFT BAND_SAMPLES brightness indexed by ring value, separate CPU-accumulated
// rates for phase, twist, and gradient drift

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float cellSize;
uniform float twistAngle;
uniform float twistRadial;
uniform float glowIntensity;

uniform float phaseAccum;
uniform float twistAccum;
uniform float driftAccum;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

float shape(vec2 p) {
    return abs(p.x) + abs(p.y) - 1.0;
}

void main() {
    vec2 pos = fragTexCoord * resolution - resolution * 0.5;

    float a = twistAngle + twistAccum *
              (1.0 + twistRadial * pow(length(pos / resolution.y), 2.0));
    pos = pos * cos(a) + vec2(pos.y, -pos.x) * sin(a);

    pos = mod(pos / cellSize, 2.0) - 1.0;

    float t = log(1.0 + phaseAccum);
    float h = abs(sin(t * shape(3.0 * pos)));
    float c = 0.05 / max(h, 1e-4);

    float freq = baseFreq * pow(maxFreq / baseFreq, h);
    float baseBin = freq / (sampleRate * 0.5);
    float binStep = 1.0 / float(textureSize(fftTexture, 0).x);

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = baseBin + (float(s) - 1.5) * binStep;
        if (bin >= 0.0 && bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    vec3 color = texture(gradientLUT, vec2(fract(driftAccum + h), 0.5)).rgb;
    finalColor = vec4(color * (c * 0.5 + 0.5) * brightness * glowIntensity, 1.0);
}
```

CPU-side per-frame setup (`DiamondWeaveEffectSetup`):

```
phaseAccum += phaseSpeed * deltaTime
twistAccum += twistRate  * deltaTime
driftAccum += driftSpeed * deltaTime
ColorLUTUpdate(gradientLUT, &cfg->gradient)
bind resolution, fftTexture, gradientLUT, sampleRate
bind cellSize, twistAngle, twistRadial, glowIntensity
bind phaseAccum, twistAccum, driftAccum
bind baseFreq, maxFreq, gain, curve, baseBright
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| cellSize | float | 20-200 | 80 | yes | `Cell Size` |
| twistAngle | float | -PI..PI (`ROTATION_OFFSET_MAX`) | 0.777 | yes | `Twist Angle` (deg) |
| twistRadial | float | 0.0-1.0 | 0.3 | yes | `Twist Radial` |
| phaseSpeed | float | 0.05-1.0 | 0.3 | yes | `Phase Speed` |
| twistRate | float | 0.0-0.001 | 0.0001 | yes | `Twist Rate` (`%.5f`) |
| driftSpeed | float | 0.0-0.5 | 0.1 | yes | `Drift Speed` |
| glowIntensity | float | 0.0-2.0 | 1.0 | yes | `Glow Intensity` |
| baseFreq | float | 27.5-440 | 55 | yes | `Base Freq (Hz)` |
| maxFreq | float | 1000-16000 | 14000 | yes | `Max Freq (Hz)` |
| gain | float | 0.1-10 | 2.0 | yes | `Gain` |
| curve | float | 0.1-3 | 1.5 | yes | `Contrast` |
| baseBright | float | 0-1 | 0.15 | yes | `Base Bright` |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | (standard generator output) |
| gradient | ColorConfig | -- | gradient mode | -- | (standard generator output) |
| blendMode | enum | -- | `EFFECT_BLEND_SCREEN` | -- | (standard generator output) |

UI sections (in `DrawDiamondWeaveParams`):

```
SeparatorText("Audio")
  Base Freq, Max Freq, Gain, Contrast, Base Bright
SeparatorText("Geometry")
  Cell Size, Twist Angle (ModulatableSliderAngleDeg), Twist Radial
SeparatorText("Animation")
  Phase Speed, Twist Rate, Drift Speed
SeparatorText("Glow")
  Glow Intensity
```

`STANDARD_GENERATOR_OUTPUT(diamondWeave)` provides the Color and Output (Blend Intensity, Blend Mode) sections.

### Constants

- Enum name: `TRANSFORM_DIAMOND_WEAVE_BLEND`
- Display name: `"Diamond Weave"`
- Category: `"GEN"` badge (auto-set by `REGISTER_GENERATOR`)
- Section index: `12` (Texture)
- Field name on `EffectConfig`: `diamondWeave`
- File names: `src/effects/diamond_weave.{h,cpp}`, `shaders/diamond_weave.fs`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create `diamond_weave.h`

**Files**: `src/effects/diamond_weave.h`
**Creates**: `DiamondWeaveConfig`, `DiamondWeaveEffect`, `DIAMOND_WEAVE_CONFIG_FIELDS`, public function declarations.

**Do**: Create the header per the Types section. Include `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`. Forward declare `struct PostEffect;` and typedef `struct ColorLUT ColorLUT;`. Declare:

```cpp
bool DiamondWeaveEffectInit(DiamondWeaveEffect *e, const DiamondWeaveConfig *cfg);
void DiamondWeaveEffectSetup(DiamondWeaveEffect *e, const DiamondWeaveConfig *cfg,
                             float deltaTime, const Texture2D &fftTexture);
void DiamondWeaveEffectUninit(DiamondWeaveEffect *e);
void DiamondWeaveRegisterParams(DiamondWeaveConfig *cfg);
DiamondWeaveEffect *GetDiamondWeaveEffect(PostEffect *pe);
```

Match the structure of `src/effects/spiral_nest.h`.

**Verify**: Header parses; `cmake.exe --build build` does not need to run yet (no consumers).

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Create `diamond_weave.cpp`

**Files**: `src/effects/diamond_weave.cpp`
**Depends on**: Wave 1 complete.

**Do**: Implement the effect module following `src/effects/spiral_nest.cpp` as the structural template.
- `DiamondWeaveEffectInit`: load `shaders/diamond_weave.fs`, cascade-cleanup on failure (unload shader if LUT init fails). Cache all uniform locations listed in the `DiamondWeaveEffect` struct. Init `gradientLUT` from `cfg->gradient`. Zero the three accumulators.
- `DiamondWeaveEffectSetup`: accumulate the three rates per the Algorithm section, call `ColorLUTUpdate`, bind every uniform.
- `DiamondWeaveEffectUninit`: `UnloadShader`, `ColorLUTUninit`.
- `DiamondWeaveRegisterParams`: register every modulatable param at the ranges in the Parameters table, including `diamondWeave.blendIntensity` (0.0f, 5.0f).
- `GetDiamondWeaveEffect(pe)` accessor: cast `pe->effectStates[TRANSFORM_DIAMOND_WEAVE_BLEND]`.
- Non-static bridge `SetupDiamondWeave(PostEffect *pe)`: calls `DiamondWeaveEffectSetup` with `pe->fftTexture` and `pe->currentDeltaTime`.
- Non-static bridge `SetupDiamondWeaveBlend(PostEffect *pe)`: calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, cfg->blendIntensity, cfg->blendMode)`.
- `// === UI ===` section with static `DrawDiamondWeaveParams` per the Parameters section. Twist Angle uses `ModulatableSliderAngleDeg` with `ROTATION_OFFSET_MAX` from `config/constants.h`. Twist Rate uses `ModulatableSlider` with `"%.5f"` format.
- `STANDARD_GENERATOR_OUTPUT(diamondWeave)` immediately before the registration macro.
- `REGISTER_GENERATOR(TRANSFORM_DIAMOND_WEAVE_BLEND, DiamondWeave, diamondWeave, "Diamond Weave", SetupDiamondWeaveBlend, SetupDiamondWeave, 12, DrawDiamondWeaveParams, DrawOutput_diamondWeave)` wrapped in `// clang-format off` / `// clang-format on`.

Includes per the conventions for generator effects with colocated UI: own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `imgui.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<math.h>`, `<stddef.h>`.

**Verify**: `cmake.exe --build build` compiles after Wave 2 completes (this task alone won't link without the other Wave 2 tasks).

---

#### Task 2.2: Create `shaders/diamond_weave.fs`

**Files**: `shaders/diamond_weave.fs`
**Depends on**: Wave 1 complete.

**Do**: Create the file with the exact shader source from the Algorithm section, including the attribution comment block before `#version 330`.

**Verify**: File exists at the expected path; build will load it from `LoadShader(NULL, "shaders/diamond_weave.fs")`.

---

#### Task 2.3: Register in `effect_config.h`

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete.

**Do**:
1. Add `#include "effects/diamond_weave.h"` to the alphabetical include block at the top.
2. Add `TRANSFORM_DIAMOND_WEAVE_BLEND,` to the `TransformEffectType` enum. Place it among the other generator `_BLEND` entries; near `TRANSFORM_SPIRAL_NEST_BLEND` is a natural spot.
3. Add `DiamondWeaveConfig diamondWeave;` member to the `EffectConfig` struct, near the other generator configs (`SpiralNestConfig spiralNest;`, etc.).

The `TransformOrderConfig` constructor already auto-fills the order array via iota, so no order-array edit is needed.

**Verify**: `cmake.exe --build build` compiles successfully.

---

#### Task 2.4: Register in `effect_serialization.cpp`

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete.

**Do**:
1. Add `#include "effects/diamond_weave.h"` alongside the other effect includes if not already pulled in transitively (it will be via `effect_config.h`, but add an explicit include if other generator headers list themselves).
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DiamondWeaveConfig, DIAMOND_WEAVE_CONFIG_FIELDS)` alongside the other per-config macros (alphabetical region around `SpiralNestConfig`).
3. Add `X(diamondWeave)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: `cmake.exe --build build` compiles successfully.

---

## Implementation Notes

The animation parameters were restructured during implementation after testing showed the original substitution-table mapping produced confusing UX:

- The TekF formula bundles three behaviors into one expression: `a = twistAngle + iTime * twistRate * (1 + twistRadial * radial²)`. That collapses to *static rotation + uniform animated rotation + differential twist*. Naming any of these "twist" is misleading because two of the three are uniform rotations, not twists.
- `twistRate` (0.0-0.001) and `twistRadial` ended up doing roughly the same thing because they both scale the same accumulator with similar magnitudes.
- The research's `t = log(1 + timeAccum)` anti-aliasing fix saturates the phase argument: `1/(1+t)` derivative makes the pattern asymptotically freeze rather than slow.

Final parameter set decouples the three behaviors and drops the log:

- `baseAngle` (UI: "Angle"): static grid rotation. Range `±ROTATION_OFFSET_MAX`, default 0.777.
- `rotationSpeed` (UI: "Rotation Speed"): uniform rotation rate. Range `±ROTATION_SPEED_MAX`, default 0.0.
- `twistSpeed` (UI: "Twist Speed"): differential swirl rate (outer pixels accumulate faster, in proportion to `radial²`). Range `±ROTATION_SPEED_MAX`, default 0.02.
- `phaseSpeed`, `driftSpeed`: unchanged.
- `twistAngle`, `twistRate`, `twistRadial`: removed.

`DiamondWeaveEffect` carries four CPU accumulators (`phaseAccum`, `rotationAccum`, `twistAccum`, `driftAccum`), each `accum += rate * deltaTime`, so slider changes affect future rate without jumping the current state. Shader formula:

```glsl
float radial = length(pos / resolution.y);
float a = baseAngle + rotationAccum + twistAccum * radial * radial;
float h = abs(sin(phaseAccum * shape(3.0 * pos)));
```

No log on the phase argument — pattern progresses linearly. Aliasing eventually appears at very long runtimes (the standard tradeoff for matching the TekF reference); a single-line `fwidth`-based attenuation can be added later if it becomes a problem.

## Final Verification

- [ ] Build succeeds with no warnings.
- [ ] "Diamond Weave" appears under the Texture generator section in the Effects window with the `GEN` badge.
- [ ] Enabling the effect produces a tiled diamond-ringing pattern that matches TekF's Shadertoy reference (slow continuous twist, brightness floor of 0.5, fract-driven gradient drift).
- [ ] Sliders modify the effect in real-time. Twist Angle slider displays degrees; Twist Rate slider shows 5 decimal places of precision.
- [ ] Audio reactivity: rings react to the spectrum from inner (bass) to outer (treble).
- [ ] Preset save/load round-trips all `DIAMOND_WEAVE_CONFIG_FIELDS` values.
- [ ] Modulation panel can route any `diamondWeave.*` registered param.
- [ ] No divide-by-zero artifacts on the diamond contours (the `max(h, 1e-4)` guard handles `h -> 0`).
