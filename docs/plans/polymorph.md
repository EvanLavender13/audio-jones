# Polymorph

SDF ray-marched glowing wireframe polyhedron that morphs between platonic solids. CPU computes the morph state machine, vertex positions, and edge capsule data; shader receives edge uniforms and performs ray marching with glow accumulation. Follows the SpinCage generator pattern (CPU geometry + shader rendering + blend compositor).

**Research**: `docs/research/polymorph.md`

## Design

### Types

**PolymorphConfig** (in `polymorph.h`):
```
struct PolymorphConfig {
  bool enabled = false;

  // Shape
  int baseShape = 4;        // Platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa
  float randomness = 0.0f;  // Shape selection chaos (0.0-1.0)
  float freeform = 0.0f;    // Vertex perturbation (0.0-1.0)

  // Morph
  float morphSpeed = 0.5f;  // Morph cycles per second (0.1-5.0)
  float holdRatio = 0.4f;   // Fraction of cycle showing full wireframe (0.0-1.0)

  // Camera
  float orbitSpeed = 0.5f;     // Camera orbit rate rad/s (-PI..+PI)
  float cameraHeight = 5.0f;   // Camera Y offset (0.0-15.0)
  float cameraDistance = 25.0f; // Camera orbit radius (5.0-50.0)

  // Geometry
  float scale = 15.0f;          // Shape size (1.0-30.0)
  float edgeThickness = 0.02f;  // Capsule radius (0.005-0.1)
  float glowIntensity = 0.2f;   // Glow brightness multiplier (0.05-1.0)
  float fov = 1.8f;             // Field of view factor (1.0-4.0)

  // Audio
  float baseFreq = 55.0f;    // (27.5-440.0)
  float maxFreq = 14000.0f;  // (1000-16000)
  float gain = 2.0f;         // (0.1-10.0)
  float curve = 1.5f;        // (0.1-3.0)
  float baseBright = 0.15f;  // (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

**PolymorphEffect** (in `polymorph.h`):
```
typedef struct PolymorphEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU morph state
  float morphPhase;      // 0..1 within current cycle
  int currentShape;      // Index into SHAPES[] (0-4)
  uint32_t rngState;     // PRNG state for shape selection and freeform

  // CPU-accumulated accumulators
  float orbitAccum;

  // Per-edge data computed each frame (max 30 edges)
  float edgeAx[30], edgeAy[30], edgeAz[30];
  float edgeBx[30], edgeBy[30], edgeBz[30];
  float edgeT[30];
  int edgeCount;

  // Freeform vertex offsets (regenerated each cycle)
  float vertexOffsetX[20], vertexOffsetY[20], vertexOffsetZ[20];

  // Uniform locations
  int resolutionLoc;
  int edgeALoc, edgeBLoc;
  int edgeTLoc;
  int edgeCountLoc;
  int edgeThicknessLoc;
  int glowIntensityLoc;
  int cameraOriginLoc;
  int cameraFovLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} PolymorphEffect;
```

### Algorithm

#### CPU Morph State Machine

`morphPhase` advances by `morphSpeed * deltaTime` each frame (wraps at 1.0).

Phase regions within one cycle (morphPhase 0.0→1.0):
- **EXPANDING** [0.0, expandEnd): `slideT = morphPhase / expandEnd`. Each edge grows from a point at endpoint A to full length A→B.
- **HOLDING** [expandEnd, collapseStart): Full wireframe. slideT = 1.0.
- **COLLAPSING** [collapseStart, 1.0): `slideT = 1.0 - (morphPhase - collapseStart) / (1.0 - collapseStart)`. Edge shrinks from opposite end (B catches up to A).
- **SWITCH** at wrap (morphPhase crosses 1.0): Pick next shape, regenerate freeform offsets.

Where: `expandEnd = (1.0 - holdRatio) / 2.0`, `collapseStart = expandEnd + holdRatio`.

#### Per-Edge Capsule Computation

Each frame, for the current shape from `platonic_solids.h`:

1. Get the `ShapeDescriptor` for `currentShape`.
2. For each vertex, compute perturbed position: `v' = normalize(v + freeform * offset) * scale`. The offset vector is from `vertexOffsetX/Y/Z[]`, regenerated each SWITCH via the PRNG. Vertices beyond the shape's count are unused.
3. For each edge `[i, j]` in the shape's edge list, compute animated capsule endpoints:
   - During EXPANDING: `A = v'[j] + slideT * (v'[i] - v'[j])`, `B = v'[j]`. Edge grows from point at j toward i.
   - During HOLDING: `A = v'[i]`, `B = v'[j]`. Full edge.
   - During COLLAPSING: `A = v'[i]`, `B = v'[i] + slideT * (v'[j] - v'[i])`. Edge shrinks as B slides from v'[j] toward v'[i] while slideT goes 1→0.
4. Set `edgeCount` to shape's edge count.
5. Compute `edgeT[e] = (float)e / (float)(edgeCount - 1)` for gradient color mapping.

#### Shape Selection (at SWITCH)

```
float roll = prng_float(&rngState);  // [0, 1)
if (roll < randomness) {
    currentShape = prng_int(&rngState) % 5;
} else {
    currentShape = baseShape;
}
```

Simple xorshift32 PRNG seeded at init from a counter or time.

#### Freeform Vertex Offsets (at SWITCH)

For each vertex 0..19:
```
vertexOffsetX[v] = prng_float_signed(&rngState);  // [-1, 1]
vertexOffsetY[v] = prng_float_signed(&rngState);
vertexOffsetZ[v] = prng_float_signed(&rngState);
```

Only the first `vertexCount` offsets matter for the current shape; the rest are ignored.

#### Camera

```
orbitAccum += orbitSpeed * deltaTime;
cameraOrigin = vec3(cameraDistance * sin(orbitAccum), cameraHeight, cameraDistance * cos(orbitAccum));
```

Passed as `uniform vec3 cameraOrigin`.

#### Shader

```glsl
// Based on "Arknights：Polyhedrons" by yli110
// https://www.shadertoy.com/view/sc2GzW
// License: CC BY-NC-SA 3.0 Unported
// Modified: All 5 platonic solids, CPU-side morph state machine, explicit edge
// capsules as uniforms, gradient LUT coloring, FFT reactivity

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec3 edgeA[30];
uniform vec3 edgeB[30];
uniform float edgeT[30];
uniform int edgeCount;
uniform float edgeThickness;
uniform float glowIntensity;
uniform vec3 cameraOrigin;
uniform float cameraFov;

uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define PIXEL (3.0 / min(resolution.x, resolution.y))

// sdCapsule — verbatim from reference
float sdCapsule(vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h) - r;
}

void main() {
    vec2 p = (2.0 * gl_FragCoord.xy - resolution) / min(resolution.x, resolution.y);

    // Camera setup — orbit camera from reference
    vec3 ta = vec3(0.0);
    vec3 ro = cameraOrigin;
    vec3 ww = normalize(ta - ro);
    vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
    vec3 vv = normalize(cross(uu, ww));
    vec3 rd = normalize(p.x * uu + p.y * vv + cameraFov * ww);

    // Ray march with per-edge color glow accumulation
    // <!-- Intentional deviation: research accumulates scalar glow; plan does
    //      per-edge color + FFT inside the loop for gradient-colored edges -->
    float t = 0.0;
    vec3 accumColor = vec3(0.0);

    for (int step = 0; step < 80; step++) {
        vec3 pos = ro + t * rd;

        // Find closest edge and minimum distance
        float minDist = 1e10;
        int closest = 0;
        for (int e = 0; e < edgeCount; e++) {
            float d = sdCapsule(pos, edgeA[e], edgeB[e], edgeThickness);
            if (d < minDist) {
                minDist = d;
                closest = e;
            }
        }

        // Glow contribution weighted by closest edge's color
        float g = PIXEL / (0.001 + minDist * minDist);
        vec3 color = texture(gradientLUT, vec2(edgeT[closest], 0.5)).rgb;

        // FFT brightness for the closest edge
        float freqRatio = maxFreq / baseFreq;
        float et = edgeT[closest];
        float bandW = 1.0 / 48.0;
        float ft0 = max(et - bandW * 0.5, 0.0);
        float ft1 = min(et + bandW * 0.5, 1.0);
        float freqLo = baseFreq * pow(freqRatio, ft0);
        float freqHi = baseFreq * pow(freqRatio, ft1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float fftEnergy = 0.0;
        for (int s = 0; s < 4; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / 4.0);
            if (bin <= 1.0) {
                fftEnergy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float brightness = baseBright + pow(clamp(fftEnergy / 4.0 * gain, 0.0, 1.0), curve);

        accumColor += color * g * glowIntensity * brightness;

        t += minDist;
        if (t > 100.0) break;
    }

    // Gamma + tonemap from reference
    accumColor = pow(accumColor, vec3(0.4545));
    finalColor = vec4(tanh(accumColor), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseShape | int | 0-4 | 4 | No | Shape |
| randomness | float | 0.0-1.0 | 0.0 | Yes | Randomness |
| freeform | float | 0.0-1.0 | 0.0 | Yes | Freeform |
| morphSpeed | float | 0.1-5.0 | 0.5 | Yes | Morph Speed |
| holdRatio | float | 0.0-1.0 | 0.4 | Yes | Hold |
| orbitSpeed | float | -PI..+PI | 0.5 | Yes | Orbit Speed |
| cameraHeight | float | 0.0-15.0 | 5.0 | Yes | Camera Height |
| cameraDistance | float | 5.0-50.0 | 25.0 | Yes | Camera Dist |
| scale | float | 1.0-30.0 | 15.0 | Yes | Scale |
| edgeThickness | float | 0.005-0.1 | 0.02 | Yes | Edge Thickness |
| glowIntensity | float | 0.05-1.0 | 0.2 | Yes | Glow |
| fov | float | 1.0-4.0 | 1.8 | Yes | FOV |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |

### Constants

- Enum name: `TRANSFORM_POLYMORPH_BLEND`
- Display name: `"Polymorph"`
- Category badge: `"GEN"` (auto-set by REGISTER_GENERATOR)
- Section index: `10` (Geometric — same category as Spin Cage)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by REGISTER_GENERATOR)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create polymorph.h

**Files**: `src/effects/polymorph.h` (create)
**Creates**: `PolymorphConfig`, `PolymorphEffect` structs, `POLYMORPH_CONFIG_FIELDS` macro, function declarations

**Do**: Create header following SpinCage header pattern (`spin_cage.h`). Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`, `<stdint.h>`.

Define `PolymorphConfig` and `PolymorphEffect` exactly as specified in the Design section. The Config fields macro lists all serializable fields. Function declarations:
- `bool PolymorphEffectInit(PolymorphEffect *e, const PolymorphConfig *cfg);`
- `void PolymorphEffectSetup(PolymorphEffect *e, const PolymorphConfig *cfg, float deltaTime, Texture2D fftTexture);`
- `void PolymorphEffectUninit(PolymorphEffect *e);`
- `PolymorphConfig PolymorphConfigDefault(void);`
- `void PolymorphRegisterParams(PolymorphConfig *cfg);`

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create polymorph.cpp

**Files**: `src/effects/polymorph.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the effect module following SpinCage `.cpp` pattern. Includes same as SpinCage plus `"config/platonic_solids.h"`, `"audio/audio.h"`, `<math.h>`.

**Init**: Load `shaders/polymorph.fs`, cache all uniform locations listed in `PolymorphEffect`, init `gradientLUT` via `ColorLUTInit(&cfg->gradient)`. Set `morphPhase = 0`, `currentShape = cfg->baseShape`, `orbitAccum = 0`, `rngState` to a non-zero seed (e.g., `0xDEADBEEF`). Zero out `vertexOffset` arrays.

**Simple xorshift32 PRNG** (file-local static helpers):
```c
static uint32_t Xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}
static float PrngFloat(uint32_t *state) {
    return (float)(Xorshift32(state) & 0xFFFFFF) / (float)0xFFFFFF;
}
static float PrngFloatSigned(uint32_t *state) {
    return PrngFloat(state) * 2.0f - 1.0f;
}
```

**Setup**: Implement the morph state machine and per-edge capsule computation exactly as described in the Algorithm section. Key steps:
1. Advance `morphPhase += cfg->morphSpeed * deltaTime`. If wraps past 1.0: run shape selection and regenerate freeform offsets (Algorithm: Shape Selection and Freeform Vertex Offsets).
2. Compute `slideT` from `morphPhase` using the phase regions (expandEnd, collapseStart).
3. Get `ShapeDescriptor` from `SHAPES[currentShape]`.
4. For each vertex, compute perturbed position: `normalize(v + freeform * offset) * scale`.
5. For each edge, compute animated capsule endpoints per the Algorithm (EXPANDING / HOLDING / COLLAPSING formulas).
6. Advance `orbitAccum += cfg->orbitSpeed * deltaTime`.
7. Compute `cameraOrigin` from `orbitAccum`, `cameraDistance`, `cameraHeight`.
8. Update gradient LUT, upload all uniforms.

**Uniform upload**: Pass `edgeA` as 30 vec3 via `SetShaderValueV` with `SHADER_UNIFORM_VEC3`. Same for `edgeB`. Pass `edgeT` as 30 floats. Pass `cameraOrigin` as vec3. Pass scalars as floats. Pattern matches SpinCage's `UploadUniforms`.

**Uninit**: `UnloadShader`, `ColorLUTUninit`.

**RegisterParams**: Register all modulatable params from the Parameters table with `ModEngineRegisterParam`.

**ConfigDefault**: `return PolymorphConfig{};`

**UI section** (`// === UI ===`): `static void DrawPolymorphParams(EffectConfig*, const ModSources*, ImU32)`. Sections:
- **Shape**: `ImGui::Combo` for baseShape (same 5-entry list as SpinCage), `ModulatableSlider` for randomness, `ModulatableSlider` for freeform.
- **Morph**: `ModulatableSlider` for morphSpeed, holdRatio.
- **Camera**: `ModulatableSliderSpeedDeg` for orbitSpeed, `ModulatableSlider` for cameraHeight, cameraDistance, fov.
- **Geometry**: `ModulatableSliderLog` for edgeThickness, `ModulatableSlider` for scale, glowIntensity.
- **Audio**: Standard order — Base Freq, Max Freq, Gain, Contrast, Base Bright (matching conventions).

**Bridge functions**: Non-static `SetupPolymorph(PostEffect* pe)` and `SetupPolymorphBlend(PostEffect* pe)` — same pattern as SpinCage.

**Registration**: `STANDARD_GENERATOR_OUTPUT(polymorph)` then `REGISTER_GENERATOR(TRANSFORM_POLYMORPH_BLEND, Polymorph, polymorph, "Polymorph", SetupPolymorphBlend, SetupPolymorph, 10, DrawPolymorphParams, DrawOutput_polymorph)`.

**Verify**: Compiles.

---

#### Task 2.2: Create polymorph.fs shader

**Files**: `shaders/polymorph.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the shader exactly as specified in the Algorithm section's Shader subsection. The complete GLSL is provided there — transcribe it verbatim. The attribution comment block is included in the Algorithm section and must appear before `#version 330`.

**Verify**: File exists, no syntax issues at compile time.

---

#### Task 2.3: Integration — effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/polymorph.h"` in alphabetical position among other effect includes.
2. Add `TRANSFORM_POLYMORPH_BLEND,` to the `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE` (keeping it with other generators near end of list, after `TRANSFORM_SYNAPSE_TREE_BLEND`).
3. Add `PolymorphConfig polymorph;` to `EffectConfig` struct with comment `// Polymorph (SDF ray-marched morphing platonic solid wireframes)`. Place after `SpinCageConfig spinCage;`.

**Verify**: Compiles.

---

#### Task 2.4: Integration — post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/polymorph.h"` in alphabetical position.
2. Add `PolymorphEffect polymorph;` to `PostEffect` struct after `SpinCageEffect spinCage;`.

**Verify**: Compiles.

---

#### Task 2.5: Integration — effect_serialization.cpp

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/polymorph.h"` in alphabetical position among effect includes.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PolymorphConfig, POLYMORPH_CONFIG_FIELDS)` in the appropriate alphabetical section (after the `P*` configs).
3. Add `X(polymorph) \` to the `EFFECT_CONFIG_FIELDS(X)` macro, after `X(spinCage)`.

**Verify**: Compiles.

---

#### Task 2.6: Integration — CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/polymorph.cpp` to `EFFECTS_SOURCES`, after `src/effects/spin_cage.cpp`.

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Polymorph appears in Geometric generator section
- [ ] Shape combo selects all 5 platonic solids
- [ ] Randomness 0 → same shape breathes, Randomness 1 → shape changes each cycle
- [ ] Freeform slider warps vertex positions
- [ ] Morph Speed controls cycle rate
- [ ] Camera orbits around shape
- [ ] FFT audio drives edge brightness
- [ ] Gradient LUT colors edges
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes work for registered parameters
