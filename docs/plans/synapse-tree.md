# Synapse Tree

Raymarched 3D fractal tree generator built from iterated sphere inversion and abs-fold branching. Organic limbs twist and split while electric synapse pulses travel along branch tips. Camera drifts forward into the canopy. Each fractal fold depth responds to a different FFT frequency band — bass illuminates the trunk, treble lights the finest twigs.

**Research**: `docs/research/synapse-tree.md`

## Design

### Types

**SynapseTreeConfig** (user-facing parameters):

```cpp
struct SynapseTreeConfig {
  bool enabled = false;

  // Raymarching
  int marchSteps = 120;            // Ray budget (40-200)
  int foldIterations = 8;          // Branching depth (4-12)
  float fov = 1.8f;                // Field of view multiplier (1.0-3.0)

  // Fractal shape
  float foldOffset = 0.5f;         // Abs-fold translation (0.2-1.0)
  float yFold = 1.78f;             // Vertical fold height (1.0-3.0)
  float branchThickness = 0.06f;   // Cylinder radius (0.01-0.2)

  // Animation
  float animSpeed = 0.4f;          // Fractal morph rate (0.0-2.0)
  float driftSpeed = 0.1f;         // Forward camera drift (0.0-1.0)

  // Synapse
  float synapseIntensity = 0.5f;   // Pulse brightness (0.0-2.0)
  float synapseBounceFreq = 11.0f; // Vertical bounce speed (1.0-20.0)
  float synapsePulseFreq = 7.0f;   // Size oscillation speed (1.0-15.0)

  // Trail persistence
  float decayHalfLife = 2.0f;      // Trail persistence seconds (0.1-10.0)
  float trailBlur = 0.5f;          // Trail blur amount (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;          // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f;        // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;               // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;              // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f;        // Min brightness floor (0.0-1.0)

  // Color
  float colorSpeed = 0.3f;         // LUT scroll rate (0.0-2.0)
  float colorStretch = 1.0f;       // Spatial color frequency (0.1-5.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Tonemap
  float brightness = 1.0f;         // Pre-tonemap multiplier (0.1-5.0)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;     // (0.0-5.0)
};
```

Header includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`

**SynapseTreeEffect** (runtime state):

```cpp
typedef struct SynapseTreeEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFFTTexture;
  float animPhase;       // CPU-accumulated anim time
  float camZ;            // CPU-accumulated camera Z offset
  float colorPhase;      // CPU-accumulated color scroll
  // Uniform locations — one int per shader uniform
  int resolutionLoc;
  int animPhaseLoc;
  int marchStepsLoc;
  int foldIterationsLoc;
  int fovLoc;
  int foldOffsetLoc;
  int yFoldLoc;
  int branchThicknessLoc;
  int camZLoc;
  int synapseIntensityLoc;
  int synapseBounceFreqLoc;
  int synapsePulseFreqLoc;
  int colorPhaseLoc;
  int colorStretchLoc;
  int brightnessLoc;
  int gradientLUTLoc;
  int previousFrameLoc;
  int decayFactorLoc;
  int trailBlurLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} SynapseTreeEffect;
```

**CONFIG_FIELDS macro:**

```cpp
#define SYNAPSE_TREE_CONFIG_FIELDS                                             \
  enabled, marchSteps, foldIterations, fov, foldOffset, yFold,                \
      branchThickness, animSpeed, driftSpeed, synapseIntensity,               \
      synapseBounceFreq, synapsePulseFreq, decayHalfLife, trailBlur,          \
      baseFreq, maxFreq, gain, curve, baseBright, colorSpeed, colorStretch,   \
      gradient, brightness, blendMode, blendIntensity
```

**Public functions** (same pattern as Vortex):

```
bool SynapseTreeEffectInit(SynapseTreeEffect *e, const SynapseTreeConfig *cfg, int width, int height);
void SynapseTreeEffectSetup(SynapseTreeEffect *e, const SynapseTreeConfig *cfg, float deltaTime, Texture2D fftTexture);
void SynapseTreeEffectRender(SynapseTreeEffect *e, const SynapseTreeConfig *cfg, float deltaTime, int screenWidth, int screenHeight);
void SynapseTreeEffectResize(SynapseTreeEffect *e, int width, int height);
void SynapseTreeEffectUninit(SynapseTreeEffect *e);
SynapseTreeConfig SynapseTreeConfigDefault(void);
void SynapseTreeRegisterParams(SynapseTreeConfig *cfg);
```

### Algorithm

The shader is a direct port of the reference with parameterized uniforms, gradient LUT coloring, FFT-per-fold-depth audio reactivity, and a trail buffer.

#### Uniforms

```glsl
uniform vec2 resolution;
uniform float animPhase;        // CPU-accumulated: phase += animSpeed * deltaTime
uniform int marchSteps;
uniform int foldIterations;
uniform float fov;
uniform float foldOffset;
uniform float yFold;
uniform float branchThickness;
uniform float camZ;             // CPU-accumulated: camZ += driftSpeed * deltaTime, fmod 100
uniform float synapseIntensity;
uniform float synapseBounceFreq;
uniform float synapsePulseFreq;
uniform float colorPhase;
uniform float colorStretch;
uniform float brightness;
uniform sampler2D gradientLUT;
uniform sampler2D previousFrame;
uniform float decayFactor;
uniform float trailBlur;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
```

#### Rotation macro

The reference uses `mat2(cos(a + vec4(0, 11, 33, 0)))`. Port verbatim:

```glsl
mat2 rot(float a) {
    vec4 v = cos(a + vec4(0.0, 11.0, 33.0, 0.0));
    return mat2(v.x, v.y, v.z, v.w);
}
```

#### FFT precomputation

<!-- Intentional deviation: uses binLo/binHi band-range sampling (codebase standard from vortex.fs/generator_patterns) rather than research's abbreviated single-point code -->
Before the ray march loop, precompute FFT energy per fold depth into a fixed-size array. This runs once per pixel:

```glsl
const int MAX_FOLDS = 12;
float fftBands[MAX_FOLDS];
for (int k = 0; k < MAX_FOLDS; k++) {
    if (k >= foldIterations) { fftBands[k] = 0.0; continue; }
    float tFreq = float(k) / float(max(foldIterations - 1, 1));
    float freqLo = baseFreq * pow(maxFreq / baseFreq, tFreq);
    float tNext = float(k + 1) / float(foldIterations);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, tNext);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int bs = 0; bs < BAND_SAMPLES; bs++) {
        float bin = mix(binLo, binHi, (float(bs) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    fftBands[k] = baseBright + energy;
}
```

#### Ray march loop (outer)

Transcribe from reference with substitutions applied:

```glsl
vec2 fragCoord = fragTexCoord * resolution;
vec2 u = (fragCoord - resolution * 0.5) / resolution.y;

float i, d, s, a, c, cyl;
float j;
vec3 color = vec3(0.0);

for (i = 0.0; i < float(marchSteps); i += 1.0) {
    // Ray setup — reference: vec3 p = vec3(d * u * 1.8, d * 1.8) + vec3(-.1, .7, -.2)
    vec3 p = vec3(d * u * fov, d * fov) + vec3(-0.1, 0.7, camZ);

    // Init fold state — reference: j = s = a = 8.45
    j = 8.45;
    s = 8.45;
    a = 8.45;

    // Inner fold loop — reference: while(j++ < 16.)
    int minFoldIdx = 0;  // Track which fold depth produced the minimum distance
    float jEnd = 8.45 + float(foldIterations);
    int foldIdx = 0;
    while (j < jEnd) {
        j += 1.0;

        // Sphere inversion — reference: c = dot(p,p), p /= c + .005, a /= c
        c = dot(p, p);
        p /= c + 0.005;
        a /= c;

        // Abs-fold with rotation — reference: p.xz = abs(rot(sin(t - 1./c) / a - j) * p.xz) - .5
        p.xz = abs(rot(sin(animPhase - 1.0 / c) / a - j) * p.xz) - foldOffset;

        // Y-fold — reference: p.y = 1.78 - p.y
        p.y = yFold - p.y;

        // Cylinder distance — reference: cyl = length(p.xz) * 2.5 - .06 / c
        cyl = length(p.xz) * 2.5 - branchThickness / c;
        cyl = max(cyl, p.y) / a;

        // Track which fold depth gave the closest approach
        float prevS = s;
        s = min(s, cyl) * 0.9;
        if (s < prevS) minFoldIdx = foldIdx;

        foldIdx++;
    }

    // Advance ray
    d += abs(s) + 1e-6;

    // Glow accumulation — reference: s < .001 ? o += .000001 / d*i : o
    // NOTE: reference formula is (0.000001 / d) * i — multiply by i, not divide
    if (s < 0.001) {
        float lutCoord = fract(d * colorStretch + colorPhase);
        vec3 stepColor = texture(gradientLUT, vec2(lutCoord, 0.5)).rgb;
        // Per-fold FFT brightness — use the fold depth that produced the hit
        float foldBright = fftBands[minFoldIdx];
        color += stepColor * foldBright * 0.000001 / d * i;
    }

    // Synapse glow — reference: bouncing sphere intersection
    if (cyl > length(p - vec3(0.0, sin(synapseBounceFreq * animPhase), 0.0))
              - 0.5 - 0.5 * cos(synapsePulseFreq * animPhase)) {
        float lutCoord = fract(d * colorStretch + colorPhase + 0.5);
        vec3 synapseColor = texture(gradientLUT, vec2(lutCoord, 0.5)).rgb;
        if (s < 0.001) {
            // Reference: o += .000005 / d * i — multiply by i
            color += synapseColor * synapseIntensity * 0.000005 / d * i;
        }
        if (s < 0.02) {
            color += synapseColor * synapseIntensity * 0.0002 / d;
        }
    }
}
```

#### Tonemap

<!-- Intentional deviation: divisor 7000.0 from Vortex pattern; research says "Same pattern as Vortex" without specifying. May need tuning. -->
```glsl
color = tanh(color * brightness / 7000.0);
```

#### Trail buffer

Verbatim from Vortex — 3x3 weighted blur kernel, mix with raw based on `trailBlur`, decay, NaN guard, `max(current, prev * decayFactor)`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| marchSteps | int | 40-200 | 120 | no | March Steps |
| foldIterations | int | 4-12 | 8 | no | Fold Depth |
| fov | float | 1.0-3.0 | 1.8 | yes | FOV |
| foldOffset | float | 0.2-1.0 | 0.5 | yes | Branch Spread |
| yFold | float | 1.0-3.0 | 1.78 | yes | Y Fold |
| branchThickness | float | 0.01-0.2 | 0.06 | yes | Thickness |
| animSpeed | float | 0.0-2.0 | 0.4 | yes | Anim Speed |
| driftSpeed | float | 0.0-1.0 | 0.1 | yes | Drift Speed |
| synapseIntensity | float | 0.0-2.0 | 0.5 | yes | Synapse |
| synapseBounceFreq | float | 1.0-20.0 | 11.0 | yes | Bounce Freq |
| synapsePulseFreq | float | 1.0-15.0 | 7.0 | yes | Pulse Freq |
| decayHalfLife | float | 0.1-10.0 | 2.0 | yes | Decay Half-Life |
| trailBlur | float | 0.0-1.0 | 0.5 | yes | Trail Blur |
| baseFreq | float | 27.5-440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |
| colorSpeed | float | 0.0-2.0 | 0.3 | yes | Color Speed |
| colorStretch | float | 0.1-5.0 | 1.0 | yes | Color Stretch |
| brightness | float | 0.1-5.0 | 1.0 | yes | Brightness |
| blendMode | enum | — | SCREEN | no | Blend Mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend |

### Constants

- Enum: `TRANSFORM_SYNAPSE_TREE_BLEND`
- Display name: `"Synapse Tree"`
- Section: `11` (Filament)
- Badge: `"GEN"` (auto from `REGISTER_GENERATOR_FULL`)
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE` (auto from macro)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Config and Effect header

**Files**: `src/effects/synapse_tree.h` (create)
**Creates**: `SynapseTreeConfig`, `SynapseTreeEffect` structs, `SYNAPSE_TREE_CONFIG_FIELDS` macro, function declarations

**Do**: Create the header following the Types section above. Follow `vortex.h` as the structural template — same include set, same function signature pattern. Include `"render/color_config.h"` for `ColorConfig`. One-line top comment: `// Synapse Tree: raymarched sphere-inversion fractal tree with synapse pulses`.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker errors expected).

---

### Wave 2: Implementation (parallel)

#### Task 2.1: C++ module

**Files**: `src/effects/synapse_tree.cpp` (create)
**Depends on**: Wave 1

**Do**: Implement all lifecycle functions following `vortex.cpp` as the exact template:

- `SynapseTreeEffectInit`: Load `shaders/synapse_tree.fs`, cache all uniform locations, init ColorLUT, init ping-pong textures via `RenderUtilsInitTextureHDR`, clear them. Return false and clean up on shader load failure.
- `SynapseTreeEffectSetup`: Accumulate `animPhase += cfg->animSpeed * deltaTime`, `camZ = fmodf(camZ + cfg->driftSpeed * deltaTime, 100.0f)`, `colorPhase += cfg->colorSpeed * deltaTime`. Compute `decayFactor = expf(-0.693147f * deltaTime / fmaxf(cfg->decayHalfLife, 0.001f))`. Bind all uniforms via `SetShaderValue`. Update ColorLUT.
- `SynapseTreeEffectRender`: Ping-pong render — same as `VortexEffectRender`. Bind previousFrame, gradientLUT, fftTexture via `SetShaderValueTexture`, draw fullscreen quad via `RenderUtilsDrawFullscreenQuad`, flip readIdx.
- `SynapseTreeEffectResize`: Unload + reinit ping-pong, reset readIdx.
- `SynapseTreeEffectUninit`: Unload shader, ColorLUT, ping-pong.
- `SynapseTreeConfigDefault`: Return `SynapseTreeConfig{}`.
- `SynapseTreeRegisterParams`: Register all modulatable params (see Parameters table — everything marked "yes"). Use `"synapseTree.fieldName"` dot-separated IDs. Ranges must match the config struct comments.
- Bridge functions: `SetupSynapseTree(PostEffect*)`, `SetupSynapseTreeBlend(PostEffect*)`, `RenderSynapseTree(PostEffect*)` — same delegation pattern as Vortex.

Includes: same as `vortex.cpp` — own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `render/render_utils.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<math.h>`, `<stddef.h>`.

**UI section** (`// === UI ===`):

`static void DrawSynapseTreeParams(EffectConfig *e, const ModSources *ms, ImU32 glow)`:

```
SeparatorText("Raymarching")
  SliderInt "March Steps##synapseTree"  marchSteps  40-200
  SliderInt "Fold Depth##synapseTree"   foldIterations  4-12
  ModulatableSlider "FOV##synapseTree"  fov  "%.1f"

SeparatorText("Shape")
  ModulatableSlider "Branch Spread##synapseTree"  foldOffset  "%.2f"
  ModulatableSlider "Y Fold##synapseTree"         yFold       "%.2f"
  ModulatableSliderLog "Thickness##synapseTree"    branchThickness  "%.3f"

SeparatorText("Animation")
  ModulatableSlider "Anim Speed##synapseTree"  animSpeed  "%.2f"
  ModulatableSlider "Drift Speed##synapseTree" driftSpeed "%.2f"

SeparatorText("Synapse")
  ModulatableSlider "Intensity##synapseTree"    synapseIntensity   "%.2f"
  ModulatableSlider "Bounce Freq##synapseTree"  synapseBounceFreq  "%.1f"
  ModulatableSlider "Pulse Freq##synapseTree"   synapsePulseFreq   "%.1f"

SeparatorText("Trails")
  ModulatableSlider "Decay Half-Life##synapseTree"  decayHalfLife  "%.1f"
  ModulatableSlider "Trail Blur##synapseTree"       trailBlur      "%.2f"

SeparatorText("Audio")
  ModulatableSlider "Base Freq (Hz)##synapseTree"  baseFreq   "%.1f"
  ModulatableSlider "Max Freq (Hz)##synapseTree"   maxFreq    "%.0f"
  ModulatableSlider "Gain##synapseTree"            gain       "%.1f"
  ModulatableSlider "Contrast##synapseTree"        curve      "%.2f"
  ModulatableSlider "Base Bright##synapseTree"     baseBright "%.2f"

SeparatorText("Color")
  ModulatableSlider "Color Speed##synapseTree"    colorSpeed    "%.2f"
  ModulatableSlider "Color Stretch##synapseTree"  colorStretch  "%.2f"

SeparatorText("Tonemap")
  ModulatableSlider "Brightness##synapseTree"  brightness  "%.2f"
```

Registration at bottom:
```cpp
STANDARD_GENERATOR_OUTPUT(synapseTree)
REGISTER_GENERATOR_FULL(TRANSFORM_SYNAPSE_TREE_BLEND, SynapseTree, synapseTree, "Synapse Tree",
                        SetupSynapseTreeBlend, SetupSynapseTree, RenderSynapseTree, 11,
                        DrawSynapseTreeParams, DrawOutput_synapseTree)
```

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/synapse_tree.fs` (create)
**Depends on**: Wave 1 (uniform names must match header)

**Do**: Create the GLSL 330 fragment shader implementing the Algorithm section above.

Attribution header (REQUIRED — first lines before `#version`):
```glsl
// Based on "Arvore 3b - sinapses" by Elsio
// https://www.shadertoy.com/view/XfXcR7
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized all constants; replaced raw color accumulation with
// gradient LUT; added FFT audio reactivity per fold depth; added trail buffer
// with decay/blur; added forward camera drift.
```

Structure:
1. `#version 330`, `in vec2 fragTexCoord`, `out vec4 finalColor`
2. All uniforms from the Algorithm > Uniforms section
3. `rot()` function from Algorithm > Rotation macro section
4. `void main()`:
   - FFT precomputation (Algorithm > FFT precomputation section) — `float fftBands[12]`
   - Screen coord setup: `u = (fragCoord - resolution * 0.5) / resolution.y`
   - Ray march outer loop (Algorithm > Ray march loop section)
   - Tanh tonemap (Algorithm > Tonemap section)
   - Trail buffer (verbatim from `vortex.fs` lines 108-121: 3x3 weighted blur, NaN guard, `max(current, prev * decayFactor)`)

**Verify**: Shader compiles at runtime (no standalone check available, verified in final build+run).

---

#### Task 2.3: Integration (config, post_effect, build, serialization)

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (all modify)
**Depends on**: Wave 1

**Do**:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/synapse_tree.h"` in the alphabetical include list (after `surface_warp.h`, before `synthwave.h`)
   - Add `TRANSFORM_SYNAPSE_TREE_BLEND,` in the enum before `TRANSFORM_ACCUM_COMPOSITE`
   - Add config member in EffectConfig struct: `SynapseTreeConfig synapseTree;` with comment `// Synapse Tree (raymarched fractal tree with synapse pulses)` — place after the `VortexConfig vortex;` entry

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/synapse_tree.h"` in alphabetical position (after `surface_warp.h`, before `synthwave.h`)
   - Add `SynapseTreeEffect synapseTree;` in the PostEffect struct after `VortexEffect vortex;`

3. **`CMakeLists.txt`**:
   - Add `src/effects/synapse_tree.cpp` to EFFECTS_SOURCES (after `surface_warp.cpp`)

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/synapse_tree.h"` in alphabetical position
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SynapseTreeConfig, SYNAPSE_TREE_CONFIG_FIELDS)` after the VortexConfig macro
   - Add `X(synapseTree)` to the `EFFECT_CONFIG_FIELDS(X)` macro — append after `X(vortex)` on the same line

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Effect appears in Filament section of transform order pipeline
- [x] Effect shows "GEN" badge
- [x] Enabling effect shows fractal tree with glow
- [ ] Synapse pulses produce colored flashes at branch tips
- [ ] Gradient LUT colors the tree
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes work for registered parameters

## Implementation Notes (Post-Mortem)

### What went wrong

**Blindly copied Vortex template.** Vortex uses ping-pong trail buffers, custom render, and resize because it's a volumetric accumulation effect. This fractal is a complete image per frame. The trail buffer added ghosting, washed out the image, and required REGISTER_GENERATOR_FULL with resize support. All of it had to be ripped out and replaced with REGISTER_GENERATOR (like Filaments). Multiple rounds of rework.

**Tonemap divisor cargo-culted from Vortex.** Vortex accumulates glow in the thousands range and uses `tanh(color / 7000)`. This fractal accumulates glow in the 0.001-0.01 range (reference has no tonemap). The `/7000` crushed everything to zero (black screen). Went through `*200`, `*100`, `*5` multipliers before removing the tonemap entirely.

**Uninitialized GLSL variable.** `float d;` in the shader is undefined in GLSL 330 (Shadertoy WebGL zero-inits, desktop GLSL does not). Caused black screen. Should have initialized `d = 0.0` from the start.

**Camera drift concept doesn't work.** Linear Z translation moves the camera past the fractal into empty space. The fractal is finite around the origin. Replaced with Y-axis orbit which keeps the camera at the reference distance while revealing different faces.

**FFT per-fold-depth mapping doesn't produce visible frequency discrimination.** Fold depths don't map to visually distinct spatial regions of the tree. A branch at fold depth 3 and a twig at fold depth 7 can be adjacent pixels. The minFoldIdx tracking was also broken by the 0.9 decay on `s` (always yielded the last fold). After fixing the tracking with separate `minCyl`, it was still visually indistinguishable. Per-step FFT (like Vortex) also doesn't work because ray steps don't represent visually distinct layers in a surface raymarcher. Screen-space Y mapping doesn't follow branches that go sideways. FFT removed entirely - needs a fundamentally different approach.

**Gradient LUT on base glow kills 3D depth.** The reference's 3D look comes from white glow with brightness variation (dim=far, bright=near). Replacing white with a saturated LUT color (e.g. cyan 0,1,1) means all depth levels are the same hue at different intensities - flat. LUT color moved to synapse accent only, base glow is white like reference.

**baseBright as FFT multiplier made tree invisible when silent.** `fftBands = baseBright + energy`, used as a multiplier on glow. baseBright=0.15 meant 15% of reference brightness with no audio. Changed to `1.0 + fftBands` which added a constant base, but that made baseBright=0 still show the tree. Ultimately removed with FFT.

**Parameter ranges invented without testing.** yFold 1.0-3.0 (useful range ~1.7-1.85), foldOffset 0.2-1.0 (useful range ~0.2-0.6), FOV scaled both XY and Z equally (not a real FOV change). yFold removed entirely and hardcoded to 1.78. foldOffset range tightened. FOV fixed to scale XY only.

### What survived

- Core raymarched fractal with sphere inversion IFS
- Camera circle (rotates offset position around Y, but view direction is fixed +Z - not a true orbit)
- Gradient LUT on synapse accent (fold-depth-based sampling)
- White base glow matching reference formula
- Glow normalization by fold count
- All shape/animation/synapse parameters (minus yFold)
- REGISTER_GENERATOR pattern (no ping-pong, no resize)
