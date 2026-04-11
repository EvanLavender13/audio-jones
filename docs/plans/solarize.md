# Solarize

Psychedelic tone inversion where colors flip past a threshold. Uses the Suricrasia Online V-type `solar_invert` function, which interpolates smoothly between identity (0.0), full solarize (0.5), and full inversion (1.0) - ideal for modulation. A threshold parameter shifts which tones get inverted.

**Research**: `docs/research/solarize.md`

## Design

### Types

**SolarizeConfig** (`src/effects/solarize.h`):

```cpp
struct SolarizeConfig {
  bool enabled = false;
  float amount = 0.5f;     // Inversion strength (0.0-1.0)
  float threshold = 0.5f;  // Tone inversion peak position (0.0-1.0)
};
```

**SolarizeEffect** (`src/effects/solarize.h`):

```cpp
typedef struct SolarizeEffect {
  Shader shader;
  int amountLoc;
  int thresholdLoc;
} SolarizeEffect;
```

No time accumulator - pure per-pixel math with no animation state.

### Algorithm

Core function (Suricrasia V-type, verbatim):

```glsl
vec3 solar_invert(vec3 color, float x) {
    float st = 1.0 - step(0.5, x);
    return abs((color - st) * (2.0 * x + 4.0 * st - 3.0) + 1.0);
}
```

Main body:

```glsl
void main() {
    vec2 uv = fragTexCoord;
    vec4 color = texture(texture0, uv);

    vec3 shifted = clamp(color.rgb + (threshold - 0.5) * 2.0, 0.0, 1.0);
    vec3 result = solar_invert(shifted, amount);

    finalColor = vec4(result, color.a);
}
```

Uniforms: `amount` (float), `threshold` (float).

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| amount | float | 0.0-1.0 | 0.5 | Yes | "Amount" |
| threshold | float | 0.0-1.0 | 0.5 | Yes | "Threshold" |

### Constants

- Enum: `TRANSFORM_SOLARIZE`
- Display name: `"Solarize"`
- Badge: `"COL"`
- Section: `8`
- Flags: `EFFECT_FLAG_NONE`
- Param prefix: `"solarize."`
- Field name: `solarize`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/solarize.h` (CREATE)
**Creates**: `SolarizeConfig`, `SolarizeEffect`, function declarations

**Do**: Create header with config struct, effect struct, and 4 function declarations (Init, Setup, Uninit, RegisterParams). Follow `false_color.h` structure but without ColorConfig/LUT. Init takes only `SolarizeEffect*` (no config needed). Use types and defaults from Design section.

**Verify**: Header compiles when included.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create effect implementation

**Files**: `src/effects/solarize.cpp` (CREATE)
**Depends on**: Wave 1 complete

**Do**: Implement Init (load shader, cache 2 uniform locs), Setup (bind amount and threshold), Uninit (unload shader), RegisterParams (register both params). Add `// === UI ===` section with `DrawSolarizeParams` (2 `ModulatableSlider` calls, format `"%.2f"`). Add `SetupSolarize` bridge function and `REGISTER_EFFECT` macro. Follow `false_color.cpp` structure but simpler (no LUT, no ColorConfig). Uses `REGISTER_EFFECT` not `REGISTER_EFFECT_CFG` since Init takes only `SolarizeEffect*`.

**Verify**: Compiles.

---

#### Task 2.2: Create shader

**Files**: `shaders/solarize.fs` (CREATE)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section. Uniforms: `sampler2D texture0`, `float amount`, `float threshold`. Include `solar_invert` function verbatim, then the main body with threshold shift and clamp. Attribution comment: `// Suricrasia Online - Interpolatable Colour Inversion` with link.

**Verify**: Shader source has correct `#version 330`, in/out/uniform declarations.

---

#### Task 2.3: Register in effect config

**Files**: `src/config/effect_config.h` (MODIFY)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/solarize.h"` with the other effect includes. Add `TRANSFORM_SOLARIZE` enum entry before `TRANSFORM_EFFECT_COUNT` (near other Color effects like `TRANSFORM_FALSE_COLOR`). Add `SolarizeConfig solarize;` member in EffectConfig (near other Color config members).

**Verify**: Compiles.

---

#### Task 2.4: Register in PostEffect

**Files**: `src/render/post_effect.h` (MODIFY)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/solarize.h"`. Add `SolarizeEffect solarize;` member in PostEffect struct (near other Color effect members like `falseColor`).

**Verify**: Compiles.

---

#### Task 2.5: Add to build system

**Files**: `CMakeLists.txt` (MODIFY)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/solarize.cpp` to `EFFECTS_SOURCES` list (alphabetical placement near other Color effects).

**Verify**: CMake configure succeeds.

---

#### Task 2.6: Add preset serialization

**Files**: `src/config/effect_serialization.cpp` (MODIFY)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/solarize.h"`. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SolarizeConfig, SOLARIZE_CONFIG_FIELDS)` with other per-config macros. Add `X(solarize) \` to `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Color section of transform pipeline
- [ ] Enable toggle works, effect renders with visible solarization
- [ ] Amount slider: 0.0 = identity, 0.5 = full solarize, 1.0 = full inversion
- [ ] Threshold slider shifts which tones get inverted
- [ ] Both params respond to modulation routing
- [ ] Preset save/load preserves settings
