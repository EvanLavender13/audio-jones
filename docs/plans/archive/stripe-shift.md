# Stripe Shift

Flat RGB color bars displaced by input brightness — any on-screen content shapes the stripe pattern. Three independent stripe channels at slightly different animation rates create hard-edged color separation through interference. Where channels agree: white or black. Where they disagree: pure R/G/B or mixed cyan/magenta/yellow.

**Research**: `docs/research/stripe-shift.md`

## Design

### Types

**StripeShiftConfig** (`src/effects/stripe_shift.h`):

```cpp
struct StripeShiftConfig {
  bool enabled = false;
  float stripeCount = 16.0f;    // Stripe density (4.0-64.0)
  float stripeWidth = 0.3f;     // Step threshold (0.05-0.5)
  float displacement = 1.0f;    // Brightness offset intensity (0.0-4.0)
  float speed = 1.0f;           // Phase accumulation rate (-PI_F to PI_F)
  float channelSpread = 0.1f;   // Per-channel rate offset (0.0-0.5)
  float colorDisplace = 0.0f;   // 0=luminance, 1=per-channel RGB (0.0-1.0)
  float hardEdge = 0.0f;        // 0=continuous, >0=binary threshold (0.0-1.0)
};
```

**StripeShiftEffect** (`src/effects/stripe_shift.h`):

```cpp
typedef struct StripeShiftEffect {
  Shader shader;
  int resolutionLoc;
  int phaseLoc;
  int stripeCountLoc;
  int stripeWidthLoc;
  int displacementLoc;
  int channelSpreadLoc;
  int colorDisplaceLoc;
  int hardEdgeLoc;
  float phase;
} StripeShiftEffect;
```

### Algorithm

**Shader** (`shaders/stripe_shift.fs`):

```glsl
// Based on "大龙猫 - Quicky#009" by totetmatt
// https://www.shadertoy.com/view/wdt3zn
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced diamond distance field with input brightness displacement,
//           added per-channel color displacement, parameterized all constants

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float phase;
uniform float stripeCount;
uniform float stripeWidth;
uniform float displacement;
uniform float channelSpread;
uniform float colorDisplace;
uniform float hardEdge;

void main() {
    vec4 src = texture(texture0, fragTexCoord);
    float lum = dot(src.rgb, vec3(0.299, 0.587, 0.114));

    // Per-channel displacement: blend between shared luminance and per-channel RGB
    float dispR = mix(lum, src.r, colorDisplace);
    float dispG = mix(lum, src.g, colorDisplace);
    float dispB = mix(lum, src.b, colorDisplace);

    // Hard edge: hardEdge=0 keeps continuous displacement,
    // hardEdge>0 snaps to binary using hardEdge value as brightness threshold
    float he = step(0.001, hardEdge); // 0 when off, 1 when active
    dispR = mix(dispR, step(hardEdge, dispR), he);
    dispG = mix(dispG, step(hardEdge, dispG), he);
    dispB = mix(dispB, step(hardEdge, dispB), he);

    // Displaced Y positions per channel
    float yR = fragTexCoord.y + dispR * displacement;
    float yG = fragTexCoord.y + dispG * displacement;
    float yB = fragTexCoord.y + dispB * displacement;

    // Per-channel stripes with phase separation
    // R: cos, base speed
    // G: sin (90 degree base offset), slightly slower
    // B: cos with +1.0 offset, slightly faster
    float r = 1.0 - step(stripeWidth, abs(cos(-phase + yR * stripeCount)));
    float g = 1.0 - step(stripeWidth, abs(sin(-phase * (1.0 - channelSpread) + yG * stripeCount)));
    float b = 1.0 - step(stripeWidth, abs(cos(1.0 + phase * (1.0 + channelSpread) + yB * stripeCount)));

    finalColor = vec4(r, g, b, 1.0);
}
```

**CPU Setup** (`StripeShiftEffectSetup`):
- Accumulate: `e->phase += cfg->speed * deltaTime`
- Bind uniforms: `resolution`, `phase`, `stripeCount`, `stripeWidth`, `displacement`, `channelSpread`, `colorDisplace`, `hardEdge`

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| stripeCount | float | 4.0–64.0 | 16.0 | Yes | Density |
| stripeWidth | float | 0.05–0.5 | 0.3 | Yes | Width |
| displacement | float | 0.0–4.0 | 1.0 | Yes | Displace |
| speed | float | -PI_F–PI_F | 1.0 | Yes | Speed |
| channelSpread | float | 0.0–0.5 | 0.1 | Yes | Spread |
| colorDisplace | float | 0.0–1.0 | 0.0 | Yes | Color Displace |
| hardEdge | float | 0.0–1.0 | 0.0 | Yes | Hard Edge (0=continuous, >0=binary threshold) |

### Constants

- Enum: `TRANSFORM_STRIPE_SHIFT`
- Display name: `"Stripe Shift"`
- Badge: `"RET"`
- Section: `6` (Retro)
- Flags: `EFFECT_FLAG_NONE`
- Field name: `stripeShift`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create stripe_shift.h

**Files**: `src/effects/stripe_shift.h` (create)
**Creates**: Config struct, Effect struct, function declarations

**Do**: Create header following `src/effects/glitch.h` structure:
- Include guard `STRIPE_SHIFT_EFFECT_H`
- Top comment: `// Stripe Shift: flat RGB bars displaced by input brightness`
- `StripeShiftConfig` struct with all fields from Design section
- `#define STRIPE_SHIFT_CONFIG_FIELDS enabled, stripeCount, stripeWidth, displacement, speed, channelSpread, colorDisplace, hardEdge`
- `StripeShiftEffect` typedef struct from Design section
- 5 function declarations: `StripeShiftEffectInit`, `StripeShiftEffectSetup`, `StripeShiftEffectUninit`, `StripeShiftConfigDefault`, `StripeShiftRegisterParams`
- Includes: `"raylib.h"`, `<stdbool.h>`

**Verify**: File exists with correct include guard.

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Create stripe_shift.cpp

**Files**: `src/effects/stripe_shift.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement following `src/effects/glitch.cpp` pattern:

Includes:
- `"stripe_shift.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_descriptor.h"`, `"render/post_effect.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `<stddef.h>`

Functions:
- `static CacheLocations(StripeShiftEffect* e)` — cache all 9 uniform locations (7 params + resolution + phase)
- `StripeShiftEffectInit(StripeShiftEffect* e)` — load `shaders/stripe_shift.fs`, cache, init `phase = 0.0f`
- `StripeShiftEffectSetup(StripeShiftEffect* e, const StripeShiftConfig* cfg, float deltaTime)` — accumulate `e->phase += cfg->speed * deltaTime`, set resolution from `GetScreenWidth()/GetScreenHeight()`, set all uniforms
- `StripeShiftEffectUninit(StripeShiftEffect* e)` — `UnloadShader(e->shader)`
- `StripeShiftConfigDefault(void)` — `return StripeShiftConfig{};`
- `StripeShiftRegisterParams(StripeShiftConfig* cfg)` — register all 7 params:
  - `"stripeShift.stripeCount"`, 4.0f, 64.0f
  - `"stripeShift.stripeWidth"`, 0.05f, 0.5f
  - `"stripeShift.displacement"`, 0.0f, 4.0f
  - `"stripeShift.speed"`, -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX
  - `"stripeShift.channelSpread"`, 0.0f, 0.5f
  - `"stripeShift.colorDisplace"`, 0.0f, 1.0f
  - `"stripeShift.hardEdge"`, 0.0f, 1.0f

UI section (`// === UI ===`):
- `static void DrawStripeShiftParams(EffectConfig* e, const ModSources* ms, ImU32 glow)`:
  - `(void)glow;`
  - `StripeShiftConfig* cfg = &e->stripeShift;`
  - `ModulatableSlider("Density##stripeShift", &cfg->stripeCount, "stripeShift.stripeCount", "%.1f", ms);`
  - `ModulatableSlider("Width##stripeShift", &cfg->stripeWidth, "stripeShift.stripeWidth", "%.2f", ms);`
  - `ModulatableSlider("Displace##stripeShift", &cfg->displacement, "stripeShift.displacement", "%.2f", ms);`
  - `ModulatableSliderSpeedDeg("Speed##stripeShift", &cfg->speed, "stripeShift.speed", ms);`
  - `ModulatableSlider("Spread##stripeShift", &cfg->channelSpread, "stripeShift.channelSpread", "%.2f", ms);`
  - `ModulatableSlider("Color Displace##stripeShift", &cfg->colorDisplace, "stripeShift.colorDisplace", "%.2f", ms);`
  - `ModulatableSlider("Hard Edge##stripeShift", &cfg->hardEdge, "stripeShift.hardEdge", "%.2f", ms);`

Bridge + registration:
- `void SetupStripeShift(PostEffect* pe)` (non-static!) — delegates to `StripeShiftEffectSetup(&pe->stripeShift, &pe->effects.stripeShift, pe->currentDeltaTime)`
- Registration macro (wrap in `// clang-format off` / `// clang-format on`):
  ```
  REGISTER_EFFECT(TRANSFORM_STRIPE_SHIFT, StripeShift, stripeShift,
                  "Stripe Shift", "RET", 6, EFFECT_FLAG_NONE,
                  SetupStripeShift, NULL, DrawStripeShiftParams)
  ```

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.2: Create stripe_shift.fs

**Files**: `shaders/stripe_shift.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the shader from the Algorithm section verbatim. The attribution comment block MUST appear before `#version 330`. Copy the complete shader code from the Algorithm section — do not modify or "improve" any formulas.

**Verify**: File exists with `#version 330` and attribution header.

---

#### Task 2.3: Integration edits

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (modify all)
**Depends on**: Wave 1 complete

**Do** (4 files, all mechanical integration):

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/stripe_shift.h"` in alphabetical order among effect includes
   - Add `TRANSFORM_STRIPE_SHIFT,` before `TRANSFORM_ACCUM_COMPOSITE` in the enum
   - Add `StripeShiftConfig stripeShift;` to `EffectConfig` struct with comment `// Stripe Shift (flat RGB bars displaced by input brightness)`

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/stripe_shift.h"` in alphabetical order
   - Add `StripeShiftEffect stripeShift;` member to `PostEffect` struct

3. **`CMakeLists.txt`**:
   - Add `src/effects/stripe_shift.cpp` to `EFFECTS_SOURCES` list

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/stripe_shift.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(StripeShiftConfig, STRIPE_SHIFT_CONFIG_FIELDS)` with other per-config macros
   - Add `X(stripeShift) \` to `EFFECT_CONFIG_FIELDS(X)` macro

**Verify**: All 4 files modified. Build succeeds after all Wave 2 tasks complete.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Retro section (section 6) of transforms panel
- [ ] Effect shows "RET" category badge
- [ ] Enabling shows 7 parameter sliders
- [ ] Stripes respond to input brightness (displacement works)
- [ ] Color Displace slider shifts from monochrome to per-channel displacement
- [ ] Hard Edge slider transitions from smooth curves to binary kinks
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to all 7 registered parameters
