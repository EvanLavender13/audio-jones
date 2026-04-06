# Infinity Matrix

Infinite recursive fractal zoom through self-similar binary digit glyphs, with gradient LUT coloring and FFT audio reactivity per recursion depth. Each lit pixel of a glyph contains a complete smaller glyph grid; the view zooms endlessly inward along a deterministic random path. Shallow recursion layers react to bass, deep layers to treble.

**Research**: `docs/research/infinity_matrix.md`

## Design

### Types

**InfinityMatrixConfig** (in `infinity_matrix.h`):

```
bool enabled = false

// Zoom
float zoomSpeed = 1.0f        // Zoom rate in levels/s (0.1-3.0)
float zoomScale = 0.1f        // Base zoom multiplier (0.01-1.0)
int recursionDepth = 5         // Fractal levels to render (2-8)
float fadeDepth = 3.0f         // Levels visible simultaneously before fading (1.0-6.0)

// Wave
float waveAmplitude = 0.1f    // UV wave distortion amount (0.0-0.5)
float waveFreq = 2.0f         // UV wave spatial frequency (0.5-8.0)
float waveSpeed = 1.0f        // UV wave animation rate (0.1-3.0)

// Audio
bool colorFreqMap = false     // Map FFT bands to glyph hash color instead of depth
float baseFreq = 55.0f        // Low FFT frequency (27.5-440)
float maxFreq = 14000.0f      // High FFT frequency (1000-16000)
float gain = 2.0f             // FFT sensitivity (0.1-10)
float curve = 1.5f            // FFT response curve (0.1-3)
float baseBright = 0.15f      // Floor brightness without audio (0-1)

// Output
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**InfinityMatrixEffect** (in `infinity_matrix.h`):

```
Shader shader
ColorLUT *gradientLUT
float zoomPhase        // CPU-accumulated: zoomPhase += zoomSpeed * dt
float wavePhase        // CPU-accumulated: wavePhase += waveSpeed * dt
int resolutionLoc
int fftTextureLoc
int sampleRateLoc
int gradientLUTLoc
int zoomPhaseLoc
int zoomScaleLoc
int recursionDepthLoc
int fadeDepthLoc
int waveAmplitudeLoc
int waveFreqLoc
int wavePhaseLoc
int colorFreqMapLoc
int baseFreqLoc
int maxFreqLoc
int gainLoc
int curveLoc
int baseBrightLoc
```

**CONFIG_FIELDS macro**: `enabled, zoomSpeed, zoomScale, recursionDepth, fadeDepth, waveAmplitude, waveFreq, waveSpeed, colorFreqMap, baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity`

### Algorithm

The shader is a mechanical transcription of the reference code from the research doc, applying the Keep/Replace substitution table. Everything below marked **(verbatim)** is kept exactly from reference. Everything else applies the documented replacements.

#### Constants and hashes (verbatim)

```glsl
const int glyphSize = 5;
const int glyphCount = 2;
const float glyphMargin = 0.5;
const float glyphSizeF = float(glyphSize) + 2.0 * glyphMargin;
const float glyphSizeLog = log(glyphSizeF);
const int powTableCount = 10;
const float gsfi = 1.0 / glyphSizeF;
const float powTable[powTableCount] = float[](
    1.0, gsfi, pow(gsfi, 2.0), pow(gsfi, 3.0), pow(gsfi, 4.0),
    pow(gsfi, 5.0), pow(gsfi, 6.0), pow(gsfi, 7.0), pow(gsfi, 8.0), pow(gsfi, 9.0)
);
const float E = 2.718281828459;

const int glyphs[glyphSize * glyphCount] = int[](
    0x01110, 0x01110,
    0x11011, 0x11110,
    0x11011, 0x01110,
    0x11011, 0x01110,
    0x01110, 0x11111
);

float RandFloat(int i) { return fract(sin(float(i)) * 43758.5453); }
int RandInt(int i) { return int(100000.0 * RandFloat(i)); }
```

#### Glyph functions (verbatim)

```glsl
int GetFocusGlyph(int i) { return RandInt(i) % glyphCount; }
int GetGlyphPixelRow(int y, int g) { return glyphs[g + (glyphSize - 1 - y) * glyphCount]; }

int GetGlyphPixel(ivec2 pos, int g) {
    if (pos.x >= glyphSize || pos.y >= glyphSize) { return 0; }
    int glyphRow = GetGlyphPixelRow(pos.y, g);
    return 1 & (glyphRow >> (glyphSize - 1 - pos.x) * 4);
}
```

#### Focus path computation (verbatim)

```glsl
ivec2 focusList[max(powTableCount, 8) + 2]; // 8 = max recursionDepth uniform
ivec2 GetFocusPos(int i) { return focusList[i + 2]; }

ivec2 CalculateFocusPos(int iterations) {
    int g = GetFocusGlyph(iterations - 1);
    int c = 18; // both glyphs have 18 lit pixels
    c -= RandInt(iterations) % c;
    for (int y = glyphCount * (glyphSize - 1); y >= 0; y -= glyphCount) {
        int glyphRow = glyphs[g + y];
        for (int x = 0; x < glyphSize; ++x) {
            c -= (1 & (glyphRow >> 4 * x));
            if (c == 0) {
                return ivec2(glyphSize - 1 - x, glyphSize - 1 - y / glyphCount);
            }
        }
    }
    return ivec2(0); // unreachable
}
```

Note: The `focusList` array size uses literal `8` (the max value of the `recursionDepth` uniform) instead of a const, because GLSL 330 array sizes must be compile-time constant. The `max()` in the array size is a const-expression.

#### GetGlyph (verbatim)

```glsl
int GetGlyph(int iterations, ivec2 glyphPos, int glyphLast,
             ivec2 glyphPosLast, ivec2 focusPos) {
    if (glyphPos == focusPos) { return GetFocusGlyph(iterations); }
    int seed = iterations + glyphPos.x * 313 + glyphPos.y * 411
             + glyphPosLast.x * 557 + glyphPosLast.y * 121;
    return RandInt(seed) % glyphCount;
}
```

#### GetRecursionFade (replace const with uniform)

```glsl
float GetRecursionFade(int r, float timePercent) {
    if (r > recursionDepth) { return timePercent; }
    float rt = max(float(r) - timePercent - fadeDepth, 0.0);
    float rc = float(recursionDepth) - fadeDepth;
    return rt / rc;
}
```

Replaces `recursionCount` with `recursionDepth` uniform and `recursionFadeDepth` with `fadeDepth` uniform.

#### GetPixelFractal (replace CombinePixelColor with gradient LUT + FFT)

```glsl
vec3 GetPixelFractal(vec2 pos, int iterations, float timePercent) {
    int glyphLast = GetFocusGlyph(iterations - 1);
    ivec2 glyphPosLast = GetFocusPos(-2);
    ivec2 glyphPos = GetFocusPos(-1);

    bool isFocus = true;
    ivec2 focusPos = glyphPos;

    vec3 color = vec3(0.0);
    for (int r = 0; r <= recursionDepth + 1; ++r) {
        // --- Replacement for CombinePixelColor ---
        float t;
        if (colorFreqMap == 0) {
            // Depth mode: recursion level drives color and FFT
            t = float(r) / float(recursionDepth);
        } else {
            // Hash mode: glyph position hash drives color and FFT
            int seed = iterations + r + 11 * glyphPosLast.x + 13 * glyphPosLast.y
                     + 17 * glyphPos.x + 19 * glyphPos.y;
            t = RandFloat(seed);
        }
        vec3 lutColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        float f = GetRecursionFade(r, timePercent);
        color += lutColor * brightness * f;
        // --- End replacement ---

        if (r > recursionDepth) { return color; }

        // Update pos (verbatim)
        pos -= vec2(glyphMargin * gsfi);
        pos *= glyphSizeF;

        glyphPosLast = glyphPos;
        glyphPos = ivec2(pos);

        int glyphValue = GetGlyphPixel(glyphPos, glyphLast);
        if (glyphValue == 0 || pos.x < 0.0 || pos.y < 0.0) { return color; }

        pos -= vec2(floor(pos));
        focusPos = isFocus ? GetFocusPos(r) : ivec2(-10);
        glyphLast = GetGlyph(iterations + r, glyphPos, glyphLast, glyphPosLast, focusPos);
        isFocus = isFocus && (glyphPos == focusPos);
    }
    return color; // loop always returns early, but compiler needs this
}
```

#### main (replace iTime/iResolution, add wave, remove FinishPixel)

```glsl
void main() {
    // Center UV, normalize by height
    vec2 uv = fragTexCoord * resolution - resolution * 0.5;
    uv /= resolution.y;

    // Wave distortion (replaces InitUV)
    uv.x += waveAmplitude * sin(waveFreq * uv.y + wavePhase);
    uv.y += waveAmplitude * sin(waveFreq * uv.x + 0.8 * wavePhase);

    // zoomPhase accumulated on CPU (replaces iTime * zoomSpeed)
    float timePercent = zoomPhase;
    int iterations = int(floor(timePercent));
    timePercent -= float(iterations);

    // Zoom (verbatim formula)
    float zoom = pow(E, -glyphSizeLog * timePercent) * zoomScale;

    // Cache focus positions (verbatim)
    for (int i = 0; i < powTableCount + 2; ++i) {
        focusList[i] = CalculateFocusPos(iterations + i - 2);
    }

    // Offset (verbatim)
    vec2 offset = vec2(0.0);
    for (int i = 0; i < powTableCount; ++i) {
        offset += ((vec2(GetFocusPos(i)) + vec2(glyphMargin)) * gsfi) * powTable[i];
    }

    // Apply zoom & offset (verbatim)
    vec2 uvFractal = uv * zoom + offset;

    vec3 pixelColor = GetPixelFractal(uvFractal, iterations, timePercent);

    // FinishPixel removed entirely - no green tint, no noise
    finalColor = vec4(pixelColor, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| zoomSpeed | float | -3.0-3.0 | 1.0 | Yes | Zoom Speed |
| zoomScale | float | 0.01-1.0 | 0.1 | Yes | Zoom Scale |
| recursionDepth | int | 2-8 | 5 | No | Recursion | <!-- Intentional deviation: int SliderInt, not modulatable. Performance scales with loop iterations. Matches Color Stretch pattern. -->
| fadeDepth | float | 1.0-6.0 | 3.0 | Yes | Fade Depth |
| waveAmplitude | float | 0.0-0.5 | 0.1 | Yes | Wave Amplitude |
| waveFreq | float | 0.5-8.0 | 2.0 | Yes | Wave Freq |
| waveSpeed | float | 0.1-3.0 | 1.0 | Yes | Wave Speed |
| colorFreqMap | bool | - | false | No | Color Freq Map |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |

### Constants

- Enum name: `TRANSFORM_INFINITY_MATRIX_BLEND`
- Display name: `"Infinity Matrix"`
- Category badge: `"GEN"` (auto from `REGISTER_GENERATOR`)
- Section index: 12 (Texture)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create infinity_matrix.h

**Files**: `src/effects/infinity_matrix.h` (create)
**Creates**: Config struct and Effect struct that all other files depend on

**Do**: Create the header following the `color_stretch.h` pattern. Define `InfinityMatrixConfig` with all fields from the Design section. Define `InfinityMatrixEffect` with all loc fields. Define `INFINITY_MATRIX_CONFIG_FIELDS` macro. Declare Init (takes `Effect*` and `const Config*`), Setup (takes `Effect*`, `const Config*`, `float deltaTime`, `const Texture2D& fftTexture`), Uninit, and RegisterParams. Include `"render/color_config.h"` for `ColorConfig` and `"render/blend_mode.h"` for `EffectBlendMode`.

**Verify**: `cmake.exe --build build` compiles (header-only, no link).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create infinity_matrix.cpp

**Files**: `src/effects/infinity_matrix.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow the `color_stretch.cpp` pattern exactly.

- **Init**: Load `shaders/infinity_matrix.fs`, cache all uniform locations listed in `InfinityMatrixEffect`, init `ColorLUT` from `cfg->gradient`, set `zoomPhase = 0`, `wavePhase = 0`. On shader fail return false. On LUT fail, unload shader and return false.
- **Setup**: Accumulate `zoomPhase += cfg->zoomSpeed * deltaTime` and `wavePhase += cfg->waveSpeed * deltaTime`. Call `ColorLUTUpdate`. Bind all uniforms: resolution, fftTexture, sampleRate, gradientLUT, zoomPhase, zoomScale, recursionDepth (int), fadeDepth, waveAmplitude, waveFreq, wavePhase, colorFreqMap (bool->int, same pattern as `cyber_march.cpp`), baseFreq, maxFreq, gain, curve, baseBright.
- **Uninit**: Unload shader, `ColorLUTUninit`.
- **RegisterParams**: Register all modulatable params from the Parameters table. Field name prefix: `"infinityMatrix."`. Include `blendIntensity` (0-5).
- **SetupInfinityMatrix** (non-static bridge): Delegates to `InfinityMatrixEffectSetup`, same pattern as `SetupColorStretch`.
- **SetupInfinityMatrixBlend** (non-static bridge): Calls `BlendCompositorApply` with `pe->generatorScratch.texture`, `blendIntensity`, `blendMode`.
- **UI section**: `DrawInfinityMatrixParams` (static). Section order: Audio (`Checkbox("Color Freq Map##infinityMatrix")` then standard 5 sliders), Geometry (`SliderInt` for recursionDepth 2-8, `ModulatableSlider` for fadeDepth and zoomScale), Wave (`ModulatableSlider` for waveAmplitude, waveFreq, waveSpeed), Animation (`ModulatableSlider` for zoomSpeed).
- **Registration**: `STANDARD_GENERATOR_OUTPUT(infinityMatrix)` then `REGISTER_GENERATOR(TRANSFORM_INFINITY_MATRIX_BLEND, InfinityMatrix, infinityMatrix, "Infinity Matrix", SetupInfinityMatrixBlend, SetupInfinityMatrix, 12, DrawInfinityMatrixParams, DrawOutput_infinityMatrix)`.
- Includes: same set as `color_stretch.cpp` plus `"audio/audio.h"` for `AUDIO_SAMPLE_RATE`.

**Verify**: Compiles (needs Tasks 2.2, 2.3, 2.4 for link).

#### Task 2.2: Create infinity_matrix.fs shader

**Files**: `shaders/infinity_matrix.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from this plan document. Attribution block at top (Frank Force, CC BY-NC-SA 3.0, shadertoy.com/view/Md2fRR). Uniforms: `resolution` (vec2), `fftTexture` (sampler2D), `sampleRate` (float), `gradientLUT` (sampler2D), `zoomPhase` (float), `zoomScale` (float), `recursionDepth` (int), `fadeDepth` (float), `waveAmplitude` (float), `waveFreq` (float), `wavePhase` (float), `colorFreqMap` (int), `baseFreq` (float), `maxFreq` (float), `gain` (float), `curve` (float), `baseBright` (float). Transcribe the Algorithm section mechanically - every (verbatim) block is copied exactly, every replacement block uses the GLSL shown in the Algorithm.

**Verify**: Shader file exists with correct uniforms matching the .h loc fields.

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/infinity_matrix.h"` with other effect includes (alphabetical).
2. Add `TRANSFORM_INFINITY_MATRIX_BLEND,` to `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE`.
3. Add `TRANSFORM_INFINITY_MATRIX_BLEND` to `TransformOrderConfig::order` array (end, before closing brace).
4. Add `InfinityMatrixConfig infinityMatrix;` to `EffectConfig` struct.

**Verify**: Compiles.

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/infinity_matrix.h"` with other effect includes.
2. Add `InfinityMatrixEffect infinityMatrix;` member to `PostEffect` struct (near `ColorStretchEffect colorStretch`).

**Verify**: Compiles.

#### Task 2.5: Build system and serialization

**Files**: `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. In `CMakeLists.txt`: Add `src/effects/infinity_matrix.cpp` to `EFFECTS_SOURCES` (alphabetical).
2. In `effect_serialization.cpp`: Add `#include "effects/infinity_matrix.h"`. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InfinityMatrixConfig, INFINITY_MATRIX_CONFIG_FIELDS)` with the other config macros. Add `X(infinityMatrix) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Full build succeeds with no warnings: `cmake.exe --build build`.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Effect appears in Effects window under Texture category with "GEN" badge
- [x] Enabling the effect renders fractal binary digit zoom
- [x] Zoom speed and wave controls animate the view
- [x] FFT audio drives per-recursion brightness (bass on outer, treble on inner)
- [x] Gradient LUT colors each recursion layer
- [ ] Preset save/load round-trips all parameters
- [ ] Modulation routes to registered parameters

## Implementation Notes

- `zoomSpeed` range widened from 0.1-3.0 to -3.0-3.0 post-implementation to allow reverse zoom (zooming outward through fractal layers). The original Shadertoy reference also supports negative zoom speed.
- FFT reactivity per recursion level works but is subtle due to the additive layer accumulation and the fade function gating how many layers are visible at once. This is inherent to the fractal structure - each pixel only shows a few active layers at any time.
