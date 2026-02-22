# Scrawl

Thick flowing marker curves from an IFS fractal fold, rendered as bold glowing strokes over a gritty scanline texture. Neon graffiti on a weathered steel wall — tangled curves that drift and evolve with the music. Single-pass fragment shader generator blended via blend compositor.

**Research**: `docs/research/scrawl.md`

## Design

### Types

**ScrawlConfig** (`src/effects/scrawl.h`):

```
struct ScrawlConfig {
  bool enabled = false;

  // Geometry
  int iterations = 6;          // Fold depth / curve density (2-12)
  float foldOffset = 1.2f;     // Fold attractor shift (0.5-2.0)
  float tileScale = 0.5f;      // Tile repetition density (0.2-2.0)
  float zoom = 4.0f;           // Overall view scale (1.0-8.0)
  float warpFreq = 20.0f;      // Sinusoidal wobble frequency (5.0-40.0)
  float warpAmp = 0.3f;        // Sinusoidal wobble amplitude (0.0-0.5)

  // Rendering
  float thickness = 0.03f;     // Stroke width (0.005-0.15)
  float glowIntensity = 1.5f;  // Stroke brightness (0.5-5.0)
  float scanlineDensity = 50.0f;  // Scanline count (10.0-100.0)
  float scanlineStrength = 0.3f;  // Scanline darkness (0.0-0.8)

  // Animation
  float scrollSpeed = 0.3f;    // Horizontal drift speed (-1.0..1.0)
  float evolveSpeed = 0.2f;    // Fold parameter evolution speed (-1.0..1.0)
  float rotationSpeed = 0.0f;  // Global rotation rate rad/s

  // Audio
  float baseFreq = 55.0f;      // Lowest FFT band Hz (27.5-440.0)
  float maxFreq = 14000.0f;    // Highest FFT band Hz (1000-16000)
  float gain = 2.0f;           // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.0f;          // Contrast exponent (0.1-3.0)
  float baseBright = 0.2f;     // Minimum brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

Fields macro: `SCRAWL_CONFIG_FIELDS` = `enabled, iterations, foldOffset, tileScale, zoom, warpFreq, warpAmp, thickness, glowIntensity, scanlineDensity, scanlineStrength, scrollSpeed, evolveSpeed, rotationSpeed, baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity`

**ScrawlEffect** (`src/effects/scrawl.h`):

```
typedef struct ScrawlEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float scrollAccum;
  float evolveAccum;
  float rotationAccum;
  int resolutionLoc;
  int fftTextureLoc, sampleRateLoc;
  int baseFreqLoc, maxFreqLoc, gainLoc, curveLoc, baseBrightLoc;
  int iterationsLoc, foldOffsetLoc, tileScaleLoc, zoomLoc;
  int warpFreqLoc, warpAmpLoc;
  int thicknessLoc, glowIntensityLoc;
  int scanlineDensityLoc, scanlineStrengthLoc;
  int scrollAccumLoc, evolveAccumLoc, rotationAccumLoc;
  int gradientLUTLoc;
} ScrawlEffect;
```

### Algorithm

The shader is a single-pass IFS fold with tracked orbit trap, FFT-driven per-layer glow, gradient LUT coloring, and scanline overlay. Structural template: `shaders/motherboard.fs`.

#### Coordinate setup

```glsl
vec2 p = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

// Scale
p *= zoom;

// Horizontal scroll (CPU-accumulated, scrolls before tiling so pattern drifts)
p.x += scrollAccum;

// Tile (centered tiles)
p = fract(p * tileScale) - 0.5;

// Global rotation (CPU-accumulated, rotates the tiled cells)
float cr = cos(rotationAccum), sr = sin(rotationAccum);
p *= mat2(cr, -sr, sr, cr);
```

#### IFS fold loop

The core fold is from the reference shader — division by `clamp(abs(p.x*p.y), 0.5, 1.0)` creates curved (not angular) structures:

```glsl
float m = 1000.0;
int winIt = 0;

for (int i = 0; i < iterations; i++) {
    p = abs(p) / clamp(abs(p.x * p.y), 0.5, 1.0) - 1.0 - foldOffset - evolveAccum;

    // Sinusoidal distance warping — hand-drawn wobble via triangle-wave approximation
    float l = abs(p.x + asin(0.9 * sin(length(p) * warpFreq)) * warpAmp);
    if (l < m) {
        m = l;
        winIt = i;
    }
}
```

Key differences from Motherboard's fold:
- Motherboard: `p = abs(p); p = p / clamp(abs(p.x*p.y), clampLo, clampHi) - foldConstant; p *= foldRot;`
- Scrawl: same division structure but with fixed clamp range (0.5, 1.0), `1.0 + foldOffset + evolveAccum` as the subtraction constant (the 1.0 comes from the reference shader), and sinusoidal distance warping on the orbit trap measurement. No per-iteration rotation.

#### Rendering

Combine smoothstep thickness with glow falloff:

```glsl
float stroke = smoothstep(thickness, 0.0, m) * glowIntensity / max(abs(m), 0.001);
```

#### FFT mapping

Same pattern as Motherboard — winning iteration maps to a frequency band:

```glsl
const int BAND_SAMPLES = 4;
float t0 = float(winIt) / float(iterations);
float t1 = float(winIt + 1) / float(iterations);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float energy = 0.0;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
}
energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + energy;
```

#### Color + scanlines + output

```glsl
vec3 layerColor = texture(gradientLUT, vec2((float(winIt) + 0.5) / float(iterations), 0.5)).rgb;
vec3 color = stroke * layerColor * brightness;

// Scanline overlay — darkens alternating horizontal bands
color *= 1.0 - scanlineStrength * step(0.5, fract(p.y * scanlineDensity));

finalColor = vec4(color, 1.0);
```

Note: the `p.y` in the scanline `fract()` uses the post-fold `p`, which gives the scanlines fractal structure rather than uniform horizontal bands.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| iterations | int | 2-12 | 6 | No (SliderInt) | Iterations |
| foldOffset | float | 0.5-2.0 | 1.2 | Yes | Fold Offset |
| tileScale | float | 0.2-2.0 | 0.5 | Yes | Tile Scale |
| zoom | float | 1.0-8.0 | 4.0 | Yes | Zoom |
| warpFreq | float | 5.0-40.0 | 20.0 | Yes | Warp Freq |
| warpAmp | float | 0.0-0.5 | 0.3 | Yes | Warp Amp |
| thickness | float | 0.005-0.15 | 0.03 | Yes | Thickness |
| glowIntensity | float | 0.5-5.0 | 1.5 | Yes | Glow Intensity |
| scanlineDensity | float | 10.0-100.0 | 50.0 | Yes | Scanline Density |
| scanlineStrength | float | 0.0-0.8 | 0.3 | Yes | Scanline Strength |
| scrollSpeed | float | -1.0-1.0 | 0.3 | Yes | Scroll Speed |
| evolveSpeed | float | -1.0-1.0 | 0.2 | Yes | Evolve Speed |
| rotationSpeed | float | -PI..PI | 0.0 | Yes (SpeedDeg) | Rotation Speed |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.0 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.2 | Yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |
| blendMode | enum | — | SCREEN | No (Combo) | Blend Mode |
| gradient | ColorConfig | — | gradient | No (widget) | Color section |

### Constants

- Enum: `TRANSFORM_SCRAWL_BLEND`
- Display name: `"Scrawl Blend"`
- Category: `"GEN"` (section 10)
- Flags: `EFFECT_FLAG_BLEND` (auto via `REGISTER_GENERATOR`)
- Macro: `REGISTER_GENERATOR`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create ScrawlConfig and ScrawlEffect

**Files**: `src/effects/scrawl.h` (create)
**Creates**: Config struct, Effect struct, function declarations that all other tasks depend on

**Do**: Create `src/effects/scrawl.h` following the Design > Types section exactly. Follow Motherboard's header structure:
1. Include guards, `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`
2. `ScrawlConfig` struct with all fields and defaults from Design > Types
3. `#define SCRAWL_CONFIG_FIELDS` macro listing all serializable fields
4. Forward declare `ColorLUT`
5. `ScrawlEffect` typedef struct with shader, gradientLUT pointer, three accumulators, and all uniform location ints
6. Five function declarations: Init, Setup, Uninit, ConfigDefault, RegisterParams
7. Init signature: `bool ScrawlEffectInit(ScrawlEffect *e, const ScrawlConfig *cfg)` (takes config for LUT init)
8. Setup signature: `void ScrawlEffectSetup(ScrawlEffect *e, const ScrawlConfig *cfg, float deltaTime, Texture2D fftTexture)`

**Verify**: `cmake.exe --build build` compiles (header-only, just needs to parse).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect Module Implementation

**Files**: `src/effects/scrawl.cpp` (create)
**Depends on**: Wave 1 (needs `scrawl.h`)

**Do**: Create `src/effects/scrawl.cpp` following Motherboard's `.cpp` structure exactly:
1. Includes: own header, `"audio/audio.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `<stddef.h>`
2. `ScrawlEffectInit`: load `"shaders/scrawl.fs"`, cache all uniform locations, init `ColorLUT` via `ColorLUTInit(&cfg->gradient)`, init three accumulators to 0. On LUT fail, unload shader and return false.
3. `ScrawlEffectSetup`: accumulate `scrollAccum += cfg->scrollSpeed * deltaTime`, `evolveAccum += cfg->evolveSpeed * deltaTime`, `rotationAccum += cfg->rotationSpeed * deltaTime`. Call `ColorLUTUpdate`. Bind all uniforms (resolution, fftTexture, sampleRate, all config fields, three accumulators, gradientLUT texture).
4. `ScrawlEffectUninit`: unload shader, `ColorLUTUninit`.
5. `ScrawlConfigDefault`: return `ScrawlConfig{}`.
6. `ScrawlRegisterParams`: register all float config fields with ranges from Design > Parameters. Use `ROTATION_SPEED_MAX` for rotationSpeed bounds.
7. `SetupScrawl` bridge: delegates to `ScrawlEffectSetup(&pe->scrawl, &pe->effects.scrawl, pe->currentDeltaTime, pe->fftTexture)`.
8. `SetupScrawlBlend` bridge: delegates to `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.scrawl.blendIntensity, pe->effects.scrawl.blendMode)`.
9. `REGISTER_GENERATOR(TRANSFORM_SCRAWL_BLEND, Scrawl, scrawl, "Scrawl Blend", SetupScrawlBlend, SetupScrawl)` at bottom wrapped in clang-format off/on.

**Verify**: Compiles (shader doesn't need to exist for C++ compilation).

#### Task 2.2: Fragment Shader

**Files**: `shaders/scrawl.fs` (create)
**Depends on**: Wave 1 (needs to know uniform names)

**Do**: Implement the shader from the Design > Algorithm section. Structure:
1. `#version 330`, in/out, all uniforms matching the ScrawlEffect location names
2. `const int BAND_SAMPLES = 4;`
3. `void main()`: coordinate setup, IFS fold loop, rendering (smoothstep + glow), FFT mapping, gradient LUT color, scanline overlay, output
4. Copy each section from Design > Algorithm verbatim — coordinate setup, IFS fold loop, rendering, FFT mapping, color + scanlines + output

Top-of-file comment: `// Scrawl: IFS fractal fold with thick marker strokes, scanline grit, and FFT-driven layer glow.`

**Verify**: Shader is syntactically valid GLSL 330 (will be tested at runtime).

#### Task 2.3: Config Registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 (needs `scrawl.h`)

**Do**:
1. Add `#include "effects/scrawl.h"` in the alphabetical include block (after `scan_bars.h`, before `shake.h`)
2. Add `TRANSFORM_SCRAWL_BLEND,` to enum before `TRANSFORM_EFFECT_COUNT`
3. Add `ScrawlConfig scrawl;` to `EffectConfig` struct (after motherboard, before dot matrix: `// Scrawl (IFS marker curves with scanline grit)`)

Note: the order array auto-initializes from 0..COUNT-1 in the constructor, so adding the enum entry is sufficient.

**Verify**: Compiles.

#### Task 2.4: PostEffect Struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 (needs `scrawl.h`)

**Do**:
1. Add `#include "effects/scrawl.h"` in the alphabetical include block
2. Add `ScrawlEffect scrawl;` member in the PostEffect struct (alphabetical placement near other effects)

**Verify**: Compiles.

#### Task 2.5: Build System

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/scrawl.cpp` to `EFFECTS_SOURCES` in alphabetical position (after `scan_bars.cpp`, before `shake.cpp`).

**Verify**: Compiles.

#### Task 2.6: Serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 (needs `SCRAWL_CONFIG_FIELDS` macro)

**Do**:
1. Add `#include "effects/scrawl.h"` in the alphabetical include block
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ScrawlConfig, SCRAWL_CONFIG_FIELDS)` with the other config macros (alphabetical)
3. Add `X(scrawl)` to `EFFECT_CONFIG_FIELDS(X)` macro (after `X(scanBars)`)

**Verify**: Compiles.

#### Task 2.7: UI Panel

**Files**: `src/ui/imgui_effects_gen_texture.cpp` (modify)
**Depends on**: Wave 1 (needs `scrawl.h`)

**Do**: Add Scrawl UI section following Motherboard's pattern exactly:
1. Add `#include "effects/scrawl.h"` at top with other effect includes
2. Add `static bool sectionScrawl = false;` with other section bools
3. Create `static void DrawGeneratorsScrawl(EffectConfig*, const ModSources*, ImU32)`:
   - `DrawSectionBegin("Scrawl", ...)` with `sectionScrawl` and `e->scrawl.enabled`
   - Enable checkbox + `MoveTransformToEnd` pattern using `TRANSFORM_SCRAWL_BLEND`
   - **Audio** section: Base Freq, Max Freq, Gain, Contrast, Base Bright (standard order/labels/formats from conventions)
   - **Geometry** section: Iterations (`SliderInt`, 2-12), Fold Offset, Tile Scale, Zoom, Warp Freq, Warp Amp
   - **Rendering** section: Thickness (`ModulatableSliderLog`), Glow Intensity, Scanline Density, Scanline Strength
   - **Animation** section: Scroll Speed, Evolve Speed, Rotation Speed (`ModulatableSliderSpeedDeg`)
   - Color widget: `ImGuiDrawColorMode(&cfg->gradient)`
   - **Output** section: Blend Intensity, Blend Mode combo
4. Add call in `DrawGeneratorsTextureCategory()`: `ImGui::Spacing(); DrawGeneratorsScrawl(e, modSources, categoryGlow);` after the Motherboard call

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Scrawl appears in generator list under Texture sub-category
- [ ] Enabling Scrawl shows marker curve visuals
- [ ] Scroll, evolve, and rotation animate smoothly
- [ ] Gradient LUT colors the layers
- [ ] Blend mode and intensity work
- [ ] Preset save/load round-trips all parameters
- [ ] Modulation routes to registered parameters

---

## Implementation Notes

The plan's algorithm diverged significantly from the reference shader. The following changes were made during implementation to stay faithful to the reference.

### Removed features (not in reference)

- **FFT audio reactivity**: The reference has no per-layer FFT glow. Removed `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright` config fields, all FFT uniform locations, FFT texture binding, and the entire FFT mapping shader section. No Audio section in UI.
- **Scanlines**: The reference uses a drawing mask (`step()`) for the grunge aesthetic, not scanline overlay. Removed `scanlineDensity`, `scanlineStrength` config fields and shader section.
- **Glow falloff**: Reference uses `smoothstep` stroke rendering directly, not the `smoothstep * glowIntensity / max(abs(m), 0.001)` pattern the plan borrowed from other generators.

### Changed algorithm (matching reference)

- **Coordinate setup**: Reference uses `uv - 0.5` with aspect correction (not the `fragTexCoord * resolution - resolution * 0.5` normalization). No perspective `p /= 1.0 - p.y` warp (kept simpler).
- **Breathing zoom**: `uv *= zoom + asin(0.9 * sin(evolveAccum)) * 0.2` — evolveAccum drives a breathing scale modulation matching reference's `p *= 0.3 + asin(0.9*sin(iTime*0.5))*0.2`.
- **Tiling**: `fract(uv * tileScale)` without centering (`- 0.5` removed), matching reference's `fract(p * 0.5)`.
- **Fold constant**: `1.0 + foldOffset` (no evolveAccum in fold subtraction — evolve drives the breathing zoom instead).
- **Drawing mask**: `stroke *= step(p2.x + float(winIt) * 0.1 - 0.8 + sin(uv.y * 2.0) * 0.1, 0.0)` — progressive reveal from reference that creates the growing-line graffiti effect.
- **Stroke rendering**: `smoothstep(thickness * 1.5, thickness, m * 0.5)` — two-threshold smoothstep matching reference's `smoothstep(.015, .01, m*.5)`.

### Changed defaults and ranges

| Field | Plan | Implemented | Reason |
|-------|------|-------------|--------|
| zoom | 4.0 (1.0-8.0) | 0.3 (0.1-1.0) | Reference uses 0.3 base scale with asin breathing |
| foldOffset | 1.2 (0.5-2.0) | 0.5 (0.0-1.5) | Reference uses `mod(floor(t), 3) * 0.5` ≈ 0-1.5 range |
| thickness | 0.03 (0.005-0.15) | 0.015 (0.005-0.05) | Reference uses 0.01-0.015 range |
| tileScale | — | — | Not modulatable in UI (plain SliderFloat) |
