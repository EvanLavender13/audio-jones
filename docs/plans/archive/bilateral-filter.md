# Bilateral Filter

Edge-preserving smoothing that averages nearby pixels weighted by both spatial distance and color similarity, producing a porcelain/CGI look where flat regions blur smooth while hard edges stay crisp. Same pipeline slot as Kuwahara (painterly category).

**Research**: `docs/research/bilateral-filter.md`

## Design

### Types

**BilateralConfig** (`src/effects/bilateral.h`):
```
bool enabled = false;
float spatialSigma = 4.0f;  // Blur radius in pixels (1.0-10.0)
float rangeSigma = 0.1f;    // Edge sensitivity (0.01-0.5)
```

**BilateralEffect** (`src/effects/bilateral.h`):
```
Shader shader;
int resolutionLoc;
int spatialSigmaLoc;
int rangeSigmaLoc;
```

### Algorithm

Adapted from tranvansang/bilateral-filter reference. Dual-Gaussian weighting: spatial weight decays with pixel distance, range weight decays with luminance difference. Kernel radius is `spatialSigma * 2`, capped at 12.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float spatialSigma;
uniform float rangeSigma;

#define EPS 1e-5

float lum(in vec4 color) {
    return length(color.xyz);
}

void main() {
    float sigS = max(spatialSigma, EPS);
    float sigL = max(rangeSigma, EPS);

    float facS = -1.0 / (2.0 * sigS * sigS);
    float facL = -1.0 / (2.0 * sigL * sigL);

    float sumW = 0.0;
    vec4  sumC = vec4(0.0);
    int halfSize = min(int(sigS * 2.0), 12);

    float l = lum(texture(texture0, fragTexCoord));

    for (int i = -halfSize; i <= halfSize; i++) {
        for (int j = -halfSize; j <= halfSize; j++) {
            vec2 pos = vec2(float(i), float(j));
            vec4 offsetColor = texture(texture0, fragTexCoord + pos / resolution);

            float distS = length(pos);
            float distL = lum(offsetColor) - l;

            float wS = exp(facS * distS * distS);
            float wL = exp(facL * distL * distL);
            float w = wS * wL;

            sumW += w;
            sumC += offsetColor * w;
        }
    }

    finalColor = sumC / sumW;
}
```

Substitutions applied from research:
- `#version 450` -> `#version 330`
- `v_texcoord` -> `fragTexCoord`
- `texture2D()` -> `texture()`
- `textureSize(texture, 0)` -> `resolution` uniform
- `sigmaS`/`sigmaL` -> `spatialSigma`/`rangeSigma`
- `FragColor` -> `finalColor`
- `texture` sampler -> `texture0`
- `float halfSize` -> `int halfSize = min(int(sigS * 2.0), 12)` (dynamic int loop + cap)

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| spatialSigma | float | 1.0-10.0 | 4.0 | Yes | "Spatial Sigma" |
| rangeSigma | float | 0.01-0.5 | 0.1 | Yes | "Range Sigma" |

UI: 2 params, flat layout (no `SeparatorText` sections needed). `spatialSigma` uses `ModulatableSlider` with `"%.1f"`. `rangeSigma` uses `ModulatableSliderLog` with `"%.3f"` (small float range).

### Constants

- Enum: `TRANSFORM_BILATERAL`
- Field name: `bilateral`
- Display name: `"Bilateral"`
- Badge: `"ART"`
- Section index: `4` (Painterly)
- Flags: `EFFECT_FLAG_NONE`
- Config fields macro: `BILATERAL_CONFIG_FIELDS`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create bilateral effect header

**Files**: `src/effects/bilateral.h` (create)
**Creates**: `BilateralConfig`, `BilateralEffect` types, lifecycle declarations

**Do**: Follow `kuwahara.h` structure exactly. Define `BilateralConfig` with fields from the Design section. Define `BilateralEffect` with shader + 3 loc fields. Define `BILATERAL_CONFIG_FIELDS` macro. Declare `Init`, `Setup`, `Uninit`, `RegisterParams`. Setup signature matches Kuwahara: `(const BilateralEffect*, const BilateralConfig*)` - no deltaTime needed.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (all parallel)

#### Task 2.1: Create bilateral effect source

**Files**: `src/effects/bilateral.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `kuwahara.cpp` as template. Same include set. `Init` loads `shaders/bilateral.fs`, caches `resolution`, `spatialSigma`, `rangeSigma` locs. `Setup` binds resolution as vec2 and both sigma floats. `Uninit` unloads shader. `RegisterParams` registers `bilateral.spatialSigma` (1.0-10.0) and `bilateral.rangeSigma` (0.01-0.5). Bridge function `SetupBilateral(PostEffect*)`. Colocated UI: `DrawBilateralParams` flat layout with `ModulatableSlider` for spatialSigma (`"%.1f"`) and `ModulatableSliderLog` for rangeSigma (`"%.3f"`). Registration macro: `REGISTER_EFFECT(TRANSFORM_BILATERAL, Bilateral, bilateral, "Bilateral", "ART", 4, EFFECT_FLAG_NONE, SetupBilateral, NULL, DrawBilateralParams)`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Create bilateral shader

**Files**: `shaders/bilateral.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section verbatim. No attribution block needed (tranvansang is a reference, not a licensed Shadertoy).

**Verify**: File exists at `shaders/bilateral.fs`.

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: (1) Add `#include "effects/bilateral.h"` with other effect includes. (2) Add `TRANSFORM_BILATERAL` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT` - place near Kuwahara in the Painterly group. (3) Add `TRANSFORM_BILATERAL` to `TransformOrderConfig::order` array. (4) Add `BilateralConfig bilateral;` to `EffectConfig` struct near the Kuwahara member.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**: (1) Add `#include "effects/bilateral.h"` with other effect includes. (2) Add `BilateralEffect bilateral;` member to `PostEffect` struct near the Kuwahara member.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Add to CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/bilateral.cpp` to `EFFECTS_SOURCES` near `kuwahara.cpp`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Register serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: (1) Add `#include "effects/bilateral.h"`. (2) Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BilateralConfig, BILATERAL_CONFIG_FIELDS)`. (3) Add `X(bilateral) \` to `EFFECT_CONFIG_FIELDS` X-macro table near the kuwahara entry.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Effects window under Painterly category
- [ ] Effect shows "ART" badge
- [ ] Enable checkbox toggles the effect
- [ ] Both sliders respond to input
- [ ] Modulation routes to both parameters
- [ ] Preset save/load round-trips settings
- [ ] Effect can be reordered in transform pipeline
