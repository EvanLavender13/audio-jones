# LED Cube

A rotating 3D grid of glowing LED points. A hot spot traces the cube's 12 edges in sequence and lights up nearby LEDs from FFT audio -- low frequencies light close LEDs, high frequencies reach further out. The grid breathes from tight lattice to organic drift. Implemented as a generator in the Field section.

**Research**: `docs/research/led_cube.md`

## Design

### Types

**`LedCubeConfig`** (in `src/effects/led_cube.h`):

```cpp
struct LedCubeConfig {
  bool enabled = false;

  // Grid
  int gridSize = 5;          // Points per axis, gridSize^3 total LEDs (3-10)
  float glowFalloff = 0.15f; // Point glow spread - sharp pinprick to soft halo (0.01-1.0)
  float displacement = 0.0f; // Grid loosening - 0=rigid lattice, 1=organic drift (0.0-1.0)

  // Camera
  float cubeScale = 4.5f; // Apparent cube size in viewport (2.0-8.0)
  float fovDiv = 2.2f;    // Perspective divisor - higher=flatter (1.0-4.0)

  // Speed (radians/second, accumulated on CPU)
  float tracerSpeed = 1.0f; // Edge tracer traversal speed (0.1-4.0)
  float rotSpeedA = 0.25f;  // First-axis rotation rate (-PI to PI)
  float rotSpeedB = 0.15f;  // Second-axis rotation rate (-PI to PI)
  float driftSpeed = 1.0f;  // Displacement drift rate (0.0-5.0)

  // Audio
  float baseFreq = 55.0f;   // FFT low bound Hz (27.5-440)
  float maxFreq = 14000.0f; // FFT high bound Hz (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum LED brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define LED_CUBE_CONFIG_FIELDS                                                 \
  enabled, gridSize, glowFalloff, displacement, cubeScale, fovDiv,             \
      tracerSpeed, rotSpeedA, rotSpeedB, driftSpeed, baseFreq, maxFreq, gain,  \
      curve, baseBright, gradient, blendMode, blendIntensity
```

**`LedCubeEffect`** (in `src/effects/led_cube.h`):

```cpp
typedef struct ColorLUT ColorLUT;

typedef struct LedCubeEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases
  float tracerPhase;
  float rotPhaseA;
  float rotPhaseB;
  float driftPhase;

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int gridSizeLoc;
  int tracerPhaseLoc;
  int rotPhaseALoc;
  int rotPhaseBLoc;
  int driftPhaseLoc;
  int glowFalloffLoc;
  int displacementLoc;
  int cubeScaleLoc;
  int fovDivLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} LedCubeEffect;

bool LedCubeEffectInit(LedCubeEffect *e, const LedCubeConfig *cfg);
void LedCubeEffectSetup(LedCubeEffect *e, const LedCubeConfig *cfg,
                        float deltaTime, const Texture2D &fftTexture);
void LedCubeEffectUninit(LedCubeEffect *e);
void LedCubeRegisterParams(LedCubeConfig *cfg);
```

### Algorithm

Fragment shader receives per-pixel coordinates and accumulates glow from every LED in the grid. The triple loop iterates `gridSize^3` grid points; for each, it computes audio-reactive brightness from its distance to the hot spot, projects the point into screen space, and adds a Gaussian glow.

Inlined shader (keep verbatim unless commented otherwise):

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform int gridSize;
uniform float tracerPhase;
uniform float rotPhaseA;
uniform float rotPhaseB;
uniform float driftPhase;
uniform float glowFalloff;
uniform float displacement;
uniform float cubeScale;
uniform float fovDiv;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float TAU = 6.28318530718;
const float INV_SQRT_3 = 0.57735; // Max distance in unit cube

// Maps a continuous phase value to a position on the unit cube's 12-edge
// skeleton. One full cycle = phase advancing by 12.0. Verbatim from research.
vec3 getCubeEdgePosition(float beat) {
    float e = mod(beat, 12.0);
    float t = fract(beat);
    if (e < 1.0)  return vec3(t, 0.0, 0.0);
    if (e < 2.0)  return vec3(1.0, t, 0.0);
    if (e < 3.0)  return vec3(1.0 - t, 1.0, 0.0);
    if (e < 4.0)  return vec3(0.0, 1.0 - t, 0.0);
    if (e < 5.0)  return vec3(0.0, 0.0, t);
    if (e < 6.0)  return vec3(t, 0.0, 1.0);
    if (e < 7.0)  return vec3(1.0, t, 1.0);
    if (e < 8.0)  return vec3(1.0 - t, 1.0, 1.0);
    if (e < 9.0)  return vec3(0.0, 1.0 - t, 1.0);
    if (e < 10.0) return vec3(1.0, 0.0, 1.0 - t);
    if (e < 11.0) return vec3(1.0, 1.0, t);
    return vec3(0.0, 1.0, 1.0 - t);
}

void main() {
    // Centered aspect-ratio UV, origin at screen center, x in [-1, 1] full-width
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 uv = (2.0 * fragCoord - resolution) / resolution.x;

    // Camera at origin looking down +z; derive z from cubeScale to preserve
    // the reference framing (ref: CAM_Z = -(BOX_HALF + CAM_DIST) = -3.8 at
    // CUBE_SCALE = 4.5; ratio 3.8 / 4.5 = 0.845).
    vec3 camRo = vec3(0.0, 0.0, -cubeScale * 0.845);

    // Two independent rotation axes, computed once per pixel.
    mat2 R1 = mat2(cos(rotPhaseA), -sin(rotPhaseA),
                   sin(rotPhaseA),  cos(rotPhaseA));
    mat2 R2 = mat2(cos(rotPhaseB), -sin(rotPhaseB),
                   sin(rotPhaseB),  cos(rotPhaseB));

    // Hot spot on the unit cube's edge skeleton; all LED reactivity keys
    // off distance from sp.
    vec3 sp = getCubeEdgePosition(tracerPhase);
    vec3 toCenter = normalize(vec3(0.5) - sp);

    float st = 1.0 / float(gridSize);
    // Offset that centers the grid midpoint at origin for any gridSize.
    // Points are at x = 0, st, 2*st, ..., (gridSize-1)*st; midpoint is
    // (gridSize-1) / (2*gridSize) = 0.5 - st/2. Subtracting this centers at 0.
    float centerOffset = 0.5 - st * 0.5;

    vec3 O = vec3(0.0);

    for (int iz = 0; iz < gridSize; iz++) {
      for (int iy = 0; iy < gridSize; iy++) {
        for (int ix = 0; ix < gridSize; ix++) {
            vec3 xyz = vec3(float(ix), float(iy), float(iz)) * st;

            // Distance from hot spot, normalized to [0, 1] by INV_SQRT_3.
            // ap is the shared t index for gradient LUT and FFT lookup.
            vec3 diff = xyz - sp;
            float d0 = length(diff);
            float ap = clamp(d0 * INV_SQRT_3, 0.0, 1.0);

            // Directional emphasis: dims LEDs on the hot-spot-to-center axis,
            // brightens LEDs off to the sides. Verbatim from reference.
            float df = max(0.0, dot(diff / (d0 + 1e-5), toCenter));

            // Standard FFT frequency spread. Near LEDs (ap ~ 0) pull from
            // baseFreq, far LEDs (ap ~ 1) pull from maxFreq.
            float freq = baseFreq * pow(maxFreq / baseFreq, ap);
            float bin = freq / (sampleRate * 0.5);
            float energy = 0.0;
            if (bin <= 1.0) {
                energy = texture(fftTexture, vec2(bin, 0.5)).r;
            }
            float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
            float brightness = baseBright + mag * (1.0 - ap);
            brightness *= 0.5 + (1.0 - df);

            // Coherent sin-wave drift per axis; displacement scales amplitude.
            // Three slightly detuned phases (1.0, 0.7, 1.3) keep the drift
            // from locking across axes.
            vec3 disp = vec3(sin(xyz.y * TAU + driftPhase),
                             cos(xyz.x * TAU + driftPhase * 0.7),
                             sin(xyz.z * TAU + driftPhase * 1.3)) * displacement;

            // Center the grid at origin, displace, rotate, scale.
            vec3 p = xyz - vec3(centerOffset) + disp;
            p.yx *= R1;
            p.zy *= R2;
            p *= cubeScale;

            // Perspective projection.
            vec3 pRel = p - camRo;
            if (pRel.z < 0.1) continue; // Skip points at/behind camera plane
            vec2 pProj = pRel.xy / (pRel.z * fovDiv);
            float d = length(uv - pProj);

            // Gradient LUT color + Gaussian glow. ap indexes both color and
            // FFT, so each LED's color matches its frequency band.
            vec3 color = texture(gradientLUT, vec2(ap, 0.5)).rgb;
            O += color * brightness * exp(-d * d / (glowFalloff * glowFalloff));
        }
      }
    }

    finalColor = vec4(O, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| gridSize | int | 3-10 | 5 | No | Grid Size |
| glowFalloff | float | 0.01-1.0 | 0.15 | Yes | Glow Falloff |
| displacement | float | 0.0-1.0 | 0.0 | Yes | Displacement |
| cubeScale | float | 2.0-8.0 | 4.5 | Yes | Cube Scale |
| fovDiv | float | 1.0-4.0 | 2.2 | Yes | FOV Divisor |
| tracerSpeed | float | 0.1-4.0 | 1.0 | Yes | Tracer Speed |
| rotSpeedA | float | -PI..PI | 0.25 | Yes | Rotation A |
| rotSpeedB | float | -PI..PI | 0.15 | Yes | Rotation B |
| driftSpeed | float | 0.0-5.0 | 1.0 | Yes | Drift Speed |
| baseFreq | float | 27.5-440 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_LED_CUBE_BLEND`
- Display name: `"LED Cube"`
- Field name: `ledCube`
- Category: GEN (generator, auto-set by `REGISTER_GENERATOR`)
- Category section index: `13` (Field)
- Registration macro: `REGISTER_GENERATOR` (init takes `Effect*, Config*`; no resize)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create `led_cube.h`

**Files**: `src/effects/led_cube.h`
**Creates**: `LedCubeConfig`, `LedCubeEffect`, `LED_CUBE_CONFIG_FIELDS`, public lifecycle function declarations

**Do**: Create the header following the structure in the Design section. Top-of-file comment: `// LED Cube effect module` + one-line description. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Declare `LedCubeConfig` with all fields and defaults from Design, the `LED_CUBE_CONFIG_FIELDS` macro, the `LedCubeEffect` struct with all phase and uniform location fields, and the four public functions (`Init`, `Setup`, `Uninit`, `RegisterParams`). Match the conventions in `src/effects/voxel_march.h` and `src/effects/neon_lattice.h`.

**Verify**: `cmake.exe --build build` compiles (header alone won't error; this task is a no-op until Wave 2 uses it, but the file must parse cleanly).

---

### Wave 2: Implementation + Integration

All Wave 2 tasks depend on Wave 1 complete. None share files with each other; run in parallel.

#### Task 2.1: Create `led_cube.cpp`

**Files**: `src/effects/led_cube.cpp`
**Depends on**: Wave 1 complete

**Do**: Create the effect module. Structure:

1. Top-of-file comment: `// LED Cube effect module implementation` + one line on technique.
2. Includes (clang-format sorts within groups): own header; `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`; `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`; `<stddef.h>`.
3. `LedCubeEffectInit(LedCubeEffect*, const LedCubeConfig*)`: load `"shaders/led_cube.fs"`, cache every uniform location in the Design struct, init `gradientLUT` from `cfg->gradient`, zero all four phases. On shader-load failure return false; on LUT failure unload shader and return false.
4. `LedCubeEffectSetup(LedCubeEffect*, const LedCubeConfig*, float deltaTime, const Texture2D& fftTexture)`: accumulate `tracerPhase`, `rotPhaseA`, `rotPhaseB`, `driftPhase` from their speed fields times `deltaTime`; update `gradientLUT` via `ColorLUTUpdate`; bind resolution (`GetScreenWidth/Height`), `fftTexture`, `sampleRate` = `(float)AUDIO_SAMPLE_RATE`; bind every remaining uniform from the Design struct. Follow `VoxelMarchEffectSetup` as the pattern.
5. `LedCubeEffectUninit`: unload shader, uninit `gradientLUT`.
6. `LedCubeRegisterParams(LedCubeConfig* cfg)`: call `ModEngineRegisterParam` for every field in the Parameters table where Modulatable = Yes. Angular fields `rotSpeedA`, `rotSpeedB` use bounds `-ROTATION_SPEED_MAX` to `+ROTATION_SPEED_MAX` (from `config/constants.h`).
7. Bridge functions (non-static, at bottom above macro): `void SetupLedCube(PostEffect* pe)` calls `LedCubeEffectSetup` with `pe->ledCube`, `pe->effects.ledCube`, `pe->currentDeltaTime`, `pe->fftTexture`. `void SetupLedCubeBlend(PostEffect* pe)` calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.ledCube.blendIntensity, pe->effects.ledCube.blendMode)`.
8. `// === UI ===` section below the core: `static void DrawLedCubeParams(EffectConfig*, const ModSources*, ImU32)`. Sections in order:
   - `Audio`: Base Freq, Max Freq, Gain, Contrast, Base Bright (all `ModulatableSlider`, formats per conventions `"%.1f"`, `"%.0f"`, `"%.1f"`, `"%.2f"`, `"%.2f"`).
   - `Geometry`: `ImGui::SliderInt` for `gridSize` (3-10), then `ModulatableSlider` for `cubeScale`, `fovDiv`, `glowFalloff`, `displacement` (use `ModulatableSliderLog` for `glowFalloff` since it spans 0.01-1.0).
   - `Animation`: `ModulatableSlider` for `tracerSpeed`; `ModulatableSliderSpeedDeg` for `rotSpeedA`, `rotSpeedB`; `ModulatableSlider` for `driftSpeed`.
9. `STANDARD_GENERATOR_OUTPUT(ledCube)` above the registration macro.
10. Registration:
    ```
    // clang-format off
    STANDARD_GENERATOR_OUTPUT(ledCube)
    REGISTER_GENERATOR(TRANSFORM_LED_CUBE_BLEND, LedCube, ledCube, "LED Cube",
                       SetupLedCubeBlend, SetupLedCube, 13,
                       DrawLedCubeParams, DrawOutput_ledCube)
    // clang-format on
    ```

Match the file shape of `src/effects/voxel_march.cpp` (comparable generator: FFT spread, gradient LUT, multiple CPU-accumulated phases, no resize, no scratch buffer ownership).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Create `shaders/led_cube.fs`

**Files**: `shaders/led_cube.fs`
**Depends on**: Wave 1 complete

**Do**: Create the fragment shader. Start with the attribution block:

```
// Based on "LED Cube Visualizer" by orblivius
// https://www.shadertoy.com/view/7fsXWj
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT audio reactivity, gradient LUT coloring,
// walls and volumetric fog removed
```

Then implement the shader exactly as inlined in the Algorithm section of this document. Do not pull additional code from the research doc; the Algorithm section is the authoritative source. Every uniform, the TAU/INV_SQRT_3 constants, `getCubeEdgePosition`, and the triple loop must match. No tonemap on the final output (`finalColor = vec4(O, 1.0)`).

**Verify**: `cmake.exe --build build` compiles (shader compile errors surface at runtime, so also confirm the compiled binary launches without logging a shader error).

---

#### Task 2.3: Wire `post_effect.h`

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/led_cube.h"` to the alphabetically-sorted include block at the top. Add `LedCubeEffect ledCube;` to the `PostEffect` struct, alphabetically placed within the existing effect-member block (between `lightMedley` and `matrixRain` is not correct; insert alphabetically among the other effect members, e.g., between `latticeCrush` and `lightMedley`, or follow clang-format placement).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Wire `effect_config.h`

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Do**: Three edits:
1. Add `#include "effects/led_cube.h"` to the alphabetized include block.
2. Add `TRANSFORM_LED_CUBE_BLEND,` to the `TransformEffectType` enum. Place before `TRANSFORM_ACCUM_COMPOSITE` (which must remain the last non-count entry) and before `TRANSFORM_EFFECT_COUNT`. Appending near the other recently-added `_BLEND` entries (e.g., after `TRANSFORM_MARBLE_BLEND` / `TRANSFORM_SNAKE_SKIN_BLEND`) is fine.
3. Add `LedCubeConfig ledCube;` to the `EffectConfig` struct, with a one-line comment above it: `// LED Cube (3D grid of glowing LED points with FFT-reactive edge tracer)`. Place near other generators in the struct.

**Verify**: `cmake.exe --build build` compiles. The order array does not need manual editing -- `TransformOrderConfig()` constructor auto-populates it from the enum.

---

#### Task 2.5: Wire `effect_serialization.cpp`

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**: Three edits:
1. Add `#include "effects/led_cube.h"` to the alphabetized include block.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LedCubeConfig, LED_CUBE_CONFIG_FIELDS)` to the H-N effect configs block (alphabetically between `LaserDanceConfig` and `LightMedleyConfig`).
3. Add `X(ledCube)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table. Placement is free-form; append near other recent generators.

**Verify**: `cmake.exe --build build` compiles. Save a preset with LED Cube enabled and reload it; config round-trips.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in the Field section of the UI with the "GEN" badge
- [ ] Enabling LED Cube adds it to the pipeline and renders LED points
- [ ] Audio reactivity: silence -> dim points at baseBright; music -> bright frequency-mapped pulses
- [ ] Tracer sweeps all 12 edges (watch for ~12 seconds at default tracerSpeed=1.0)
- [ ] Rotation works on both axes independently
- [ ] `displacement = 0` produces a rigid lattice; `displacement = 1` produces organic drift
- [ ] `glowFalloff` crossfades from sharp pinpricks to soft halos
- [ ] `gridSize` scales from 3 (sparse) to 10 (dense) without crashing
- [ ] Modulation routes correctly (LFO -> glowFalloff modulates visibly)
- [ ] Preset save/load round-trips all fields
