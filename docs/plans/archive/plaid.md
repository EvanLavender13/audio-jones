# Plaid

Woven tartan fabric generator with colored bands crossing at right angles, twill thread-over-thread texture at intersections, and FFT-driven per-band glow. Band widths morph continuously — wide ground bands pulse to bass, thin accent stripes flicker to treble. Colors come from the gradient LUT.

**Research**: `docs/research/plaid.md`

## Design

### Types

**PlaidConfig** (in `src/effects/plaid.h`):

```
bool enabled = false

// Fabric
float scale = 2.0f          // Tiles per screen (0.5-8.0)
int bandCount = 5            // Unique bands per sett half, mirrored to 2N (3-8)
float accentWidth = 0.15f    // Thin accent stripe width relative to wide bands (0.05-0.5)
float threadDetail = 128.0f  // Twill texture fineness (16.0-512.0)
int twillRepeat = 4          // Twill over/under repeat count (2-8)

// Animation
float morphSpeed = 0.3f      // Band width oscillation speed (0.0-2.0)
float morphAmount = 0.3f     // Strength of width morphing (0.0-1.0)

// Glow
float glowIntensity = 1.0f   // Overall brightness multiplier (0.0-2.0)

// FFT
float baseFreq = 55.0f       // Lowest FFT frequency Hz (27.5-440.0)
float maxFreq = 14000.0f     // Highest FFT frequency Hz (1000-16000)
float gain = 2.0f            // FFT sensitivity (0.1-10.0)
float curve = 1.5f           // FFT response curve (0.1-3.0)
float baseBright = 0.3f      // Minimum band brightness without audio (0.0-1.0)

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}

// Blend
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**PlaidEffect** (in `src/effects/plaid.h`):

```
Shader shader
ColorLUT* gradientLUT
float time               // morphSpeed accumulator
int resolutionLoc
int fftTextureLoc
int sampleRateLoc
int scaleLoc
int bandCountLoc
int accentWidthLoc
int threadDetailLoc
int twillRepeatLoc
int morphAmountLoc
int timeLoc
int glowIntensityLoc
int baseFreqLoc
int maxFreqLoc
int gainLoc
int curveLoc
int baseBrightLoc
int gradientLUTLoc
```

### Algorithm

The shader is the core deliverable. All GLSL below is the single source of truth.

**Constants and uniforms:**

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float scale;
uniform int bandCount;       // N unique bands (3-8)
uniform float accentWidth;
uniform float threadDetail;
uniform int twillRepeat;
uniform float morphAmount;
uniform float time;          // accumulated morphSpeed * deltaTime
uniform float glowIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define MAX_BANDS 16   // bandCount*2 after mirroring
#define BAND_SAMPLES 4
```

**Band width computation** — every Nth band is thin (accent), rest are wide (ground). Widths oscillate with time. Mirror the N bands into 2N for symmetric sett:

```glsl
void computeBandWidths(out float widths[MAX_BANDS], out int totalBands) {
    totalBands = bandCount * 2;
    for (int i = 0; i < bandCount; i++) {
        // Every 3rd band is thin accent, rest are wide ground
        bool isAccent = (mod(float(i), 3.0) < 0.5);
        float baseWidth = isAccent ? accentWidth : 1.0;
        // Morph: oscillate width over time with per-band phase offset
        float phase = float(i) * 1.618;  // golden ratio spread
        widths[i] = baseWidth + sin(time + phase) * morphAmount * baseWidth;
        widths[i] = max(widths[i], 0.01);  // clamp positive
    }
    // Mirror: bands go 0..N-1, N-1..0
    for (int i = 0; i < bandCount; i++) {
        widths[bandCount + i] = widths[bandCount - 1 - i];
    }
}
```

**Band color lookup** — cursor walk through cumulative widths, return band index and LUT color:

```glsl
int getBandIndex(float pos, float widths[MAX_BANDS], int count) {
    float total = 0.0;
    for (int i = 0; i < count; i++) total += widths[i];
    float localPos = mod(pos, total);
    float cursor = 0.0;
    for (int i = 0; i < count; i++) {
        if (localPos < cursor + widths[i]) return i;
        cursor += widths[i];
    }
    return 0;
}
```

**FFT per-band brightness** — each band maps to a log-space frequency slice:

```glsl
float bandBrightness(int bandIdx, int totalBands) {
    float t0 = float(bandIdx) / float(totalBands);
    float t1 = float(bandIdx + 1) / float(totalBands);
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
    return baseBright + energy;
}
```

**Main — assemble plaid:**

```glsl
void main() {
    // Center UV, aspect-correct, apply scale
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;
    uv *= scale;

    float widths[MAX_BANDS];
    int totalBands;
    computeBandWidths(widths, totalBands);

    // Cumulative total for mod wrapping
    float totalWidth = 0.0;
    for (int i = 0; i < totalBands; i++) totalWidth += widths[i];

    // Warp (vertical bands) and weft (horizontal bands)
    float warpPos = mod(uv.x * totalWidth, totalWidth);
    float weftPos = mod(uv.y * totalWidth, totalWidth);

    int warpIdx = getBandIndex(warpPos, widths, totalBands);
    int weftIdx = getBandIndex(weftPos, widths, totalBands);

    // Map post-mirror index back to pre-mirror for symmetric LUT colors
    // Bands go 0..N-1, N-1..0 so mirrored half maps back: idx >= N → (2N-1-idx)
    int warpOrig = warpIdx < bandCount ? warpIdx : (totalBands - 1 - warpIdx);
    int weftOrig = weftIdx < bandCount ? weftIdx : (totalBands - 1 - weftIdx);
    vec3 warpColor = texture(gradientLUT, vec2((float(warpOrig) + 0.5) / float(bandCount), 0.5)).rgb;
    vec3 weftColor = texture(gradientLUT, vec2((float(weftOrig) + 0.5) / float(bandCount), 0.5)).rgb;

    // Twill: floor-based thread interleave (hard, no AA)
    vec2 threadUV = uv * threadDetail;
    float ix = mod(floor(threadUV.x), float(twillRepeat));
    float iy = mod(floor(threadUV.y), float(twillRepeat));
    bool warpOnTop = mod(ix + iy, float(twillRepeat)) < float(twillRepeat) * 0.5;

    // Blend: top thread 75%, bottom 25%
    vec3 topColor = warpOnTop ? warpColor : weftColor;
    vec3 botColor = warpOnTop ? weftColor : warpColor;
    vec3 color = mix(botColor, topColor, 0.75);

    // FFT brightness from the top band
    int topIdx = warpOnTop ? warpIdx : weftIdx;
    float bright = bandBrightness(topIdx, totalBands);

    color *= bright * glowIntensity;
    finalColor = vec4(color, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| scale | float | 0.5-8.0 | 2.0 | yes | Scale |
| bandCount | int | 3-8 | 5 | no | Band Count |
| accentWidth | float | 0.05-0.5 | 0.15 | yes | Accent Width |
| threadDetail | float | 16.0-512.0 | 128.0 | yes | Thread Detail |
| twillRepeat | int | 2-8 | 4 | no | Twill Repeat |
| morphSpeed | float | 0.0-2.0 | 0.3 | yes | Morph Speed |
| morphAmount | float | 0.0-1.0 | 0.3 | yes | Morph Amount |
| glowIntensity | float | 0.0-2.0 | 1.0 | yes | Glow Intensity |
| baseFreq | float | 27.5-440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.3 | yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |
| blendMode | enum | — | SCREEN | no | Blend Mode |

### Constants

- Enum: `TRANSFORM_PLAID_BLEND`
- Display name: `"Plaid"`
- Category badge: `"GEN"` (section 10)
- Flags: `EFFECT_FLAG_BLEND` (auto via `REGISTER_GENERATOR`)
- Config field: `plaid`
- Effect field: `plaid`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create PlaidConfig and PlaidEffect

**Files**: `src/effects/plaid.h` (create)
**Creates**: Config struct and Effect struct that all other tasks depend on

**Do**: Create the header following the exact struct layouts from the Design section above. Follow `motherboard.h` as the pattern — includes `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Declare `PlaidConfig` with defaults from the Parameters table, `PLAID_CONFIG_FIELDS` macro listing all fields, forward-declare `ColorLUT`, define `PlaidEffect` typedef struct with all `*Loc` ints, and declare the 5 public functions: `PlaidEffectInit`, `PlaidEffectSetup`, `PlaidEffectUninit`, `PlaidConfigDefault`, `PlaidRegisterParams`. Init takes `(PlaidEffect*, const PlaidConfig*)`. Setup takes `(PlaidEffect*, const PlaidConfig*, float deltaTime, Texture2D fftTexture)`.

**Verify**: `cmake.exe --build build` compiles (header-only, just needs to parse).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Plaid shader

**Files**: `shaders/plaid.fs` (create)

**Do**: Implement the Algorithm section from this plan exactly. All GLSL is specified in the Design section — transcribe it into a complete `.fs` file.

**Verify**: File exists and is syntactically valid GLSL (will be verified at runtime).

---

#### Task 2.2: Plaid effect module

**Files**: `src/effects/plaid.cpp` (create)

**Do**: Implement `PlaidEffectInit`, `PlaidEffectSetup`, `PlaidEffectUninit`, `PlaidConfigDefault`, `PlaidRegisterParams`, and the `SetupPlaid`/`SetupPlaidBlend` bridge functions. Follow `motherboard.cpp` exactly as pattern:
- Init: `LoadShader(NULL, "shaders/plaid.fs")`, cache all uniform locations, `ColorLUTInit(&cfg->gradient)`, init `time = 0.0f`
- Setup: accumulate `time += cfg->morphSpeed * deltaTime`, `ColorLUTUpdate`, bind all uniforms. Note: `morphSpeed` is NOT a shader uniform — it only controls the CPU-side time accumulator rate. The shader receives `time` (the accumulated value).
- Uninit: `UnloadShader` + `ColorLUTUninit`
- RegisterParams: register all modulatable float params from the Parameters table
- Bottom: `REGISTER_GENERATOR(TRANSFORM_PLAID_BLEND, Plaid, plaid, "Plaid Blend", SetupPlaidBlend, SetupPlaid)`

Includes: `"plaid.h"`, `"audio/audio.h"`, `"automation/modulation_engine.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `<stddef.h>`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)

**Do**: Four mechanical edits following the add-effect checklist:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/plaid.h"` with other effect includes
   - Add `TRANSFORM_PLAID_BLEND,` before `TRANSFORM_EFFECT_COUNT` in the enum
   - Add `PlaidConfig plaid;` to `EffectConfig` struct (after `dataTraffic` or nearby texture generators)

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/plaid.h"` with other effect includes
   - Add `PlaidEffect plaid;` to `PostEffect` struct

3. **`CMakeLists.txt`**:
   - Add `src/effects/plaid.cpp` to `EFFECTS_SOURCES`

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/plaid.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PlaidConfig, PLAID_CONFIG_FIELDS)` with other macros
   - Add `X(plaid) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: UI panel

**Files**: `src/ui/imgui_effects_gen_texture.cpp` (modify)

**Do**: Add Plaid UI section following `DrawGeneratorsMotherboard` as the pattern:

1. Add `#include "effects/plaid.h"` to includes
2. Add `static bool sectionPlaid = false;` with other section bools
3. Add `DrawGeneratorsPlaid()` helper with sections:
   - **Audio**: Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Base Bright — standard FFT audio section pattern
   - **Fabric**: Scale, Band Count (`SliderInt`), Accent Width, Thread Detail, Twill Repeat (`SliderInt`)
   - **Animation**: Morph Speed, Morph Amount
   - **Glow**: Glow Intensity
   - **Color**: `ImGuiDrawColorMode(&cfg->gradient)`
   - **Output**: Blend Intensity, Blend Mode combo
4. Add `ImGui::Spacing(); DrawGeneratorsPlaid(e, modSources, categoryGlow);` call in `DrawGeneratorsTexture()` after the `DrawGeneratorsDataTraffic` call

Use `ModulatableSlider` for float params, `ImGui::SliderInt` for bandCount and twillRepeat. ImGui ID suffix: `##plaid`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline with "GEN" badge
- [ ] Enabling "Plaid" adds it to the generator blend chain
- [ ] Band count slider changes visible stripe count
- [ ] Twill texture visible at moderate threadDetail
- [ ] FFT brightness reacts to audio input
- [ ] Morph animation causes bands to breathe over time
- [ ] Gradient LUT colors the bands
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
