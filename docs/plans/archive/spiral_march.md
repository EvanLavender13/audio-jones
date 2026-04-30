# Spiral March

A raymarched flight through an infinite tumbling lattice of SDF primitives, where the sample point itself rotates as it marches so straight rays trace helical paths. New Field-category generator effect; same compute model as Voxel March / Cyber March / Geode (single fragment shader, no compute pass, no input texture, blends via the standard generator scratch path).

**Research**: `docs/research/spiral_march.md`

---

## Design

### Types

#### `SpiralMarchConfig` (in `src/effects/spiral_march.h`)

```cpp
struct SpiralMarchConfig {
  bool enabled = false;

  // Geometry
  int shapeType = 0;         // 0=Octahedron, 1=Box, 2=Sphere, 3=Torus
  float shapeSize = 0.25f;   // Half-extent of SDF primitive (0.05-0.45)
  float cellPitchZ = 0.20f;  // Z-axis spacing between cells (0.05-1.0)

  // Raymarching
  int marchSteps = 80;             // Max raymarch iterations (30-120)
  float stepFactor = 0.9f;         // Distance multiplier per step (0.5-1.0)
  float maxDist = 100.0f;          // Far clip distance (20-200)
  float spiralRate = 0.8f;         // Ray-space rotation per unit dt (0-3); the doom-spiral knob
  float tColorDistScale = 0.012f;  // Color/FFT index from march distance (0-0.1)
  float tColorIterScale = 0.005f;  // Color/FFT index from iteration count (0-0.05)

  // Animation (CPU-accumulated phases)
  float flySpeed = 0.20f;          // Forward translation rate (0-5)
  float cellRotateSpeed = 1.0f;    // Per-cell tumble rate rad/s (0-ROTATION_SPEED_MAX)

  // Camera
  float fov = 0.15f;               // Field of view scalar (0.05-1.0)
  float pitchAngle = 0.53f;        // Camera pitch radians (-PI to +PI)
  float rollAngle = -0.531f;       // Camera roll radians; reference's 100.0 wrapped to [-PI, PI] = 100 - 16*PI ~ -0.531

  // Audio (FFT)
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

#define SPIRAL_MARCH_CONFIG_FIELDS                                              \
  enabled, shapeType, shapeSize, cellPitchZ, marchSteps, stepFactor, maxDist,   \
      spiralRate, tColorDistScale, tColorIterScale, flySpeed, cellRotateSpeed,  \
      fov, pitchAngle, rollAngle, baseFreq, maxFreq, gain, curve, baseBright,   \
      gradient, blendMode, blendIntensity
```

#### `SpiralMarchEffect` (in `src/effects/spiral_march.h`)

```cpp
typedef struct ColorLUT ColorLUT;

typedef struct SpiralMarchEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;     // accumulates flySpeed * deltaTime
  float cellPhase;    // accumulates cellRotateSpeed * deltaTime
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int flyPhaseLoc;
  int cellPhaseLoc;
  int shapeTypeLoc;
  int shapeSizeLoc;
  int cellPitchZLoc;
  int marchStepsLoc;
  int stepFactorLoc;
  int maxDistLoc;
  int spiralRateLoc;
  int tColorDistScaleLoc;
  int tColorIterScaleLoc;
  int fovLoc;
  int pitchAngleLoc;
  int rollAngleLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} SpiralMarchEffect;
```

#### Public API

```cpp
bool SpiralMarchEffectInit(SpiralMarchEffect *e, const SpiralMarchConfig *cfg);
void SpiralMarchEffectSetup(SpiralMarchEffect *e, SpiralMarchConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);
void SpiralMarchEffectUninit(SpiralMarchEffect *e);
void SpiralMarchRegisterParams(SpiralMarchConfig *cfg);
```

(Setup takes non-const config to allow `ColorLUTUpdate` to mutate the LUT cache. No DualLissajous in this effect — research explicitly excludes a screen-space pan offset.)

---

### Algorithm

#### Fragment shader (`shaders/spiral_march.fs`)

Complete shader. Begin with the attribution comment block (the research doc has a Shadertoy attribution). Uniforms map 1:1 to the cached locations on `SpiralMarchEffect`.

```glsl
// Based on "Evil Doom Spiral" by QnRA
// https://www.shadertoy.com/view/Nc2SRd
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized SDF primitive choice, exposed camera/spiral knobs,
//           gradient LUT replaces IQ palette, FFT brightness on shared t index
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float flyPhase;
uniform float cellPhase;

uniform int shapeType;
uniform float shapeSize;
uniform float cellPitchZ;

uniform int marchSteps;
uniform float stepFactor;
uniform float maxDist;
uniform float spiralRate;
uniform float tColorDistScale;
uniform float tColorIterScale;

uniform float fov;
uniform float pitchAngle;
uniform float rollAngle;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

mat2 rot2D(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

float sdfMap(vec3 p, float s, int shape) {
    if (shape == 0) {
        // Octahedron (bound form)
        vec3 q = abs(p);
        return (q.x + q.y + q.z - s) * 0.57735027;
    } else if (shape == 1) {
        // Box
        vec3 q = abs(p) - vec3(s);
        return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
    } else if (shape == 2) {
        // Sphere
        return length(p) - s;
    } else {
        // Torus (major = s, minor = s * 0.4)
        vec2 q = vec2(length(p.xz) - s, p.y);
        return length(q) - s * 0.4;
    }
}

float map(vec3 p) {
    vec3 ohPos = p;
    ohPos.z += flyPhase;
    ohPos.xy = fract(ohPos.xy) - 0.5;
    ohPos.z = mod(ohPos.z, cellPitchZ) - cellPitchZ * 0.5;
    ohPos.xy *= rot2D(cellPhase);
    ohPos.yz *= rot2D(cellPhase);
    return sdfMap(ohPos, shapeSize, shapeType);
}

void main() {
    // Centered coords: (0,0) at screen center, y in [-1, 1].
    // Equivalent to the reference's `(fragCoord*2 - iResolution.xy)/iResolution.y`,
    // adapted to raylib's normalized fragTexCoord (Shadertoy fragCoord doesn't exist here).
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;

    vec3 ro = vec3(0.0, 0.0, -1.0);
    vec3 rd = normalize(vec3(uv * fov, 1.0));

    ro.yz *= rot2D(pitchAngle);
    rd.yz *= rot2D(pitchAngle);
    ro.yz *= rot2D(rollAngle);
    rd.yz *= rot2D(rollAngle);

    float dt = 0.0;
    int i;
    for (i = 0; i < marchSteps; i++) {
        vec3 p = ro + rd * dt;
        // Spiral the sample point as it marches -- the signature mechanic
        p.xy *= rot2D(dt * spiralRate);
        float d = map(p);
        if (d < 0.001 || dt > maxDist) {
            break;
        }
        dt += d * stepFactor;
    }

    // Shared color/audio index: same t drives gradient LUT and FFT lookup
    float t = fract(dt * tColorDistScale + float(i) * tColorIterScale);
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) {
        energy = texture(fftTexture, vec2(bin, 0.5)).r;
    }
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    finalColor = vec4(color * brightness, 1.0);
}
```

Key invariants enforced by this code:
- Centered UV (the `(fragTexCoord * 2.0 - 1.0)` mapping puts (0,0) at screen center; matches the reference's `(fragCoord * 2.0 - iResolution.xy) / iResolution.y`).
- Three preserved-verbatim mechanics: `fract`/`mod` domain repetition, two-axis per-cell rotation, `p.xy *= rot2D(dt * spiralRate)` ray spiral.
- Hit epsilon `0.001` is a numerical-convergence constant, not a knob.
- No tonemap. Output is `color * brightness` in [0, ~1.15]; gradient LUT outputs are already in [0, 1] and brightness peak is `baseBright + 1.0`.

#### CPU-side phase accumulation (in `SpiralMarchEffectSetup`)

```
e->flyPhase  += cfg->flySpeed * deltaTime
e->cellPhase += cfg->cellRotateSpeed * deltaTime
ColorLUTUpdate(e->gradientLUT, &cfg->gradient)
SetShaderValue(... resolution, fftTexture, sampleRate ...)
SetShaderValue(flyPhase, cellPhase, all config uniforms)
SetShaderValueTexture(gradientLUT, ColorLUTGetTexture(e->gradientLUT))
```

No DualLissajous, no pan uniform. Setup signature mirrors `VoxelMarchEffectSetup` minus the lissajous block.

---

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `enabled` | bool | -- | false | no | (toggle from dispatch) |
| `shapeType` | int (combo) | 0-3 | 0 (Octahedron) | no | "Shape" — `Octahedron\0Box\0Sphere\0Torus\0` |
| `shapeSize` | float | 0.05-0.45 | 0.25 | yes | "Shape Size" |
| `cellPitchZ` | float | 0.05-1.0 | 0.20 | yes | "Cell Pitch Z" |
| `marchSteps` | int | 30-120 | 80 | no (`SliderInt`) | "March Steps" |
| `stepFactor` | float | 0.5-1.0 | 0.9 | yes | "Step Factor" |
| `maxDist` | float | 20-200 | 100 | yes | "Max Dist" |
| `spiralRate` | float | 0.0-3.0 | 0.8 | yes | "Spiral Rate" |
| `tColorDistScale` | float | 0.0-0.1 | 0.012 | yes (`Log`) | "Color Dist Scale" |
| `tColorIterScale` | float | 0.0-0.05 | 0.005 | yes (`Log`) | "Color Iter Scale" |
| `flySpeed` | float | 0.0-5.0 | 0.20 | yes | "Fly Speed" |
| `cellRotateSpeed` | float (speed) | 0 to ROTATION_SPEED_MAX | 1.0 | yes (`SpeedDeg`) | "Cell Rotate Speed" |
| `fov` | float | 0.05-1.0 | 0.15 | yes | "FOV" |
| `pitchAngle` | float (angle) | -ROTATION_OFFSET_MAX to +ROTATION_OFFSET_MAX | 0.53 | yes (`AngleDeg`) | "Pitch" |
| `rollAngle` | float (angle) | -ROTATION_OFFSET_MAX to +ROTATION_OFFSET_MAX | -0.531 | yes (`AngleDeg`) | "Roll" |
| `baseFreq` | float | 27.5-440 | 55.0 | yes | "Base Freq (Hz)" |
| `maxFreq` | float | 1000-16000 | 14000 | yes | "Max Freq (Hz)" |
| `gain` | float | 0.1-10 | 2.0 | yes | "Gain" |
| `curve` | float | 0.1-3.0 | 1.5 | yes | "Contrast" |
| `baseBright` | float | 0.0-1.0 | 0.15 | yes | "Base Bright" |
| `gradient` | ColorConfig | -- | default gradient | -- | (rendered by `STANDARD_GENERATOR_OUTPUT`) |
| `blendMode` | enum | -- | EFFECT_BLEND_SCREEN | -- | (rendered by `STANDARD_GENERATOR_OUTPUT`) |
| `blendIntensity` | float | 0.0-5.0 | 1.0 | yes | "Blend Intensity" (rendered by `STANDARD_GENERATOR_OUTPUT`) |

UI section ordering (per `/ui-guide`):
1. **Audio** — `baseFreq`, `maxFreq`, `gain`, `curve` (label "Contrast"), `baseBright`
2. **Geometry** — `Shape` combo, `shapeSize`, `cellPitchZ`
3. **Raymarching** — `marchSteps` (`SliderInt`), `stepFactor`, `maxDist`, `spiralRate`, `tColorDistScale` (`Log`), `tColorIterScale` (`Log`)
4. **Animation** — `flySpeed`, `cellRotateSpeed` (`SpeedDeg`)
5. **Camera** — `fov`, `pitchAngle` (`AngleDeg`), `rollAngle` (`AngleDeg`)
6. **(STANDARD_GENERATOR_OUTPUT macro)** — Color (gradient widget), then Output (`blendIntensity`, `blendMode` combo)

`ModEngineRegisterParam` calls map to all `yes` rows above with the listed ranges.

---

### Constants

- Enum value: `TRANSFORM_SPIRAL_MARCH_BLEND` (placed at end of `TransformEffectType` before `TRANSFORM_ACCUM_COMPOSITE`)
- Display name: `"Spiral March"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Category section index: `13` (Field, matching Voxel March / Cyber March / Geode / Random Volumetric)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)
- Param prefix: `"spiralMarch."` (auto-set from field name)
- Field name on `EffectConfig` and `PostEffect`: `spiralMarch`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create config + effect header

**Files**: `src/effects/spiral_march.h`
**Creates**: `SpiralMarchConfig`, `SpiralMarchEffect`, public function declarations, `SPIRAL_MARCH_CONFIG_FIELDS` macro

**Do**: Mirror `src/effects/voxel_march.h`'s structure exactly, minus the `DualLissajousConfig` member and its include — Spiral March has no screen-space pan. Use the struct layouts from the Design > Types section verbatim. Header must include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `ColorLUT`. Field-list macro must list every config field including nested `gradient`, `blendMode`, `blendIntensity`.

**Verify**: `cmake.exe --build build` compiles (this header is included nowhere yet, so success means the header itself parses).

---

### Wave 2: Implementation (parallel — no file overlap)

#### Task 2.1: Create effect implementation file

**Files**: `src/effects/spiral_march.cpp`
**Depends on**: Wave 1 (Task 1.1)

**Do**: Mirror `src/effects/voxel_march.cpp` exactly, with these specific differences:
- Drop the `DualLissajous` block (no `pan` uniform, no `DualLissajousUpdate` call, no `panLoc`).
- Drop the `colorFreqMap`, `surfaceShape`, `domainFold`, `boundaryWave`, `surfaceCount`, `voxelScale`, `voxelVariation`, `cellSize`, `shellRadius`, `highlightIntensity`, `positionTint`, `tonemapGain`, `gridAnimSpeed` uniform plumbing — Spiral March doesn't use any of them.
- Add the spiral-specific uniform plumbing per the `SpiralMarchEffect` struct: `flyPhase`, `cellPhase`, `shapeType`, `shapeSize`, `cellPitchZ`, `marchSteps`, `stepFactor`, `maxDist`, `spiralRate`, `tColorDistScale`, `tColorIterScale`, `fov`, `pitchAngle`, `rollAngle`, plus the standard FFT block (`baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`).
- `SpiralMarchEffectSetup` accumulates two phases on CPU: `e->flyPhase += cfg->flySpeed * deltaTime` and `e->cellPhase += cfg->cellRotateSpeed * deltaTime`. No GLSL-side `iTime` accumulation. (Memory rule: speed is always accumulated on CPU.)
- `SpiralMarchRegisterParams` registers every "yes"-modulatable param from the Parameters table with the listed ranges.
- UI section in the file (`// === UI ===`) follows the canonical order Audio / Geometry / Raymarching / Animation / Camera. Use widget choices per the Parameters table column. Audio block follows the convention from `/ui-guide` exactly (label spelling and order). Use `ImGui::Combo("Shape##spiralMarch", &cfg->shapeType, "Octahedron\0Box\0Sphere\0Torus\0")` for the shape selector.
- TWO non-static bridge functions at the bottom (memory rule: bridge functions are NOT static; the colocated `DrawSpiralMarchParams` IS static):
  - `void SetupSpiralMarch(PostEffect *pe)` — calls `SpiralMarchEffectSetup(&pe->spiralMarch, &pe->effects.spiralMarch, pe->currentDeltaTime, pe->fftTexture)`
  - `void SetupSpiralMarchBlend(PostEffect *pe)` — calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.spiralMarch.blendIntensity, pe->effects.spiralMarch.blendMode)`
- `STANDARD_GENERATOR_OUTPUT(spiralMarch)` immediately before the registration macro.
- `REGISTER_GENERATOR(TRANSFORM_SPIRAL_MARCH_BLEND, SpiralMarch, spiralMarch, "Spiral March", SetupSpiralMarchBlend, SetupSpiralMarch, 13, DrawSpiralMarchParams, DrawOutput_spiralMarch)`.
- Wrap the macro line in `// clang-format off` / `// clang-format on`.

Includes (clang-format will sort within groups):
- Own header: `"spiral_march.h"`
- Project: `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`
- ImGui/UI: `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`
- System: `<stddef.h>`

Implement the Algorithm section — the `Setup` function reads from cfg/Effect and pushes uniforms in the same order as Voxel March's `BindUniforms` for consistency.

**Verify**: `cmake.exe --build build` succeeds. The effect's source compiles standalone (it will not yet be linked into the descriptor table because Wave 2.3's enum entry is needed; but the .cpp itself is self-contained at compile time since the `REGISTER_GENERATOR` macro only depends on `effect_descriptor.h` types).

---

#### Task 2.2: Create fragment shader

**Files**: `shaders/spiral_march.fs`
**Depends on**: Wave 1 (Task 1.1) only insofar as uniform names must match — but the .h is text only.

**Do**: Implement the shader exactly as written in the Design > Algorithm > Fragment shader section. Begin with the four-line attribution comment block (Based / URL / License / Modified) per the add-effect skill rule for derived shaders. Do not add any uniforms not listed in the Algorithm code. Do not add a tonemap. Do not split rays into separate origin offsets — keep `ro = vec3(0,0,-1)`. Preserve the three signature mechanics verbatim:
- `ohPos.xy = fract(ohPos.xy) - 0.5; ohPos.z = mod(ohPos.z, cellPitchZ) - cellPitchZ * 0.5;`
- `ohPos.xy *= rot2D(cellPhase); ohPos.yz *= rot2D(cellPhase);`
- `p.xy *= rot2D(dt * spiralRate);` inside the march loop

**Verify**: `cmake.exe --build build` succeeds (shader is loaded at runtime — build verifies file presence indirectly through resource embedding if applicable, but does not compile GLSL).

---

#### Task 2.3: Wire into EffectConfig and transform enum

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 (Task 1.1)

**Do**: Three changes:
1. Add `#include "effects/spiral_march.h"` in the alphabetized include block (between `spectral_rings.h` and `spin_cage.h` per alphabetical order — clang-format will fix on save).
2. Add `TRANSFORM_SPIRAL_MARCH_BLEND,` to the `TransformEffectType` enum, placed immediately before `TRANSFORM_ACCUM_COMPOSITE` and `TRANSFORM_EFFECT_COUNT`.
3. Add `SpiralMarchConfig spiralMarch;` member to `EffectConfig` (place near the other "March" generators, e.g. near `VoxelMarchConfig voxelMarch;`). Add a one-line `// Spiral March (...)` comment above it consistent with the surrounding style.

No need to touch `TransformOrderConfig::order[]` initialization — it's auto-populated by the constructor that fills indices in enum order.

**Verify**: `cmake.exe --build build` succeeds. The new enum value parses; the new member compiles because the header was added in Wave 1.

---

#### Task 2.4: Wire into PostEffect

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 (Task 1.1)

**Do**: Two changes:
1. Add `#include "effects/spiral_march.h"` in the alphabetized include block.
2. Add `SpiralMarchEffect spiralMarch;` runtime-state member to the `PostEffect` struct, placed near other generator effect members (e.g. near `VoxelMarchEffect voxelMarch;`).

No changes to `post_effect.cpp` — the descriptor table loop handles init/uninit/registerParams automatically.

**Verify**: `cmake.exe --build build` succeeds.

---

#### Task 2.5: Wire into preset serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 (Task 1.1)

**Do**: Three changes:
1. Add `#include "effects/spiral_march.h"` in the alphabetized include block (so `SPIRAL_MARCH_CONFIG_FIELDS` is visible).
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpiralMarchConfig, SPIRAL_MARCH_CONFIG_FIELDS)` alongside the other per-config macros (S section, near `SpiralNestConfig` / `SpiralWalkConfig`).
3. Add `X(spiralMarch)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table (single entry handles both serialize and deserialize).

**Verify**: `cmake.exe --build build` succeeds. Save a preset with Spiral March enabled, reload — settings preserved (manual test after build).

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds with no warnings
- [ ] App launches; "Spiral March" appears in the Generators > Field section of the Effects window
- [ ] Category badge reads "GEN" (not "???")
- [ ] Enabling the effect produces a tumbling-lattice spiraling tunnel reminiscent of the reference shader (octahedron default)
- [ ] Shape combo cycles through Octahedron / Box / Sphere / Torus and each renders distinct geometry
- [ ] `spiralRate` slider visibly changes the helical curl: at 0 the tunnel is straight, at 0.8 it matches the reference look, at 3.0 produces tight vertigo
- [ ] FFT reactivity: with audio playing, near cells (small `dt`) brighten on bass content, far cells (large `dt`) brighten on treble content — confirm by toggling music on/off
- [ ] Pitch and Roll sliders display in degrees, store in radians, range +/- 180 deg
- [ ] Cell Rotate Speed slider displays in deg/s
- [ ] Preset save/load round-trips all knobs (toggle a few, save, edit defaults, reload preset)
- [ ] Modulation routes onto every "yes"-modulatable param (open mod engine, route LFO to spiralRate; verify visible motion)
- [ ] Effect can be reordered in the transform pipeline via drag-drop
- [ ] No tonemap was added (output is `gradient * brightness`, brightness peaks at `baseBright + 1.0`)
