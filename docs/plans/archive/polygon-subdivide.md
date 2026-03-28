# Polygon Subdivide

Recursive convex polygon subdivision via arbitrary-angle line cuts. Starts with a screen-filling quad and repeatedly slices polygons with lines through their bounding-box center, producing irregular stained-glass / geological fault mosaics. Each cell gets a unique hash ID for gradient LUT coloring and per-cell FFT brightness. Sibling to the existing Subdivide generator (same author, shared utility functions), but produces arbitrary convex N-gons instead of axis-aligned quads.

**Research**: `docs/research/polygon_subdivide.md`

## Design

### Types

```c
// polygon_subdivide.h

struct PolygonSubdivideConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor (0.0-1.0)

  // Geometry
  float speed = 0.45f;     // Animation rate (0.1-2.0)
  float threshold = 0.15f; // Subdivision probability cutoff (0.01-0.9)
  int maxIterations = 8;   // Maximum subdivision depth (2-20)

  // Visual
  float edgeDarken = 0.75f; // SDF edge/shadow darkening intensity (0.0-1.0)
  float areaFade = 0.0007f; // Area below which cells dissolve (0.0001-0.005)
  float desatThreshold = 0.5f;  // Hash above which desaturation applies (0.0-1.0)
  float desatAmount = 0.9f;     // Strength of desaturation on gated cells (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

```c
#define POLYGON_SUBDIVIDE_CONFIG_FIELDS                                        \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, speed, threshold,       \
      maxIterations, edgeDarken, areaFade, desatThreshold, desatAmount,        \
      gradient, blendMode, blendIntensity
```

```c
typedef struct PolygonSubdivideEffect {
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
  int thresholdLoc;
  int maxIterationsLoc;
  int edgeDarkenLoc;
  int areaFadeLoc;
  int desatThresholdLoc;
  int desatAmountLoc;
  int gradientLUTLoc;
} PolygonSubdivideEffect;
```

### Algorithm

The shader is built by applying the research doc's Keep/Replace/Add substitution tables to the reference code. Helper functions are kept verbatim from the reference; `h12()` is added from the Subdivide sibling; FFT brightness, desaturation gate, area fade, and edge darkening are added from Subdivide.

#### Uniforms

```glsl
uniform vec2 resolution;
uniform float time;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float threshold;
uniform int maxIterations;
uniform float edgeDarken;
uniform float areaFade;
uniform float desatThreshold;
uniform float desatAmount;
uniform sampler2D gradientLUT;
```

#### Macros and constants

```glsl
#define C(a,b) ((a).x*(b).y-(a).y*(b).x)
#define S(a) smoothstep(2./R.y, -2./R.y, a)

const int maxN = 15;
```

#### Helper functions (all verbatim from reference)

```glsl
vec2 center(vec2[maxN] P, int N) {
    vec2 bl = vec2(1e5), tr = vec2(-1e5);
    for (int j = 0; j < N; j++) {
        bl = min(bl, P[j]);
        tr = max(tr, P[j]);
    }
    return (bl + tr) / 2.0;
}

float h11(float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec2 h21(float p) {
    vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float catrom1(float t) {
    float f = floor(t),
          x = t - f;
    float v0 = h11(f), v1 = h11(f+1.), v2 = h11(f+2.), v3 = h11(f+3.);
    float c2 = -.5 * v0    + 0.5*v2;
    float c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    float c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

vec2 catrom2(float t) {
    float f = floor(t),
          x = t - f;
    vec2 v0 = h21(f), v1 = h21(f+1.), v2 = h21(f+2.), v3 = h21(f+3.);
    vec2 c2 = -.5 * v0    + 0.5*v2;
    vec2 c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    vec2 c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

float sdPoly(in vec2[maxN] v, int N, in vec2 p) {
    float d = dot(p-v[0], p-v[0]);
    float s = 1.0;
    for (int i = 0, j = N-1; i < N; j = i, i++) {
        vec2 e = v[j] - v[i];
        vec2 w = p - v[i];
        vec2 b = w - e * clamp(dot(w,e)/dot(e,e), 0.0, 1.0);
        d = min(d, dot(b,b));
        bvec3 c = bvec3(p.y>=v[i].y, p.y<v[j].y, e.x*w.y>e.y*w.x);
        if (all(c) || all(not(c))) { s *= -1.0; }
    }
    return s * sqrt(d);
}
```

#### Added from Subdivide (not in reference)

```glsl
float h12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
```

#### main() - subdivision loop

Initial quad covers full screen (no intro animation, no margins):

```glsl
void main() {
    vec2 R = resolution;
    vec2 fc = fragTexCoord * R;
    vec2 u = (fc + fc - R) / R.y;

    vec2 e;
    vec2[maxN] P = vec2[](
        vec2(-1, 1) * R / R.y,
        vec2( 1, 1) * R / R.y,
        vec2( 1,-1) * R / R.y,
        vec2(-1,-1) * R / R.y,
        e, e, e, e, e, e, e, e, e, e, e);
    int N = 4;
    float id = 0.0;

    float t = time;

    for (int i = 0; i < maxIterations; i++) {
        vec2 c = center(P, N);

        vec2 cr = catrom2(t);
        float d = sdPoly(P, N, c);
        vec2 p0 = c + (cr - 0.5) * d;

        float a = 2.0 * 6.28 * catrom1(t / 5.0);
        vec2 p1 = p0 + 2.0 * vec2(cos(a), sin(a));

        // First line-polygon intersection
        int i0; vec2 q0;
        for (int j = 0; j < N; j++) {
            vec2 a = P[j], b = P[j + 1];
            float s = C(p1 - p0, p0 - a) / C(p1 - p0, b - a);
            if (s >= 0.0 && s < 1.0) { i0 = j; q0 = mix(a, b, s); break; }
        }

        // Second line-polygon intersection
        int i1; vec2 q1;
        for (int j = i0 + 1; j < N; j++) {
            vec2 a = P[j], b = P[(j + 1) % N];
            float s = C(p1 - p0, p0 - a) / C(p1 - p0, b - a);
            if (s >= 0.0 && s < 1.0) { i1 = j; q1 = mix(a, b, s); break; }
        }

        // Ensure p1 is closer to q0 than q1
        int ti; vec2 tq;
        if (dot(p1 - q0, p1 - q0) > dot(p1 - q1, p1 - q1)) {
            ti = i0; i0 = i1; i1 = ti;
            tq = q0; q0 = q1; q1 = tq;
        }

        // Split polygon based on which side of the line u falls on
        vec2[maxN] Q;
        if (C(u - p0, p1 - p0) > 0.0) {
            int M = i0 < i1 ? (i1 - i0) + 2 : N - (i0 - i1) + 2;
            Q[0] = q0;
            for (int j = 1; j < M - 1; j++) { Q[j] = P[(i0 + j) % N]; }
            Q[M - 1] = q1;
            N = M;
            id += 1.0 / float(i + 1);
        } else {
            int M = i1 < i0 ? (i0 - i1) + 2 : N - (i1 - i0) + 2;
            Q[0] = q1;
            for (int j = 1; j < M - 1; j++) { Q[j] = P[(i1 + j) % N]; }
            Q[M - 1] = q0;
            N = M;
            id -= 1.0 / float(i + 1);
        }
        P = Q;

        t += 1e2 * (h11(id) - 0.5);

        // Procedural hash replaces noise texture; threshold replaces hardcoded 0.1
        float tx = h12(u * 0.11);
        if (h11(id + floor(t / 1.5 - tx / 8.0)) < threshold) { break; }
    }
```

#### main() - post-loop coloring

Bounding box area computed from final polygon vertices (not shoelace):

```glsl
    // Bounding box for area fade and edge diagonal
    vec2 bl = vec2(1e5);
    vec2 tr = vec2(-1e5);
    for (int j = 0; j < N; j++) {
        bl = min(bl, P[j]);
        tr = max(tr, P[j]);
    }
    float area = (tr.x - bl.x) * (tr.y - bl.y);

    float h = h11(id);

    // FFT brightness (same as subdivide.fs lines 122-140)
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

    // Edge darkening (two-pass sdPoly, matching subdivide.fs style)
    float l = length(tr - bl);
    col *= mix(1.0, 0.25 + 0.75 * S(sdPoly(P, N, u) + 0.0033), edgeDarken);
    col *= mix(1.0, 0.75 + 0.25 * S(sdPoly(P, N, u + 0.07 * l)), edgeDarken);

    // Desaturation gate (same as subdivide.fs lines 151-154)
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
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| speed | float | 0.1-2.0 | 0.45 | Yes | Speed |
| threshold | float | 0.01-0.9 | 0.15 | Yes | Threshold |
| maxIterations | int | 2-20 | 8 | No | Iterations |
| edgeDarken | float | 0.0-1.0 | 0.75 | Yes | Edge Darken |
| areaFade | float | 0.0001-0.005 | 0.0007 | Yes | Area Fade |
| desatThreshold | float | 0.0-1.0 | 0.5 | Yes | Desat Threshold |
| desatAmount | float | 0.0-1.0 | 0.9 | Yes | Desat Amount |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | (in output section) |

### Constants

- Enum: `TRANSFORM_POLYGON_SUBDIVIDE_BLEND`
- Display name: `"Polygon Subdivide"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `10` (Geometric)
- Field name: `polygonSubdivide`
- Macro: `REGISTER_GENERATOR`
- Blend default: `EFFECT_BLEND_SCREEN`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create polygon_subdivide.h

**Files**: `src/effects/polygon_subdivide.h` (create)
**Creates**: `PolygonSubdivideConfig`, `PolygonSubdivideEffect` types, lifecycle declarations

**Do**: Create the header with config struct, config fields macro, forward-declare `ColorLUT`, effect struct, and 4 function declarations. Follow exact struct layouts from Design > Types. Use `subdivide.h` as structural template (same includes: `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`).

**Verify**: `cmake.exe --build build` compiles with the new header included from nowhere yet.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create polygon_subdivide.cpp

**Files**: `src/effects/polygon_subdivide.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the implementation file. Use `subdivide.cpp` as structural template. Same includes (own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`).

Lifecycle functions:
- `PolygonSubdivideEffectInit`: Same pattern as `SubdivideEffectInit` - load shader, cache 16 uniform locations (all from Design > Types minus squishLoc), init gradientLUT, set time to 0. Cascade cleanup on failure.
- `PolygonSubdivideEffectSetup`: Same pattern as `SubdivideEffectSetup` - accumulate `time += speed * deltaTime`, update LUT, bind all uniforms. No squish uniform.
- `PolygonSubdivideEffectUninit`: Unload shader, free LUT.
- `PolygonSubdivideRegisterParams`: Register all 12 modulatable params (same as Subdivide minus squish). Use `"polygonSubdivide."` prefix. Include `blendIntensity` with range 0-5.

Bridge functions:
- `SetupPolygonSubdivide(PostEffect* pe)` - calls `PolygonSubdivideEffectSetup`
- `SetupPolygonSubdivideBlend(PostEffect* pe)` - calls `BlendCompositorApply`

UI section (`// === UI ===`):
- `static void DrawPolygonSubdivideParams(EffectConfig*, const ModSources*, ImU32)` - same layout as `DrawSubdivideParams` minus Squish slider. Three sections: Audio (Base Freq, Max Freq, Gain, Contrast, Base Bright), Geometry (Speed, Threshold, Iterations), Visual (Edge Darken, Area Fade, Desat Threshold, Desat Amount). `areaFade` uses `ModulatableSliderLog`. `maxIterations` uses `ImGui::SliderInt`.

Registration:
```
STANDARD_GENERATOR_OUTPUT(polygonSubdivide)
REGISTER_GENERATOR(TRANSFORM_POLYGON_SUBDIVIDE_BLEND, PolygonSubdivide, polygonSubdivide,
                   "Polygon Subdivide", SetupPolygonSubdivideBlend,
                   SetupPolygonSubdivide, 10, DrawPolygonSubdivideParams, DrawOutput_polygonSubdivide)
```

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.2: Create polygon_subdivide.fs

**Files**: `shaders/polygon_subdivide.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Create the fragment shader. Implement the Algorithm section from the Design verbatim. The shader file structure is:

1. Attribution comment block (from research Attribution section):
   ```
   // Based on "Convex Polygon Subdivision" by SnoopethDuckDuck
   // https://www.shadertoy.com/view/NfBGzd
   // License: CC BY-NC-SA 3.0 Unported
   // Modified: replaced palette with gradient LUT, added FFT audio reactivity,
   // exposed config uniforms, removed intro animation and vignette
   ```
2. `#version 330`
3. Inputs/outputs (`fragTexCoord`, `finalColor`)
4. All uniforms from Design > Algorithm > Uniforms
5. Macros and constants (`C`, `S`, `maxN`)
6. Helper functions in order: `center`, `h11`, `h12`, `h21`, `catrom1`, `catrom2`, `sdPoly`
7. `main()` from Design > Algorithm > main() sections (subdivision loop + post-loop coloring)

Transcribe the Algorithm section mechanically. Do not reinterpret formulas.

**Verify**: File exists and has correct `#version 330` header.

---

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Four changes:
1. Add `#include "effects/polygon_subdivide.h"` with the other effect includes
2. Add `TRANSFORM_POLYGON_SUBDIVIDE_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
3. Add `TRANSFORM_POLYGON_SUBDIVIDE_BLEND` to `TransformOrderConfig::order` array (at end, before closing brace)
4. Add `PolygonSubdivideConfig polygonSubdivide;` to `EffectConfig` struct

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Two changes:
1. Add `#include "effects/polygon_subdivide.h"` with the other effect includes
2. Add `PolygonSubdivideEffect polygonSubdivide;` to `PostEffect` struct (near the existing `SubdivideEffect subdivide;` member)

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.5: Add to CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/polygon_subdivide.cpp` to `EFFECTS_SOURCES` list (alphabetical order near `src/effects/polymorph.cpp`).

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.6: Register serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Three changes:
1. Add `#include "effects/polygon_subdivide.h"` with the other effect includes
2. Add JSON macro: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PolygonSubdivideConfig, POLYGON_SUBDIVIDE_CONFIG_FIELDS)` near the existing Subdivide entry
3. Add `X(polygonSubdivide) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles after all Wave 2 tasks complete.

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe --build build`
- [ ] Effect appears in Geometric section of the effects panel
- [ ] Enabling shows stained-glass polygon pattern
- [ ] Polygons are irregular convex N-gons, not axis-aligned quads
- [ ] FFT audio drives per-cell brightness
- [ ] Gradient LUT colors cells
- [ ] Preset save/load round-trips all parameters
