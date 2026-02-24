# Risograph

Bold flat ink layers with grainy, speckled coverage printed slightly out of register onto warm paper. Three ink colors (hot pink, teal, golden yellow) decompose the image into stencil-like channels that overlap to create subtractive mix colors. Single-pass transform effect in the Graphic category.

**Research**: `docs/research/risograph.md`

## Design

### Types

```cpp
// risograph.h
struct RisographConfig {
  bool enabled = false;
  float grainScale = 200.0f;    // Noise sample scale (50-800)
  float grainIntensity = 0.4f;  // Ink erosion amount (0.0-1.0)
  float grainSpeed = 0.3f;      // Grain animation rate (0.0-2.0)
  float misregAmount = 0.005f;  // UV offset per ink layer (0.0-0.02)
  float misregSpeed = 0.2f;     // Misregistration drift rate (0.0-2.0)
  float inkDensity = 1.0f;      // Overall ink coverage (0.2-1.5)
  int posterize = 0;            // Tonal steps (0=off, 2-16)
  float paperTone = 0.3f;       // Paper warmth (0.0-1.0)
};

typedef struct RisographEffect {
  Shader shader;
  int resolutionLoc;
  int grainScaleLoc;
  int grainIntensityLoc;
  int grainTimeLoc;
  int misregAmountLoc;
  int misregTimeLoc;
  int inkDensityLoc;
  int posterizeLoc;
  int paperToneLoc;
  float grainTime;   // grainSpeed accumulator
  float misregTime;  // misregSpeed accumulator
} RisographEffect;
```

### Algorithm

Single-pass fragment shader. All GLSL below is the source of truth for the shader task.

#### Simplex noise (Stefan Gustavson, public domain — verbatim)

```glsl
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289((( x * 34.0) + 1.0) * x); }

float snoise(vec2 v) {
  const vec4 C = vec4(0.211324865405187, 0.366025403784439,
    -0.577350269189626, 0.024390243902439);
  vec2 i = floor(v + dot(v, C.yy));
  vec2 x0 = v - i + dot(i, C.xx);
  vec2 i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;
  i = mod289(i);
  vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0));
  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m; m = m*m;
  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 a0 = x - floor(x + 0.5);
  m *= 1.792843 - 0.853735 * (a0*a0 + h*h);
  vec3 g;
  g.x = a0.x * x0.x + h.x * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}
```

#### Multi-octave grain noise (Gustavson pattern)

```glsl
float grainNoise(vec2 st, float timeOffset) {
  vec2 p = st + timeOffset;
  float n = 0.1 * snoise(p * 200.0);
  n += 0.05 * snoise(p * 400.0);
  n += 0.025 * snoise(p * 800.0);
  return n;
}
```

#### Main shader logic

```glsl
void main() {
    vec2 uv = fragTexCoord;
    vec2 st = fragTexCoord * resolution / grainScale;

    // --- Optional posterization ---
    // Applied per-layer after sampling, before decomposition

    // --- Misregistration: each layer samples at a different UV offset ---
    vec2 offC = misregAmount * vec2(sin(misregTime * 0.7), cos(misregTime * 0.9));
    vec2 offM = misregAmount * vec2(sin(misregTime * 1.1 + 2.0), cos(misregTime * 0.6 + 1.0));
    vec2 offY = misregAmount * vec2(sin(misregTime * 0.8 + 4.0), cos(misregTime * 1.3 + 3.0));

    vec3 rgbC = texture(texture0, uv + offC).rgb;
    vec3 rgbM = texture(texture0, uv + offM).rgb;
    vec3 rgbY = texture(texture0, uv + offY).rgb;

    // Posterize each layer independently (if enabled)
    if (posterize > 0) {
        float levels = float(posterize);
        rgbC = floor(rgbC * levels + 0.5) / levels;
        rgbM = floor(rgbM * levels + 0.5) / levels;
        rgbY = floor(rgbY * levels + 0.5) / levels;
    }

    // --- CMY decomposition per layer ---
    // Each layer extracts only its own channel
    vec3 cmyC = 1.0 - rgbC;
    float kC = min(cmyC.r, min(cmyC.g, cmyC.b));
    float cAmount = cmyC.r - kC;  // Cyan channel from C-offset sample

    vec3 cmyM = 1.0 - rgbM;
    float kM = min(cmyM.r, min(cmyM.g, cmyM.b));
    float mAmount = cmyM.g - kM;  // Magenta channel from M-offset sample

    vec3 cmyY = 1.0 - rgbY;
    float kY = min(cmyY.r, min(cmyY.g, cmyY.b));
    float yAmount = cmyY.b - kY;  // Yellow channel from Y-offset sample

    // Average K from all three samples
    float k = (kC + kM + kY) / 3.0;

    // --- Grain: per-layer noise erodes ink coverage ---
    float nC = grainNoise(st, grainTime);
    float nM = grainNoise(st, grainTime + 17.3);
    float nY = grainNoise(st, grainTime + 31.7);

    float grainC = smoothstep(1.0 - grainIntensity, 1.0, nC * 0.5 + 0.5);
    float grainM = smoothstep(1.0 - grainIntensity, 1.0, nM * 0.5 + 0.5);
    float grainY = smoothstep(1.0 - grainIntensity, 1.0, nY * 0.5 + 0.5);

    cAmount *= (1.0 - grainC);
    mAmount *= (1.0 - grainM);
    yAmount *= (1.0 - grainY);

    // --- Subtractive compositing ---
    vec3 tealInk = vec3(0.0, 0.72, 0.74);
    vec3 pinkInk = vec3(0.91, 0.16, 0.54);
    vec3 yellowInk = vec3(0.98, 0.82, 0.05);

    vec3 paper = mix(vec3(1.0), vec3(0.96, 0.93, 0.88), paperTone);

    // Paper texture: fine-scale noise modulates paper color
    float paperNoise = snoise(st * 3.0 + grainTime * 0.1) * 0.02;
    paper += paperNoise;

    vec3 result = paper;
    result *= 1.0 - cAmount * inkDensity * (1.0 - tealInk);
    result *= 1.0 - mAmount * inkDensity * (1.0 - pinkInk);
    result *= 1.0 - yAmount * inkDensity * (1.0 - yellowInk);
    result *= 1.0 - k * inkDensity * 0.7;

    finalColor = vec4(result, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| grainScale | float | 50-800 | 200.0 | Yes | Grain Scale |
| grainIntensity | float | 0.0-1.0 | 0.4 | Yes | Grain |
| grainSpeed | float | 0.0-2.0 | 0.3 | Yes | Grain Speed |
| misregAmount | float | 0.0-0.02 | 0.005 | Yes | Misregister |
| misregSpeed | float | 0.0-2.0 | 0.2 | Yes | Misreg Speed |
| inkDensity | float | 0.2-1.5 | 1.0 | Yes | Ink Density |
| posterize | int | 0-16 | 0 | No | Posterize |
| paperTone | float | 0.0-1.0 | 0.3 | Yes | Paper Tone |

### Constants

- Enum: `TRANSFORM_RISOGRAPH`
- Display name: `"Risograph"`
- Category badge: `"GFX"` (Graphic)
- Section index: `5`
- Flags: `EFFECT_FLAG_NONE`
- Macro: `REGISTER_EFFECT`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create risograph.h

**Files**: `src/effects/risograph.h` (create)
**Creates**: `RisographConfig`, `RisographEffect`, function declarations, `RISOGRAPH_CONFIG_FIELDS` macro

**Do**: Follow `halftone.h` structure exactly. Define `RisographConfig` and `RisographEffect` structs as specified in the Design section. Declare `Init`, `Setup`, `Uninit`, `ConfigDefault`, `RegisterParams`. The `CONFIG_FIELDS` macro lists all serializable fields: `enabled, grainScale, grainIntensity, grainSpeed, misregAmount, misregSpeed, inkDensity, posterize, paperTone`.

**Verify**: Header compiles standalone.

---

### Wave 2: Parallel implementation (no file overlaps)

#### Task 2.1: Create risograph.cpp

**Files**: `src/effects/risograph.cpp` (create)
**Depends on**: Wave 1

**Do**: Follow `halftone.cpp` as pattern. Implement:
- `Init`: Load `shaders/risograph.fs`, cache all uniform locations, init both time accumulators to 0
- `Setup`: Accumulate `grainTime += cfg->grainSpeed * deltaTime` and `misregTime += cfg->misregSpeed * deltaTime`. Bind resolution, all config params, and both accumulated times as uniforms. `posterize` is sent as int via `SHADER_UNIFORM_INT`.
- `Uninit`: Unload shader
- `ConfigDefault`: Return `RisographConfig{}`
- `RegisterParams`: Register 7 float params (all except `posterize`): `risograph.grainScale` (50-800), `risograph.grainIntensity` (0-1), `risograph.grainSpeed` (0-2), `risograph.misregAmount` (0-0.02), `risograph.misregSpeed` (0-2), `risograph.inkDensity` (0.2-1.5), `risograph.paperTone` (0-1)
- Bridge function `SetupRisograph(PostEffect*)` — NOT static
- `// === UI ===` section with `static void DrawRisographParams(EffectConfig*, const ModSources*, ImU32)`:
  - `ModulatableSlider("Grain Scale##risograph", ..., "risograph.grainScale", "%.0f", ms)`
  - `ModulatableSlider("Grain##risograph", ..., "risograph.grainIntensity", "%.2f", ms)`
  - `ModulatableSlider("Grain Speed##risograph", ..., "risograph.grainSpeed", "%.2f", ms)`
  - `ModulatableSlider("Misregister##risograph", ..., "risograph.misregAmount", "%.4f", ms)`
  - `ModulatableSlider("Misreg Speed##risograph", ..., "risograph.misregSpeed", "%.2f", ms)`
  - `ModulatableSlider("Ink Density##risograph", ..., "risograph.inkDensity", "%.2f", ms)`
  - `ImGui::SliderInt("Posterize##risograph", &cfg->posterize, 0, 16)`
  - `ModulatableSlider("Paper Tone##risograph", ..., "risograph.paperTone", "%.2f", ms)`
- `REGISTER_EFFECT(TRANSFORM_RISOGRAPH, Risograph, risograph, "Risograph", "GFX", 5, EFFECT_FLAG_NONE, SetupRisograph, NULL, DrawRisographParams)` wrapped in clang-format off/on

**Verify**: Compiles.

#### Task 2.2: Create risograph.fs shader

**Files**: `shaders/risograph.fs` (create)
**Depends on**: Wave 1 (needs to know uniform names)

**Do**: Implement the Algorithm section from the Design verbatim. Uniforms: `texture0` (sampler2D), `resolution` (vec2), `grainScale` (float), `grainIntensity` (float), `grainTime` (float), `misregAmount` (float), `misregTime` (float), `inkDensity` (float), `posterize` (int), `paperTone` (float). Include the simplex noise function and multi-octave grain function exactly as written in the Algorithm section.

**Verify**: Valid GLSL 330. No build needed (runtime compiled).

#### Task 2.3: Config registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/risograph.h"` in the include list (alphabetical, after `#include "effects/radial_streak.h"`)
2. Add `TRANSFORM_RISOGRAPH,` in the enum before `TRANSFORM_EFFECT_COUNT`
3. Add `RisographConfig risograph;` to `EffectConfig` struct with comment `// Risograph (CMY ink separation with grain and misregistration)`

**Verify**: Compiles.

#### Task 2.4: PostEffect struct member

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/risograph.h"` (alphabetical, after `#include "effects/radial_streak.h"`)
2. Add `RisographEffect risograph;` member in the PostEffect struct (after the existing Graphic effects like `HalftoneEffect halftone`)

**Verify**: Compiles.

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/risograph.cpp` to `EFFECTS_SOURCES` list (alphabetical placement near other r-files).

**Verify**: `cmake.exe -G Ninja -B build -S .` succeeds.

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/risograph.h"` (alphabetical)
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RisographConfig, RISOGRAPH_CONFIG_FIELDS)` in the O-Z section
3. Add `X(risograph) \` to `EFFECT_CONFIG_FIELDS(X)` macro

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build`
- [ ] Effect appears in Graphic section with "GFX" badge
- [ ] Effect can be enabled, reordered in transform pipeline
- [ ] All 7 modulatable sliders respond to modulation
- [ ] Posterize slider goes 0-16 as integer
- [ ] Preset save/load preserves all settings
