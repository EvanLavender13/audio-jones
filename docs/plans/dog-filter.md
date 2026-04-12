# DoG Filter

Single-pass Difference of Gaussians edge detection that subtracts two Gaussian blurs at different scales to isolate edges, then applies soft thresholding to produce ink-like contour lines over the image. Based on the XDoG isotropic shader by Jan Eric Kyprianidis.

**Research**: `docs/research/dog-filter.md`

## Design

### Types

**DogFilterConfig** (`src/effects/dog_filter.h`):

```
struct DogFilterConfig {
  bool enabled = false;
  float sigma = 1.5f;  // Edge Gaussian sigma (0.5-5.0)
  float tau = 0.99f;    // Gaussian weighting (0.9-1.0)
  float phi = 2.0f;     // Threshold steepness (0.5-10.0)
};
```

**DogFilterEffect** (`src/effects/dog_filter.h`):

```
typedef struct DogFilterEffect {
  Shader shader;
  int resolutionLoc;
  int sigmaLoc;
  int tauLoc;
  int phiLoc;
} DogFilterEffect;
```

No time accumulator - this is a static spatial filter with no animation state.

### Algorithm

Single-pass isotropic DoG adapted from `dog_fs.glsl`. The shader computes two Gaussian blurs simultaneously within a single kernel loop, subtracts them, and applies soft thresholding.

```glsl
// Based on XDoG isotropic shader by Jan Eric Kyprianidis
// https://github.com/jkyprian/flowabs
// License: GPL-3.0
// Modified: GLSL 330, fragTexCoord, resolution uniform, luminance input,
//   derived sigma_r, halfWidth cap, color compositing

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float sigma;
uniform float tau;
uniform float phi;

void main() {
    float sigmaR = sigma * 1.6;
    float twoSigmaESquared = 2.0 * sigma * sigma;
    float twoSigmaRSquared = 2.0 * sigmaR * sigmaR;
    int halfWidth = int(ceil(2.0 * sigmaR));
    halfWidth = min(halfWidth, 12);

    vec2 sum = vec2(0.0);
    vec2 norm = vec2(0.0);

    for (int i = -halfWidth; i <= halfWidth; i++) {
        for (int j = -halfWidth; j <= halfWidth; j++) {
            float d = length(vec2(i, j));
            vec2 kernel = vec2(exp(-d * d / twoSigmaESquared),
                               exp(-d * d / twoSigmaRSquared));

            vec4 texSample = texture(texture0,
                fragTexCoord + vec2(i, j) / resolution);
            float L = dot(texSample.rgb, vec3(0.299, 0.587, 0.114));

            norm += 2.0 * kernel;
            sum += kernel * vec2(L, L);
        }
    }
    sum /= norm;

    float H = 100.0 * (sum.x - tau * sum.y);
    float edge = (H > 0.0) ? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H);

    vec4 inputColor = texture(texture0, fragTexCoord);
    finalColor = vec4(inputColor.rgb * edge, inputColor.a);
}
```

Line-by-line derivation from reference `dog_fs.glsl`:
- `sigma_e` -> `sigma` (exposed uniform)
- `sigma_r` -> `sigmaR = sigma * 1.6` (derived in shader, k = 1.6 is the standard LoG approximation ratio)
- `int halfWidth = int(ceil(2.0 * sigma_r))` -> kept, plus `min(halfWidth, 12)` cap
- Dual-kernel loop structure: kept verbatim
- `texture2D(img, uv + vec2(i,j) / img_size).xx` -> `texture(texture0, fragTexCoord + vec2(i,j) / resolution)` with `dot(rgb, vec3(0.299, 0.587, 0.114))` for perceptual luminance, then `vec2(L, L)` to match the `.xx` swizzle
- `sum /= norm` -> kept
- `100.0 * (sum.x - tau * sum.y)` -> kept
- `(H > 0.0) ? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H)` -> kept
- `gl_FragColor = vec4(vec3(edge), 1.0)` -> `finalColor = vec4(inputColor.rgb * edge, inputColor.a)` (multiply with input color for colored contour compositing)

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| sigma | float | 0.5-5.0 | 1.5 | yes | `"Sigma##dogFilter"` |
| tau | float | 0.9-1.0 | 0.99 | yes | `"Tau##dogFilter"` |
| phi | float | 0.5-10.0 | 2.0 | yes | `"Phi##dogFilter"` |

Slider formats: sigma `"%.2f"`, tau `"%.3f"` (narrow range needs precision), phi `"%.1f"`.

### Constants

- Enum: `TRANSFORM_DOG_FILTER`
- Field: `dogFilter`
- Display name: `"DoG Filter"`
- Badge: `"PRT"`
- Section: `5`
- Flags: `EFFECT_FLAG_NONE`
- Macro: `REGISTER_EFFECT`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create DogFilterConfig and DogFilterEffect

**Files**: `src/effects/dog_filter.h` (create)
**Creates**: Config struct, Effect struct, function declarations

**Do**: Follow `toon.h` structure. Define `DogFilterConfig` and `DogFilterEffect` per Design section. Declare `DogFilterEffectInit`, `DogFilterEffectSetup`, `DogFilterEffectUninit`, `DogFilterRegisterParams`. Setup signature takes `const DogFilterEffect*` and `const DogFilterConfig*` (no deltaTime - no animation state). Define `DOG_FILTER_CONFIG_FIELDS` macro.

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Implementation, Shader, Integration

#### Task 2.1: Effect implementation

**Files**: `src/effects/dog_filter.cpp` (create)
**Depends on**: Wave 1

**Do**: Follow `toon.cpp` structure. Implement Init (load shader, cache 4 uniform locations: resolution, sigma, tau, phi), Setup (bind resolution from `GetScreenWidth`/`GetScreenHeight`, bind sigma/tau/phi from config), Uninit, RegisterParams (3 params: `dogFilter.sigma` 0.5-5.0, `dogFilter.tau` 0.9-1.0, `dogFilter.phi` 0.5-10.0). Colocated UI: `DrawDogFilterParams` with 3 `ModulatableSlider` calls per Parameters table. Bridge function `SetupDogFilter(PostEffect* pe)`. `REGISTER_EFFECT` macro at bottom with constants from Design section.

**Verify**: Compiles.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/dog_filter.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the Algorithm section verbatim. Attribution block at top before `#version 330`. Uniforms: `texture0`, `resolution`, `sigma`, `tau`, `phi`.

**Verify**: File exists with correct attribution header.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/dog_filter.h"` with other effect includes. Add `TRANSFORM_DOG_FILTER` enum entry before `TRANSFORM_EFFECT_COUNT`. Add `TRANSFORM_DOG_FILTER` to `TransformOrderConfig::order` array. Add `DogFilterConfig dogFilter;` member to `EffectConfig` struct.

**Verify**: Compiles.

---

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/dog_filter.h"`. Add `DogFilterEffect dogFilter;` member to `PostEffect` struct (after existing effect members).

**Verify**: Compiles.

---

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/dog_filter.cpp` to `EFFECTS_SOURCES` in alphabetical order (after `digital_rain.cpp` or nearest alphabetical neighbor).

**Verify**: Compiles.

---

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/dog_filter.h"`. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DogFilterConfig, DOG_FILTER_CONFIG_FIELDS)` with other per-config macros. Add `X(dogFilter) \` to `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Print category section of effects panel
- [ ] Effect shows "PRT" badge
- [ ] Enabling effect draws contour lines over the image
- [ ] Sigma slider changes line thickness
- [ ] Tau slider controls edge sensitivity
- [ ] Phi slider controls edge sharpness
- [ ] All 3 params modulatable via LFO routing
- [ ] Preset save/load preserves settings
- [ ] Effect can be reordered via drag-drop
