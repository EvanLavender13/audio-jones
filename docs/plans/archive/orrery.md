# Orrery

Procedural hierarchical ring orrery generator. Circles orbit on their parent's circumference in a configurable tree structure (depth, branches, scaling rules). Deeper rings spin faster and respond to higher frequencies. Connecting lines between leaves form a shifting geometric web. Gradient LUT for color, FFT for per-ring brightness.

**Research**: `docs/research/orrery.md`

## Design

### Types

```cpp
// src/effects/orrery.h

struct OrreryConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Baseline brightness when audio silent (0.0-1.0)

  // Tree structure
  int branches = 3;             // Children per ring at each level (2-6)
  int depth = 3;                // Hierarchy levels, root = level 0 (2-4)
  float rootRadius = 0.5f;      // Root ring radius (0.1-0.8)
  float radiusDecay = 0.4f;     // Per-level radius shrink factor (0.1-0.8)
  float radiusVariation = 0.0f; // Per-ring random radius offset (0.0-1.0)

  // Animation
  float baseSpeed = 0.8f;       // Level 1 orbital speed, rad/s (0.1-3.0)
  float speedScale = 2.0f;      // Speed multiplier per depth level (1.0-4.0)
  float speedVariation = 0.0f;  // Per-ring random speed offset (0.0-1.0)

  // Stroke
  float strokeWidth = 0.005f;   // Base ring thickness (0.001-0.02)
  float strokeTaper = 0.5f;     // Depth thinning: 0=uniform, 1=leaves thinnest (0.0-1.0)

  // Lines
  int lineMode = 0;             // 0=All Leaves, 1=Siblings
  float lineBrightness = 0.5f;  // Line opacity, 0=off (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define ORRERY_CONFIG_FIELDS                                                   \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, branches, depth,        \
      rootRadius, radiusDecay, radiusVariation, baseSpeed, speedScale,         \
      speedVariation, strokeWidth, strokeTaper, lineMode, lineBrightness,      \
      gradient, blendMode, blendIntensity

typedef struct OrreryEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float timeAccum;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int timeAccumLoc;
  int branchesLoc;
  int depthLoc;
  int rootRadiusLoc;
  int radiusDecayLoc;
  int radiusVariationLoc;
  int speedScaleLoc;
  int speedVariationLoc;
  int strokeWidthLoc;
  int strokeTaperLoc;
  int lineModeLoc;
  int lineBrightnessLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} OrreryEffect;
```

### Algorithm

The shader builds a breadth-first ring tree in a flat loop. Since GLSL has no recursion, we use a flat index scheme where each ring's parent index and sibling position are derived arithmetically.

#### Indexing scheme

Total rings = `1 + b + b^2 + ... + b^d` where `b = branches`, `d = depth`.

Level `L` starts at flat index `levelStart(L) = (pow(b, L) - 1) / (b - 1)` (geometric series partial sum; special case `b == 1` not needed since branches >= 2).

For ring at flat index `i` in level `L`:
- Parent index: `levelStart(L-1) + (i - levelStart(L)) / b`
- Sibling index: `(i - levelStart(L)) % b`
- Rotation offset: `2 * PI * siblingIndex / b`

#### Full shader

```glsl
// Based on "Circles spinning" by evewas
// https://www.shadertoy.com/view/scXXz4
// License: CC BY-NC-SA 3.0 Unported
// Modified: procedural tree, FFT brightness, gradient LUT, connecting lines
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float timeAccum;
uniform int branches;
uniform int depth;
uniform float rootRadius;
uniform float radiusDecay;
uniform float radiusVariation;
uniform float speedScale;
uniform float speedVariation;
uniform float strokeWidth;
uniform float strokeTaper;
uniform int lineMode;
uniform float lineBrightness;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform sampler2D gradientLUT;

const float PI = 3.14159265359;
const float TAU = 6.28318530718;
const int MAX_RINGS = 40;

// Deterministic hash for per-ring variation
float ringHash(int idx) {
    return fract(sin(float(idx) * 127.1) * 43758.5453);
}

// Antialiased SDF cutoff - from reference
float cutoff(float sdf, float thickness) {
    float fade = 0.001;
    return smoothstep(clamp(sdf - thickness, 0.0, 1.0), -fade, fade);
}

// Line segment SDF - from reference
float lineSDF(vec2 uv, vec2 a, vec2 b) {
    vec2 pa = uv - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - h * ba);
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    vec2 uv = (fc - 0.5 * resolution) / min(resolution.x, resolution.y);

    // Compute level start indices
    int levelStart[5]; // depth <= 4, so max 5 levels (0..4)
    levelStart[0] = 0;
    int ringsPerLevel = 1;
    for (int L = 1; L <= depth; L++) {
        levelStart[L] = levelStart[L - 1] + ringsPerLevel;
        ringsPerLevel *= branches;
    }
    int totalRings = levelStart[depth] + ringsPerLevel;
    // Clamp to MAX_RINGS only as array guard
    if (totalRings > MAX_RINGS) { totalRings = MAX_RINGS; }

    // Per-ring state arrays
    vec2 ringCenter[MAX_RINGS]; // world-space center
    float ringRadius[MAX_RINGS];
    float ringT[MAX_RINGS]; // depth-normalized index for color/FFT
    int ringLevel[MAX_RINGS];
    int ringParent[MAX_RINGS];

    // Build tree breadth-first
    int ringIdx = 0;

    // Level 0: root ring - stationary at center
    ringCenter[0] = vec2(0.0);
    ringRadius[0] = rootRadius;
    ringT[0] = 0.0;
    ringLevel[0] = 0;
    ringParent[0] = -1;
    ringIdx = 1;

    for (int L = 1; L <= depth; L++) {
        float levelRadius = rootRadius * pow(radiusDecay, float(L));
        float levelSpeed = pow(speedScale, float(L));
        float tBase = float(L) / float(depth);
        int parentStart = levelStart[L - 1];
        int parentsInLevel = levelStart[L] - parentStart;

        for (int p = 0; p < parentsInLevel; p++) {
            int parentIdx = parentStart + p;
            for (int s = 0; s < branches; s++) {
                if (ringIdx >= MAX_RINGS) { break; }

                // Per-ring variation
                float h = ringHash(ringIdx);
                float r = levelRadius * (1.0 + (h - 0.5) * radiusVariation);
                float spd = levelSpeed * (1.0 + (ringHash(ringIdx + MAX_RINGS) - 0.5) * speedVariation);
                float rotOffset = TAU * float(s) / float(branches);

                // Orbital position on parent's circumference
                float angle = timeAccum * spd + rotOffset;
                vec2 orbitOffset = ringRadius[parentIdx] * vec2(cos(angle), sin(angle));
                ringCenter[ringIdx] = ringCenter[parentIdx] + orbitOffset;

                ringRadius[ringIdx] = r;
                // Normalized t: spread within this level so siblings get slightly different t
                ringT[ringIdx] = (float(L) - 1.0 + float(s) / float(branches)) / float(depth);
                ringLevel[ringIdx] = L;
                ringParent[ringIdx] = parentIdx;
                ringIdx++;
            }
            if (ringIdx >= MAX_RINGS) { break; }
        }
        if (ringIdx >= MAX_RINGS) { break; }
    }

    int actualRings = ringIdx;

    // Evaluate ring SDFs and accumulate color
    vec3 total = vec3(0.0);
    float freqRatio = maxFreq / baseFreq;

    for (int i = 0; i < actualRings; i++) {
        // SDF: ring outline at ringCenter with ringRadius
        float d = abs(length(uv - ringCenter[i]) - ringRadius[i]);

        // Depth-tapered stroke width
        float taper = 1.0 - strokeTaper * float(ringLevel[i]) / float(depth);
        float sw = strokeWidth * taper;
        float ring = cutoff(d, sw);

        // FFT lookup using this ring's t index
        float t = ringT[i];
        float freq = baseFreq * pow(freqRatio, t);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // Color from gradient LUT
        vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

        total += color * ring * brightness;
    }

    // Connecting lines between leaf rings
    if (lineBrightness > 0.0) {
        // Collect leaf indices (deepest level)
        int leafStart = levelStart[depth];
        int leafCount = actualRings - leafStart;
        // Clamp leafCount guard
        if (leafStart < 0 || leafStart >= actualRings) { leafCount = 0; }

        for (int i = 0; i < leafCount; i++) {
            int ai = leafStart + i;

            // Determine connection range based on mode
            if (lineMode == 0) {
                // All Leaves: connect to every other leaf with higher index
                for (int j = i + 1; j < leafCount; j++) {
                    int bi = leafStart + j;
                    float ld = lineSDF(uv, ringCenter[ai], ringCenter[bi]);
                    float line = cutoff(ld, strokeWidth * 0.5);
                    float lt = (ringT[ai] + ringT[bi]) * 0.5;
                    vec3 lColor = texture(gradientLUT, vec2(lt, 0.5)).rgb;

                    float lFreq = baseFreq * pow(freqRatio, lt);
                    float lBin = lFreq / (sampleRate * 0.5);
                    float lEnergy = 0.0;
                    if (lBin <= 1.0) { lEnergy = texture(fftTexture, vec2(lBin, 0.5)).r; }
                    float lMag = pow(clamp(lEnergy * gain, 0.0, 1.0), curve);
                    float lBright = baseBright + lMag;

                    total += lColor * line * lineBrightness * lBright;
                }
            } else {
                // Siblings: connect to other leaves sharing same parent
                for (int j = i + 1; j < leafCount; j++) {
                    int bi = leafStart + j;
                    if (ringParent[ai] != ringParent[bi]) { continue; }
                    float ld = lineSDF(uv, ringCenter[ai], ringCenter[bi]);
                    float line = cutoff(ld, strokeWidth * 0.5);
                    float lt = (ringT[ai] + ringT[bi]) * 0.5;
                    vec3 lColor = texture(gradientLUT, vec2(lt, 0.5)).rgb;

                    float lFreq = baseFreq * pow(freqRatio, lt);
                    float lBin = lFreq / (sampleRate * 0.5);
                    float lEnergy = 0.0;
                    if (lBin <= 1.0) { lEnergy = texture(fftTexture, vec2(lBin, 0.5)).r; }
                    float lMag = pow(clamp(lEnergy * gain, 0.0, 1.0), curve);
                    float lBright = baseBright + lMag;

                    total += lColor * line * lineBrightness * lBright;
                }
            }
        }
    }

    finalColor = vec4(total, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5-440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |
| branches | int | 2-6 | 3 | no | Branches |
| depth | int | 2-4 | 3 | no | Depth |
| rootRadius | float | 0.1-0.8 | 0.5 | yes | Root Radius |
| radiusDecay | float | 0.1-0.8 | 0.4 | yes | Radius Decay |
| radiusVariation | float | 0.0-1.0 | 0.0 | yes | Radius Variation |
| baseSpeed | float | 0.1-3.0 | 0.8 | yes | Base Speed |
| speedScale | float | 1.0-4.0 | 2.0 | yes | Speed Scale |
| speedVariation | float | 0.0-1.0 | 0.0 | yes | Speed Variation |
| strokeWidth | float | 0.001-0.02 | 0.005 | yes | Stroke Width |
| strokeTaper | float | 0.0-1.0 | 0.5 | yes | Stroke Taper |
| lineMode | int | 0-1 | 0 | no | Line Mode |
| lineBrightness | float | 0.0-1.0 | 0.5 | yes | Line Brightness |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_ORRERY_BLEND`
- Display name: `"Orrery"`
- Category badge: `"GEN"` (automatic from `REGISTER_GENERATOR`)
- Section index: `10` (Geometric)
- Flags: `EFFECT_FLAG_BLEND` (automatic from `REGISTER_GENERATOR`)

### UI Layout

```
Audio
  Base Freq (Hz)     ModulatableSlider  "%.1f"
  Max Freq (Hz)      ModulatableSlider  "%.0f"
  Gain               ModulatableSlider  "%.1f"
  Contrast           ModulatableSlider  "%.2f"
  Base Bright        ModulatableSlider  "%.2f"

Geometry
  Branches           SliderInt          2-6
  Depth              SliderInt          2-4
  Root Radius        ModulatableSlider  "%.2f"
  Radius Decay       ModulatableSlider  "%.2f"
  Radius Variation   ModulatableSlider  "%.2f"

Lines
  Line Mode          Combo              "All Leaves\0Siblings\0"
  Line Brightness    ModulatableSlider  "%.2f"

Animation
  Base Speed         ModulatableSlider  "%.2f"
  Speed Scale        ModulatableSlider  "%.2f"
  Speed Variation    ModulatableSlider  "%.2f"

Stroke
  Stroke Width       ModulatableSliderLog "%.3f"
  Stroke Taper       ModulatableSlider  "%.2f"
```

Standard generator output (Color + Output sections) via `STANDARD_GENERATOR_OUTPUT(orrery)`.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/orrery.h` (create)
**Creates**: `OrreryConfig`, `OrreryEffect` structs, `ORRERY_CONFIG_FIELDS` macro, lifecycle declarations

**Do**: Create header following the `OrreryConfig` and `OrreryEffect` struct definitions from the Design section above. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Declare `OrreryEffectInit`, `OrreryEffectSetup`, `OrreryEffectUninit`, `OrreryRegisterParams`. Follow `signal_frames.h` as the structural template.

Init signature: `bool OrreryEffectInit(OrreryEffect *e, const OrreryConfig *cfg)` (needs cfg for ColorLUT init).

Setup signature: `void OrreryEffectSetup(OrreryEffect *e, const OrreryConfig *cfg, float deltaTime, const Texture2D &fftTexture)`.

**Verify**: `cmake.exe --build build` compiles (header-only, no link errors expected if not yet included).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect implementation

**Files**: `src/effects/orrery.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create implementation following `signal_frames.cpp` as the structural template.

Functions:
- `OrreryEffectInit`: Load shader `"shaders/orrery.fs"`, cache all uniform locations (one per field in `OrreryEffect`), init `ColorLUT`, init `timeAccum = 0.0f`. Return false and clean up if shader load fails.
- `OrreryEffectSetup`: Accumulate `timeAccum += cfg->baseSpeed * deltaTime`. Call `ColorLUTUpdate`. Bind all uniforms via `SetShaderValue`/`SetShaderValueTexture`. Bind `fftTexture`, `gradientLUT`, `sampleRate`, `resolution`.
- `OrreryEffectUninit`: Unload shader, `ColorLUTUninit`.
- `OrreryRegisterParams`: Register all modulatable params from the Parameters table (prefix `"orrery."`).

Bridge functions (non-static):
- `SetupOrrery(PostEffect *pe)`: delegates to `OrreryEffectSetup`.
- `SetupOrreryBlend(PostEffect *pe)`: calls `BlendCompositorApply`.

UI section (`// === UI ===`):
- `static void DrawOrreryParams(EffectConfig *e, const ModSources *ms, ImU32 glow)`: implement the UI Layout from the Design section. Use `ImGui::Combo` for lineMode with `"All Leaves\0Siblings\0"`.

Registration:
```cpp
STANDARD_GENERATOR_OUTPUT(orrery)
REGISTER_GENERATOR(TRANSFORM_ORRERY_BLEND, Orrery, orrery,
                   "Orrery", SetupOrreryBlend,
                   SetupOrrery, 10, DrawOrreryParams, DrawOutput_orrery)
```

Includes: same as `signal_frames.cpp` (own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`).

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Fragment shader

**Files**: `shaders/orrery.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the full shader from the Algorithm section of this plan. Copy verbatim - the Algorithm section contains the complete GLSL source. The attribution comment block at the top is required.

**Verify**: File exists at `shaders/orrery.fs`.

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/orrery.h"` in the include block (alphabetical position after `#include "effects/neon_lattice.h"`)
2. Add `TRANSFORM_ORRERY_BLEND,` to the `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE`
3. Add `OrreryConfig orrery;` to the `EffectConfig` struct with comment `// Orrery (hierarchical ring tree generator with blend)`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/orrery.h"` in the include block
2. Add `OrreryEffect orrery;` member to the `PostEffect` struct (near other generator effect members)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/orrery.cpp` to `EFFECTS_SOURCES` (alphabetical position).

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/orrery.h"` in the include block
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OrreryConfig, ORRERY_CONFIG_FIELDS)` with the other per-config macros
3. Add `X(orrery) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles.

---

## Implementation Notes

Deviations from original plan during implementation:

- **No shader arrays or textures**: Ring positions computed on the fly via `computeRing()` which walks the parent chain from root (max `depth` trig ops). Eliminates all compile-time array limits.
- **Seed-based lines replace All Leaves mode**: `lineMode` is now 0=Seeded (random pairs from `lineSeed`), 1=Siblings. All-to-all was O(N^2) per pixel and infeasible at scale.
- **`lineSeed` parameter added**: Modulatable float (0-100) that selects which leaf pairs to connect. `lineCount` (0-20) controls how many lines.
- **Per-level CPU phase accumulation**: `levelPhase[5]` and `variationPhase[5]` accumulated on CPU with `speedScale` baked in. Prevents position jumps when adjusting speedScale, speedVariation, or baseSpeed.
- **`cutoff()` inverted**: Reference function returned 0 on ring, 1 away (subtractive). Inverted with `1.0 -` for additive rendering.
- **Flat-index `t` for color/FFT**: `t = float(i) / float(ringCount)` gives every ring a unique gradient color and FFT band, instead of depth-based mapping that only produced ~4 distinct values.
- **BAND_SAMPLES**: All FFT lookups (rings and lines) use 4-sample sub-band averaging via `fftBrightness()` helper.
- **Radius decay inverted**: `pow(1.0 - radiusDecay, level)` so higher decay = smaller children.
- **Radius variation range doubled**: `(hash * 2.0 - 1.0)` for +/-100% at max.
- **baseSpeed range**: -1.0 to 1.0 (bidirectional), default 0.2.

## Final Verification

- [x] Build succeeds with no warnings
- [ ] Orrery appears in Geometric section of Effects window
- [ ] Enable checkbox shows orrery rings on screen
- [ ] Branches/Depth sliders change tree structure
- [ ] Line Mode combo switches between Seeded/Siblings
- [ ] Line Brightness at 0 hides all lines
- [ ] FFT drives per-ring brightness (bass = root, treble = leaves)
- [ ] Gradient LUT colors rings by depth
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
