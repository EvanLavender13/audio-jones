# Subdivide

Animated recursive BSP generator creating organic shifting quads with gradient LUT coloring and FFT-driven brightness. Based on "Always Watching" by SnoopethDuckDuck.

**Research**: `docs/research/subdivide.md`

## Design

### Types

```cpp
// src/effects/subdivide.h

struct SubdivideConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor (0.0-1.0)

  // Geometry
  float speed = 0.45f;     // Animation rate (0.1-2.0)
  float squish = 0.01f;    // Cut line waviness (0.001-0.05)
  float threshold = 0.15f; // Subdivision probability cutoff (0.01-0.9)
  int maxIterations = 14;  // Maximum BSP recursion depth (2-20)

  // Visual
  float edgeDarken = 0.75f;      // SDF edge/shadow darkening intensity (0.0-1.0)
  float areaFade = 0.0007f;      // Area below which cells dissolve (0.0001-0.005)
  float desatThreshold = 0.5f;   // Hash above which desaturation applies (0.0-1.0)
  float desatAmount = 0.9f;      // Strength of desaturation on gated cells (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SUBDIVIDE_CONFIG_FIELDS                                                \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, speed, squish,         \
      threshold, maxIterations, edgeDarken, areaFade, desatThreshold,         \
      desatAmount, gradient, blendMode, blendIntensity

typedef struct SubdivideEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // CPU-accumulated animation phase
  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int squishLoc;
  int thresholdLoc;
  int maxIterationsLoc;
  int edgeDarkenLoc;
  int areaFadeLoc;
  int desatThresholdLoc;
  int desatAmountLoc;
  int gradientLUTLoc;
} SubdivideEffect;
```

### Algorithm

The shader implements a recursive binary space partition. Each pixel traces through up to `maxIterations` subdivision cuts to find which cell it belongs to, then colors that cell using gradient LUT with FFT-driven brightness.

#### Utility functions (keep verbatim from reference)

```glsl
#define C(u,a,b) cross(vec3(u-a,0), vec3(b-a,0)).z > 0.
#define S(a) smoothstep(2./R.y, -2./R.y, a)

float h11(float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float h12(vec2 p) {
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float catrom(float t) {
    float f = floor(t),
          x = t - f;
    float v0 = h11(f), v1 = h11(f+1.), v2 = h11(f+2.), v3 = h11(f+3.);
    float c2 = -.5 * v0    + 0.5*v2;
    float c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    float c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

float sdPoly(in vec2[4] v, in vec2 p) {
    float d = dot(p-v[0],p-v[0]);
    float s = 1.0;
    for (int i=0, j=3; i<4; j=i, i++) {
        vec2 e = v[j] - v[i];
        vec2 w =    p - v[i];
        vec2 b = w - e*clamp(dot(w,e)/dot(e,e), 0.0, 1.0);
        d = min(d, dot(b,b));
        bvec3 c = bvec3(p.y>=v[i].y,p.y<v[j].y,e.x*w.y>e.y*w.x);
        if (all(c) || all(not(c))) s*=-1.0;
    }
    return s*sqrt(d);
}
```

#### Main shader body

```glsl
void main() {
    vec2 R = resolution;
    vec2 fc = fragTexCoord * R;
    vec2 u = (fc + fc - R) / R.y;

    // Initialize quad corners to screen bounds (aspect-corrected)
    vec2 tl = vec2(-1, 1) * R / R.y;
    vec2 tr = vec2( 1, 1) * R / R.y;
    vec2 bl = vec2(-1,-1) * R / R.y;
    vec2 br = vec2( 1,-1) * R / R.y;

    float t = time;  // CPU-accumulated with speed

    vec2 ID = vec2(0.0);
    float area;

    // BSP subdivision loop
    for (int i = 0; i < maxIterations; i++) {
        t += 7.3 * h12(ID);

        float k = float(i) + 1.0;
        float K = 1.0 / k;

        // Squish: replaces original's intro-animated `to` variable
        float to = squish * (10.0 - k) * cos(t*k + (u.x+u.y)*k/2.0 + h12(ID)*10.0);
        float mx1 = catrom(t);
        float mx2 = catrom(t + to);
        vec2 x1, x2;

        // Alternate horizontal and vertical cuts
        if (i % 2 == 0) {
            x1 = mix(tl, tr, mx1);
            x2 = mix(bl, br, mx2);
            if (C(u,x1,x2)) { tr = x1; br = x2; ID += vec2(K,0); }
            else             { tl = x1; bl = x2; ID -= vec2(K,0); }
        } else {
            x1 = mix(tl, bl, mx1);
            x2 = mix(tr, br, mx2);
            if (C(u,x1,x2)) { tl = x1; tr = x2; ID += vec2(0,K); }
            else             { bl = x1; br = x2; ID -= vec2(0,K); }
        }

        // Shoelace area
        area = tl.x*bl.y + bl.x*br.y + br.x*tr.y + tr.x*tl.y
             - tl.y*bl.x - bl.y*br.x - br.y*tr.x - tr.y*tl.x;

        // Probabilistic early exit
        if (h12(ID) < threshold) break;
    }

    // Per-cell hash for color and frequency assignment
    float h = h12(ID);

    // FFT brightness: cell hash maps to frequency band
    float freqRatio = maxFreq / baseFreq;
    float bandWidth = 0.05;
    float t0 = max(0.0, h - bandWidth);
    float t1 = min(1.0, h + bandWidth);
    float freqLo = baseFreq * pow(freqRatio, t0);
    float freqHi = baseFreq * pow(freqRatio, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float brightness = baseBright + pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

    // Gradient LUT coloring
    vec3 col = texture(gradientLUT, vec2(h, 0.5)).rgb * brightness;

    // Edge darkening (configurable intensity)
    float l = max(length(tl-br), length(tr-bl));
    col *= mix(1.0, 0.25 + 0.75 * S(sdPoly(vec2[](tl, bl, br, tr), u) + 0.0033), edgeDarken);
    col *= mix(1.0, 0.75 + 0.25 * S(sdPoly(vec2[](tl, bl, br, tr), u + 0.07*l)), edgeDarken);

    // Desaturation gate
    if (h > desatThreshold) {
        float lum = dot(col, vec3(0.299, 0.587, 0.114));
        col = mix(col, vec3(lum), desatAmount);
    }

    col = clamp(col, 0.0, 1.0);

    // Area fade: tiny cells dissolve to black
    vec3 bg = vec3(0.0);
    col = mix(bg, col, exp(-areaFade / area));

    finalColor = vec4(col, 1.0);
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
| speed | float | 0.1-2.0 | 0.45 | yes | Speed |
| squish | float | 0.001-0.05 | 0.01 | yes | Squish |
| threshold | float | 0.01-0.9 | 0.15 | yes | Threshold |
| maxIterations | int | 2-20 | 14 | no | Iterations |
| edgeDarken | float | 0.0-1.0 | 0.75 | yes | Edge Darken |
| areaFade | float | 0.0001-0.005 | 0.0007 | yes | Area Fade |
| desatThreshold | float | 0.0-1.0 | 0.5 | yes | Desat Threshold |
| desatAmount | float | 0.0-1.0 | 0.9 | yes | Desat Amount |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_SUBDIVIDE_BLEND`
- Display name: `"Subdivide"`
- Badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section: `12` (Texture)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create subdivide header

**Files**: `src/effects/subdivide.h` (create)
**Creates**: `SubdivideConfig`, `SubdivideEffect` types, function declarations

**Do**: Create the header file with the struct layouts from the Design > Types section above. Follow `hex_rush.h` as the structural pattern:
- Include guards `#ifndef SUBDIVIDE_H` / `#define SUBDIVIDE_H`
- Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`
- `SubdivideConfig` struct with all fields and defaults from Design > Types
- `SUBDIVIDE_CONFIG_FIELDS` macro listing all serializable fields from Design > Types
- Forward declare `typedef struct ColorLUT ColorLUT;`
- `SubdivideEffect` typedef struct with all `int *Loc` fields and `float time` accumulator from Design > Types
- Function declarations:
  - `bool SubdivideEffectInit(SubdivideEffect *e, const SubdivideConfig *cfg);`
  - `void SubdivideEffectSetup(SubdivideEffect *e, const SubdivideConfig *cfg, float deltaTime, const Texture2D &fftTexture);`
  - `void SubdivideEffectUninit(SubdivideEffect *e);`
  - `void SubdivideRegisterParams(SubdivideConfig *cfg);`
- Top-of-file comment: `// Subdivide generator - recursive BSP with squishy Catmull-Rom cut positions`

**Verify**: `cmake.exe --build build` compiles (header-only, no link yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create subdivide shader

**Files**: `shaders/subdivide.fs` (create)
**Depends on**: Wave 1 complete (for uniform name agreement)

**Do**: Create the fragment shader. Follow the Design > Algorithm section as the single source of truth for all GLSL code.

Attribution header (REQUIRED, before `#version`):
```glsl
// Based on "Always Watching" by SnoopethDuckDuck
// https://www.shadertoy.com/view/ffB3D1
// License: CC BY-NC-SA 3.0 Unported
// Modified: removed faces/stripes/vignette, replaced cosine palette with
// gradient LUT, added FFT audio reactivity, exposed config uniforms
```

Then `#version 330`, standard ins/outs, all uniforms matching the `*Loc` fields in the Effect struct.

Implement the Algorithm section. Keep the utility functions verbatim. The main body applies the substitutions documented in the research doc (already reflected in the Algorithm section).

**Verify**: Shader file exists and has correct uniform names matching the header.

---

#### Task 2.2: Create subdivide implementation

**Files**: `src/effects/subdivide.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the implementation file. Follow `hex_rush.cpp` as the structural pattern.

Includes (generator `.cpp` convention):
- `"subdivide.h"`
- `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`
- `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`
- `<stddef.h>`

Functions:
1. `SubdivideEffectInit`: Load shader `"shaders/subdivide.fs"`, cache all uniform locations, `ColorLUTInit(&cfg->gradient)`, init `e->time = 0.0f`. Cascade cleanup on failure (same pattern as hex_rush).

2. `SubdivideEffectSetup`: Accumulate `e->time += cfg->speed * deltaTime`. Call `ColorLUTUpdate`. Bind all uniforms via `SetShaderValue`/`SetShaderValueTexture`. Bind `fftTexture`, `sampleRate` (`AUDIO_SAMPLE_RATE`), all config fields, `e->time`, `resolution`, `gradientLUT`.

3. `SubdivideEffectUninit`: `UnloadShader`, `ColorLUTUninit`.

4. `SubdivideRegisterParams`: Register all modulatable params with dot-separated IDs (`"subdivide.baseFreq"`, etc.). Ranges must match the Parameter table. `maxIterations` is NOT registered (int, not modulatable).

5. Non-static bridge: `void SetupSubdivide(PostEffect *pe)` - calls `SubdivideEffectSetup` with `pe->subdivide`, `pe->effects.subdivide`, `pe->currentDeltaTime`, `pe->fftTexture`.

6. Non-static bridge: `void SetupSubdivideBlend(PostEffect *pe)` - calls `BlendCompositorApply` with `pe->blendCompositor`, `pe->generatorScratch.texture`, blend intensity/mode from config.

7. Colocated UI section (`// === UI ===`):
   - `static void DrawSubdivideParams(EffectConfig *e, const ModSources *ms, ImU32 glow)`:
     - `(void)glow;`
     - Audio section: `ImGui::SeparatorText("Audio")`, then Base Freq, Max Freq, Gain, Contrast, Base Bright sliders (standard order/format from conventions)
     - Geometry section: `ImGui::SeparatorText("Geometry")`, then Speed (`ModulatableSlider`, `"%.2f"`), Squish (`ModulatableSliderLog`, `"%.4f"`), Threshold (`ModulatableSlider`, `"%.2f"`), Iterations (`ImGui::SliderInt`, 2-20)
     - Visual section: `ImGui::SeparatorText("Visual")`, then Edge Darken (`ModulatableSlider`, `"%.2f"`), Area Fade (`ModulatableSliderLog`, `"%.4f"`), Desat Threshold (`ModulatableSlider`, `"%.2f"`), Desat Amount (`ModulatableSlider`, `"%.2f"`)

8. Registration (bottom of file):
```cpp
// clang-format off
STANDARD_GENERATOR_OUTPUT(subdivide)
REGISTER_GENERATOR(TRANSFORM_SUBDIVIDE_BLEND, Subdivide, subdivide,
                   "Subdivide", SetupSubdivideBlend,
                   SetupSubdivide, 12, DrawSubdivideParams, DrawOutput_subdivide)
// clang-format on
```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Config registration and integration

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Wire the new effect into all integration points.

1. `src/config/effect_config.h`:
   - Add `#include "effects/subdivide.h"` with other effect includes
   - Add `TRANSFORM_SUBDIVIDE_BLEND,` to `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE` (order array auto-populates)
   - Add `SubdivideConfig subdivide;` to `EffectConfig` struct

2. `src/render/post_effect.h`:
   - Add `#include "effects/subdivide.h"` with other effect includes
   - Add `SubdivideEffect subdivide;` to `PostEffect` struct

3. `CMakeLists.txt`:
   - Add `src/effects/subdivide.cpp` to `EFFECTS_SOURCES`

4. `src/config/effect_serialization.cpp`:
   - Add `#include "effects/subdivide.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SubdivideConfig, SUBDIVIDE_CONFIG_FIELDS)` with other JSON macros
   - Add `X(subdivide) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `TRANSFORM_SUBDIVIDE_BLEND` appears in enum before `TRANSFORM_ACCUM_COMPOSITE`
- [ ] `SubdivideConfig subdivide` in `EffectConfig`
- [ ] `SubdivideEffect subdivide` in `PostEffect`
- [ ] `src/effects/subdivide.cpp` in `EFFECTS_SOURCES`
- [ ] `X(subdivide)` in serialization X-macro table
- [ ] Shader has attribution comment block before `#version`
- [ ] All uniform names in shader match `*Loc` fields in Effect struct
- [ ] `e->time` accumulated on CPU (`speed * deltaTime`), never in shader
- [ ] FFT brightness uses standard `baseBright + pow(clamp(energy * gain, 0, 1), curve)` formula
- [ ] Bridge functions (`SetupSubdivide`, `SetupSubdivideBlend`) are NOT static
- [ ] `DrawSubdivideParams` IS static
- [ ] `REGISTER_GENERATOR` macro at bottom of `.cpp` with correct arguments
