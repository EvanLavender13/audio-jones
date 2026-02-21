# Hex Rush

Super Hexagon-inspired geometric generator: sharp radial wall segments rush inward toward a pulsing center polygon over alternating colored background wedges, all rotating with configurable snap and intensity. A "difficulty" meta-parameter simultaneously scales speed, density, and gap tightness in the shader for per-frame modulation response.

**Research**: `docs/research/hex-rush.md`

## Design

### Types

```
struct HexRushConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;     // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f;   // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;          // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.0f;         // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.1f;    // Minimum brightness floor (0.0-1.0)

  // Geometry
  int sides = 6;              // Number of angular segments (3-12)
  float centerSize = 0.15f;   // Center polygon radius (0.05-0.5)
  float wallThickness = 0.1f; // Radial thickness of wall bands (0.02-0.3)
  float wallSpacing = 0.5f;   // Distance between wall rings (0.2-2.0)

  // Dynamics
  float difficulty = 0.5f;    // Meta-parameter scaling speed/density/gaps (0.0-1.0)
  float wallSpeed = 3.0f;     // Base inward rush speed (0.5-10.0)
  float gapChance = 0.35f;    // Probability a segment is open per ring (0.1-0.8)
  float rotationSpeed = 0.5f; // Global rotation rate (rad/s, -PI..PI)
  float flipRate = 1.0f;      // Rotation direction reversal frequency Hz (0.0-5.0)
  float pulseAmount = 0.1f;   // Center polygon pulse intensity (0.0-0.5)
  float patternSeed = 0.0f;   // Seed for wall pattern hash (0.0-100.0)

  // Visual
  float perspective = 0.3f;   // Pseudo-3D perspective distortion (0.0-1.0)
  float bgContrast = 0.3f;    // Brightness diff between alternating segments (0.0-1.0)
  float wallGlow = 0.5f;      // Soft glow width on wall edges (0.0-2.0)
  float glowIntensity = 1.0f; // Overall brightness multiplier (0.1-3.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

Config fields macro:
```
#define HEX_RUSH_CONFIG_FIELDS \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, \
  sides, centerSize, wallThickness, wallSpacing, \
  difficulty, wallSpeed, gapChance, rotationSpeed, flipRate, \
  pulseAmount, patternSeed, \
  perspective, bgContrast, wallGlow, glowIntensity, \
  gradient, blendMode, blendIntensity
```

Effect struct:
```
typedef struct HexRushEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum;  // CPU-accumulated rotation phase
  float flipAccum;      // CPU-accumulated flip timer
  float rotationDir;    // Current rotation direction (+1 or -1)
  float pulseAccum;     // CPU-accumulated pulse timer
  float time;           // Raw elapsed time accumulator (shader applies wallSpeed via difficulty)
  float wobbleTime;     // Separate time for perspective wobble (independent of wallSpeed)
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int sidesLoc;
  int centerSizeLoc;
  int wallThicknessLoc;
  int wallSpacingLoc;
  int difficultyLoc;
  int wallSpeedLoc;
  int gapChanceLoc;
  int rotationAccumLoc;
  int pulseAmountLoc;
  int pulseAccumLoc;
  int patternSeedLoc;
  int perspectiveLoc;
  int bgContrastLoc;
  int wallGlowLoc;
  int glowIntensityLoc;
  int timeLoc;
  int wobbleTimeLoc;
  int gradientLUTLoc;
} HexRushEffect;
```

### Algorithm

Polar-coordinate fragment shader with hash-based procedural wall patterns. The shader receives a raw elapsed `time` (accumulated as `+= deltaTime` in C++, shader applies wallSpeed via difficulty scaling), a separate `wobbleTime` for perspective wobble (also `+= deltaTime`, decoupled from wallSpeed), and a `rotationAccum` that incorporates periodic direction flips and difficulty-scaled rotation speed.

**Coordinate setup:**
1. Center UV relative to screen center with aspect correction: `uv = (fragCoord - 0.5 * resolution) / resolution.y`
2. Apply perspective distortion using `wobbleTime` (independent of wallSpeed): `uv /= dot(uv, vec2(sin(wobbleTime * 0.34), sin(wobbleTime * 0.53))) * perspective + 1.0`
3. Convert to polar: `r = length(uv)`, `theta = atan(uv.y, uv.x) + rotationAccum`
4. Compute section index: `zone = floor(float(sides) * mod(theta + PI, TAU) / TAU)`, `odd = mod(zone, 2.0)`

**N-gon corrected radius** (from SHAPE macro pattern):
```glsl
float slice = TAU / float(sides);
float hexR = cos(floor(0.5 + theta / slice) * slice - theta) * r;
```
This corrects radial distance to be perpendicular to polygon edges rather than measured from center.

**Background:** Alternating segments using `odd`:
```glsl
vec3 bgColor = mix(baseBg, baseBg * (1.0 + bgContrast), odd);
```
Where `baseBg` comes from gradient LUT sampled at a fixed low `t` value (e.g. 0.2).

**Difficulty meta-parameter** (applied in shader):
```glsl
float effSpeed = wallSpeed * (1.0 + difficulty * 2.0);
float effGap = gapChance * (1.0 - difficulty * 0.5);
float effSpacing = wallSpacing * (1.0 - difficulty * 0.3);
```

**Wall depth and procedural generation:**
```glsl
float depth = hexR * 10.0 + time * effSpeed;  // time is raw elapsed; effSpeed provides wallSpeed * difficulty scaling
float ringIndex = floor(depth / effSpacing);
float ringFrac = fract(depth / effSpacing);
float segIndex = zone;

// Hash-based wall presence
float wallHash = fract(sin(ringIndex * 127.1 + segIndex * 311.7 + patternSeed) * 43758.5453);
float hasWall = step(effGap, wallHash);

// Guarantee at least one gap per ring
float gapSeg = floor(fract(sin(ringIndex * 269.3 + patternSeed) * 43758.5453) * float(sides));
if (segIndex == gapSeg) hasWall = 0.0;

// Wall rendering with thickness and glow
float wallDist = abs(ringFrac - 0.5) * 2.0 * effSpacing;
float wallMask = smoothstep(wallThickness * 0.5 + wallGlow * 0.01, wallThickness * 0.5, wallDist);
wallMask *= hasWall;
```

**FFT wall brightness:** Semitone lookup per ring via `fftTexture`:
```glsl
float freqT = clamp((ringIndex - minRing) / (maxRing - minRing), 0.0, 1.0);
float freq = baseFreq * pow(maxFreq / baseFreq, freqT);
float bin = freq / (sampleRate / 2.0) * 1024.0;
float mag = texture(fftTexture, vec2(bin / 1025.0, 0.5)).r;
float brightness = baseBright + pow(mag * gain, curve) * (1.0 - baseBright);
```

**Center polygon:** N-gon outline using `hexR`:
```glsl
float pulseR = centerSize + sin(pulseAccum) * pulseAmount;
float centerOutline = smoothstep(pulseR + 0.015, pulseR + 0.005, hexR)
                    * (1.0 - smoothstep(pulseR - 0.005, pulseR - 0.015, hexR));
```

**Color compositing:**
```glsl
float colorT = ringFrac; // or use ring index for discrete color bands
vec3 wallColor = texture(gradientLUT, vec2(colorT, 0.5)).rgb;
vec3 color = bgColor;
color = mix(color, wallColor * brightness * glowIntensity, wallMask);
color = mix(color, wallColor, centerOutline);
color *= smoothstep(1.8, 0.5, length(uv)); // vignette
finalColor = vec4(color, 1.0);
```

**C++ Setup — flip logic:**
```
flipAccum += flipRate * deltaTime;
if (flipRate > 0 && fract(flipAccum) < fract(flipAccum - flipRate * deltaTime)) {
    rotationDir *= -1.0f;
}
// Difficulty scales rotation speed: effectiveRotSpeed = rotationSpeed * (1 + difficulty * 1.5)
float effRotSpeed = rotationSpeed * (1.0f + difficulty * 1.5f);
rotationAccum += effRotSpeed * rotationDir * deltaTime;
```
Pass `rotationAccum` as a float uniform. `rotationDir` stays in C++ only.

**C++ Setup — pulse and time:**
```
pulseAccum += 6.0f * deltaTime;   // ~1Hz pulse cycle
time += deltaTime;                // raw elapsed time (shader applies wallSpeed via difficulty)
wobbleTime += deltaTime;          // independent wobble clock (decoupled from wallSpeed)
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.0 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.1 | Yes | Base Bright |
| sides | int | 3-12 | 6 | No | Sides |
| centerSize | float | 0.05-0.5 | 0.15 | No | Center Size |
| wallThickness | float | 0.02-0.3 | 0.1 | No | Wall Thickness |
| wallSpacing | float | 0.2-2.0 | 0.5 | No | Wall Spacing |
| difficulty | float | 0.0-1.0 | 0.5 | Yes | Difficulty |
| wallSpeed | float | 0.5-10.0 | 3.0 | Yes | Wall Speed |
| gapChance | float | 0.1-0.8 | 0.35 | Yes | Gap Chance |
| rotationSpeed | float | -PI..PI | 0.5 | Yes | Rotation Speed |
| flipRate | float | 0.0-5.0 | 1.0 | No | Flip Rate |
| pulseAmount | float | 0.0-0.5 | 0.1 | Yes | Pulse Amount |
| patternSeed | float | 0.0-100.0 | 0.0 | Yes | Pattern Seed |
| perspective | float | 0.0-1.0 | 0.3 | Yes | Perspective |
| bgContrast | float | 0.0-1.0 | 0.3 | No | BG Contrast |
| wallGlow | float | 0.0-2.0 | 0.5 | No | Wall Glow |
| glowIntensity | float | 0.1-3.0 | 1.0 | Yes | Glow Intensity |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_HEX_RUSH_BLEND`
- Display name: `"Hex Rush Blend"`
- Category badge: `"GEN"` (auto via `REGISTER_GENERATOR`)
- Section index: 10 (auto via `REGISTER_GENERATOR`)
- Field name: `hexRush` (on both `EffectConfig` and `PostEffect`)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create hex_rush.h

**Files**: `src/effects/hex_rush.h` (create)
**Creates**: `HexRushConfig` struct, `HexRushEffect` struct, function declarations

**Do**: Create the header following `signal_frames.h` as pattern. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Define `HexRushConfig` struct with all fields from the Design section. Define `HEX_RUSH_CONFIG_FIELDS` macro. Forward-declare `ColorLUT`. Define `HexRushEffect` typedef struct with all uniform location ints and accumulators from the Design section. Declare `HexRushEffectInit` (takes `HexRushEffect*` and `const HexRushConfig*`), `HexRushEffectSetup` (takes `HexRushEffect*`, `const HexRushConfig*`, `float deltaTime`, `Texture2D fftTexture`), `HexRushEffectUninit`, `HexRushConfigDefault`, `HexRushRegisterParams`.

**Verify**: `cmake.exe --build build` compiles (header-only, included nowhere yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create hex_rush.cpp

**Files**: `src/effects/hex_rush.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement effect module following `signal_frames.cpp` as pattern. Includes: own header, `"audio/audio.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `<stddef.h>`.

`HexRushEffectInit`: Load `shaders/hex_rush.fs`, cache all uniform locations (including `wobbleTimeLoc`), init `ColorLUT` from config gradient, zero all accumulators, set `rotationDir = 1.0f`.

`HexRushEffectSetup`: Implement flip logic from the Algorithm section (accumulate flipAccum, toggle rotationDir on cycle boundary). Apply difficulty-scaled rotation: `effRotSpeed = rotationSpeed * (1.0f + difficulty * 1.5f)`, then `rotationAccum += effRotSpeed * rotationDir * deltaTime`. Accumulate `pulseAccum += 6.0f * deltaTime`. Accumulate `time += deltaTime` (raw elapsed, NOT `wallSpeed * deltaTime` — shader applies wallSpeed via difficulty). Accumulate `wobbleTime += deltaTime` (independent wobble clock). Update ColorLUT. Bind all uniforms including resolution, fftTexture, sampleRate, wobbleTime.

`HexRushEffectUninit`: Unload shader, uninit ColorLUT.

`HexRushConfigDefault`: Return `HexRushConfig{}`.

`HexRushRegisterParams`: Register modulatable params per the Parameters table (only fields marked Modulatable=Yes). Use `"hexRush.fieldName"` IDs. `rotationSpeed` bounds: `-ROTATION_SPEED_MAX` to `ROTATION_SPEED_MAX`.

Bridge functions: `SetupHexRush(PostEffect* pe)` calls `HexRushEffectSetup`, `SetupHexRushBlend(PostEffect* pe)` calls `BlendCompositorApply`.

Registration macro at bottom: `REGISTER_GENERATOR(TRANSFORM_HEX_RUSH_BLEND, HexRush, hexRush, "Hex Rush Blend", SetupHexRushBlend, SetupHexRush)`.

**Verify**: Compiles after Wave 2 integration tasks complete.

---

#### Task 2.2: Create hex_rush.fs shader

**Files**: `shaders/hex_rush.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the fragment shader following the Algorithm section exactly. Declare all uniforms matching the locations cached in `hex_rush.cpp`. Use `#version 330`. Input `fragTexCoord`, output `finalColor`. Uniforms: `resolution`, `fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`, `sides`, `centerSize`, `wallThickness`, `wallSpacing`, `difficulty`, `wallSpeed`, `gapChance`, `rotationAccum`, `pulseAmount`, `pulseAccum`, `patternSeed`, `perspective`, `bgContrast`, `wallGlow`, `glowIntensity`, `time`, `wobbleTime`, `gradientLUT`.

Implement the full rendering pipeline from the Algorithm section: coordinate setup (centered, aspect-corrected), perspective distortion (uses `wobbleTime`, NOT `time`), polar conversion, N-gon corrected radius, alternating background, difficulty scaling, wall generation with hash, gap guarantee, wall rendering with smoothstep glow, FFT brightness per ring, center polygon outline with pulse, gradient LUT coloring, vignette.

Remember: center coordinates relative to screen center (not bottom-left). The `time` uniform is raw elapsed time; the shader computes `effSpeed = wallSpeed * (1 + difficulty * 2)` and uses `time * effSpeed` for wall depth. Perspective wobble uses `wobbleTime` (decoupled from wallSpeed).

**Verify**: Shader loads at runtime (no build step needed for `.fs` files).

---

#### Task 2.3: Config registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow the add-effect checklist Phase 2.
1. Add `#include "effects/hex_rush.h"` in alphabetical order among the effect includes.
2. Add `TRANSFORM_HEX_RUSH_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`.
3. Add `HexRushConfig hexRush;` to `EffectConfig` struct with comment `// Hex Rush (Super Hexagon-inspired geometric generator)`.

**Verify**: Compiles.

---

#### Task 2.4: PostEffect struct integration

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow add-effect checklist Phase 4.
1. Add `#include "effects/hex_rush.h"` in alphabetical order among effect includes.
2. Add `HexRushEffect hexRush;` member to `PostEffect` struct (among the generator effects, near `SignalFramesEffect signalFrames`).

**Verify**: Compiles.

---

#### Task 2.5: Build system registration

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/hex_rush.cpp` to `EFFECTS_SOURCES` in alphabetical order.

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` compiles.

---

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow add-effect checklist Phase 7.
1. Add `#include "effects/hex_rush.h"` in alphabetical order among effect includes.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HexRushConfig, HEX_RUSH_CONFIG_FIELDS)` with the other per-config macros.
3. Add `X(hexRush)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Compiles.

---

#### Task 2.7: UI panel

**Files**: `src/ui/imgui_effects_gen_geometric.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add Hex Rush to the Geometric generator sub-category UI, following the Signal Frames pattern.
1. Add `#include "effects/hex_rush.h"` to the includes.
2. Add `static bool sectionHexRush = false;` with the other section bools.
3. Create `DrawHexRushParams(HexRushConfig*, const ModSources*)`:
   - **Audio** section: Base Freq, Max Freq, Gain, Contrast, Base Bright (standard order/format)
   - **Geometry** section: `ImGui::SliderInt("Sides##hexrush", ...)` (3-12), `ModulatableSlider` for Center Size, Wall Thickness, Wall Spacing (non-modulatable params still use `ImGui::SliderFloat`)
   - **Dynamics** section: `ModulatableSlider` for Difficulty, Wall Speed, Gap Chance, `ModulatableSliderSpeedDeg` for Rotation Speed, `ImGui::SliderFloat` for Flip Rate, `ModulatableSlider` for Pulse Amount, Pattern Seed
   - **Visual** section: `ModulatableSlider` for Perspective, `ImGui::SliderFloat` for BG Contrast, Wall Glow, `ModulatableSlider` for Glow Intensity
4. Create `DrawHexRushOutput(HexRushConfig*, const ModSources*)`: Color mode + Blend Intensity + Blend Mode combo (same pattern as Signal Frames).
5. Create `DrawGeneratorsHexRush(EffectConfig*, const ModSources*, ImU32)`: Section begin/end, enabled checkbox with `MoveTransformToEnd(&e->transformOrder, TRANSFORM_HEX_RUSH_BLEND)`, call Params then Output.
6. Add `ImGui::Spacing(); DrawGeneratorsHexRush(e, modSources, categoryGlow);` at end of `DrawGeneratorsGeometric`.

Use `##hexrush` suffix for all ImGui IDs. Non-modulatable float params use `ImGui::SliderFloat` (not `ModulatableSlider`).

**Verify**: Compiles.

---

## Implementation Notes

Changes from original plan made during implementation:

- **Removed `difficulty` parameter**: Shader-side scaling of wallSpeed/gapChance/wallSpacing made those sliders meaningless. Removed entirely.
- **`wallSpeed` accumulates on CPU**: `wallAccum += wallSpeed * deltaTime` in Setup, shader receives `wallAccum`. Prevents jumps when modulating wallSpeed. Removed `wallSpeed` and `time` uniforms from shader.
- **Added `colorSpeed` parameter** (0.0-1.0, default 0.1): Controls color cycle rate through gradient. Accumulated on CPU as `colorAccum`.
- **Added `pulseSpeed` parameter** (0.0-2.0, default 0.3): Replaces hardcoded `6.0` pulse rate.
- **Walls stop at center polygon**: `wallMask *= smoothstep(pulseR, pulseR + 0.02, hexR)` clips walls at center edge. Center interior fills with dark background color, not black.
- **FFT uses standard BAND_SAMPLES pattern**: 20 frequency bands cycling across rings, 4-sample averaging per band matching signal_frames/spectral_arcs convention.
- **Wall glow reworked**: Separate `wallCore` (hard edge) and `wallSoft` (glow extension) so glow=0 gives hard edges without making walls thicker.
- **Flat coloring**: Walls and center outline share a single slowly cycling gradient color (driven by `colorAccum`), not per-ring gradient. Matches Super Hexagon flat aesthetic.
- **Default tuning**: wallSpeed 3.0→1.5, flipRate 1.0→0.1 (max 1.0), pulseAmount 0.1→0.02, wallThickness 0.1→0.15 (max 0.6).

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Geometric generator section
- [ ] Hex Rush shows "GEN" category badge in transform order
- [ ] Enabling shows alternating background wedges with rushing walls
- [ ] Walls scroll inward at configurable speed
- [ ] Difficulty slider simultaneously affects speed, gap density, spacing, and rotation speed
- [ ] Rotation flips direction at flipRate frequency
- [ ] Center polygon pulses
- [ ] FFT audio drives wall brightness
- [ ] Gradient LUT colors walls and background
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
