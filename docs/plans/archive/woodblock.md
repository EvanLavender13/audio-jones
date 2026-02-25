# Woodblock

Japanese ukiyo-e print effect: posterized flat color fields with Sobel keyblock outlines, directional wood grain texture in inked areas, per-layer registration offsets, and warm paper base. Single-pass transform shader combining toon.fs posterization/Sobel, risograph.fs simplex noise/registration drift, and Godot reference wood grain.

**Research**: `docs/research/woodblock.md`

## Design

### Types

**WoodblockConfig** (in `src/effects/woodblock.h`):

```
bool enabled = false;
int levels = 5;                      // Posterization steps (2-12)
float edgeThreshold = 0.25f;         // Sobel sensitivity (0.0-1.0)
float edgeSoftness = 0.05f;          // Outline transition width (0.0-0.5)
float edgeThickness = 1.2f;          // Outline width multiplier (0.5-3.0)
float grainIntensity = 0.25f;        // Wood grain visibility in inked areas (0.0-1.0)
float grainScale = 8.0f;             // Grain pattern fineness (1.0-20.0)
float grainAngle = 0.0f;             // Grain direction (radians, -PI..PI)
float registrationOffset = 0.004f;   // Color layer misalignment (0.0-0.02)
float registrationSpeed = 0.3f;      // Registration drift rate (radians/s, -PI..PI)
float inkDensity = 1.0f;             // Ink coverage strength (0.3-1.5)
float paperTone = 0.3f;              // Paper warmth 0=white, 1=cream (0.0-1.0)
```

**WoodblockEffect** (in `src/effects/woodblock.h`):

```
Shader shader;
int resolutionLoc;
int levelsLoc;
int edgeThresholdLoc;
int edgeSoftnessLoc;
int edgeThicknessLoc;
int grainIntensityLoc;
int grainScaleLoc;
int grainAngleLoc;
int registrationOffsetLoc;
int registrationTimeLoc;
int inkDensityLoc;
int paperToneLoc;
float registrationTime;   // registrationSpeed accumulator
```

### Algorithm

Single-pass fragment shader. No compute, no extra render textures.

#### Noise functions

Copy risograph.fs `snoise()` verbatim (simplex 2D — `mod289`, `permute`, `snoise`). Used for registration spatial warp.

Add value noise for wood grain (from Godot reference):

```glsl
vec2 grainRandom(vec2 pos) {
    return fract(sin(vec2(
        dot(pos, vec2(12.9898, 78.233)),
        dot(pos, vec2(-148.998, -65.233))
    )) * 43758.5453);
}

float valueNoise(vec2 pos) {
    vec2 p = floor(pos);
    vec2 f = fract(pos);
    float v00 = grainRandom(p).x;
    float v10 = grainRandom(p + vec2(1.0, 0.0)).x;
    float v01 = grainRandom(p + vec2(0.0, 1.0)).x;
    float v11 = grainRandom(p + vec2(1.0, 1.0)).x;
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(v00, v10, u.x), mix(v01, v11, u.x), u.y);
}
```

#### Wood grain generation

Rotate UV by `grainAngle`, then apply the Godot pattern:

```glsl
float woodGrain(vec2 uv, float scale, float angle) {
    float ca = cos(angle), sa = sin(angle);
    vec2 ruv = vec2(ca * uv.x + sa * uv.y, -sa * uv.x + ca * uv.y);
    ruv.x += valueNoise(ruv * 2.0);
    float x = ruv.x + sin(ruv.y * 4.0);
    return mod(x * scale + grainRandom(ruv).x * 0.3, 1.0);
}
```

Returns 0..1 grain intensity. Only applied in inked areas (where posterized color is dark enough).

#### Posterization

Identical to toon.fs — max-RGB luminance quantization:

```glsl
float greyscale = max(color.r, max(color.g, color.b));
float fLevels = float(levels);
float lower = floor(greyscale * fLevels) / fLevels;
float upper = ceil(greyscale * fLevels) / fLevels;
float level = (abs(greyscale - lower) <= abs(upper - greyscale)) ? lower : upper;
float adjustment = level / max(greyscale, 0.001);
vec3 posterized = color.rgb * adjustment;
```

#### Sobel edge detection

Identical to toon.fs with noise-varied texel size for organic outlines. Use toon.fs `hash()` + `gnoise()` for thickness variation:

```glsl
float noise = gnoise(vec3(uv * 5.0, 0.0));
float thickMul = 1.0 + noise * 0.5;  // fixed 50% variation for woodcut feel
vec2 texel = edgeThickness * thickMul / resolution;

// 3x3 Sobel on un-offset center sample (same kernel as toon.fs)
// ... sobelH, sobelV, edge magnitude, smoothstep outline ...
float outline = smoothstep(edgeThreshold - edgeSoftness,
                           edgeThreshold + edgeSoftness, edge);
```

#### Registration offsets

Smooth sinusoidal drift only (no frame-jitter, unlike risograph). Three layer offsets:

```glsl
vec2 off0 = registrationOffset * vec2(sin(registrationTime * 0.7), cos(registrationTime * 0.9));
vec2 off1 = registrationOffset * vec2(sin(registrationTime * 1.1 + 2.0), cos(registrationTime * 0.6 + 1.0));
vec2 off2 = registrationOffset * vec2(sin(registrationTime * 0.8 + 4.0), cos(registrationTime * 1.3 + 3.0));

// Per-layer spatial warp (smooth, no frame-jitter)
vec2 centered = fragTexCoord - 0.5;
vec2 warp0 = registrationOffset * 0.15 * vec2(
    snoise(centered * 5.0 + vec2(0.0, registrationTime * 0.3)),
    snoise(centered * 5.0 + vec2(3.1, registrationTime * 0.3)));
// warp1, warp2 with different offsets

vec3 rgb0 = texture(texture0, uv + off0 + warp0).rgb;
vec3 rgb1 = texture(texture0, uv + off1 + warp1).rgb;
vec3 rgb2 = texture(texture0, uv + off2 + warp2).rgb;
```

Posterize each layer independently after sampling.

#### Compositing

Paper base → subtractive blending of 3 posterized RGB layers → multiplicative wood grain in inked areas → keyblock outline on top:

```glsl
// Paper base (same warm tint as risograph.fs)
vec3 paper = mix(vec3(1.0), vec3(0.96, 0.93, 0.88), paperTone);

// Average the 3 misregistered posterized layers to get ink color
vec3 ink = (post0 + post1 + post2) / 3.0;

// Subtractive compositing: ink darkens paper, inkDensity controls coverage
// At inkDensity=1.0 full ink on paper; at 0.3 ghostly transparent
vec3 result = paper * mix(vec3(1.0), ink, inkDensity);

// Wood grain: darken inked areas (darker pixels get more grain)
float inked = 1.0 - max(ink.r, max(ink.g, ink.b));
float grain = woodGrain((fragTexCoord - 0.5) * resolution / 200.0, grainScale, grainAngle);
result *= 1.0 - grainIntensity * inked * grain;

// Keyblock outline on top (black)
result = mix(result, vec3(0.0), outline);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| levels | int | 2–12 | 5 | Yes | `Levels##woodblock` |
| edgeThreshold | float | 0.0–1.0 | 0.25 | Yes | `Edge Threshold##woodblock` |
| edgeSoftness | float | 0.0–0.5 | 0.05 | Yes | `Edge Softness##woodblock` |
| edgeThickness | float | 0.5–3.0 | 1.2 | Yes | `Thickness##woodblock` |
| grainIntensity | float | 0.0–1.0 | 0.25 | Yes | `Grain##woodblock` |
| grainScale | float | 1.0–20.0 | 8.0 | Yes | `Grain Scale##woodblock` |
| grainAngle | float | -PI..PI | 0.0 | Yes | `Grain Angle##woodblock` (ModulatableSliderAngleDeg) |
| registrationOffset | float | 0.0–0.02 | 0.004 | Yes | `Misregister##woodblock` |
| registrationSpeed | float | -PI..PI | 0.3 | Yes | `Misreg Speed##woodblock` (ModulatableSliderSpeedDeg) |
| inkDensity | float | 0.3–1.5 | 1.0 | Yes | `Ink Density##woodblock` |
| paperTone | float | 0.0–1.0 | 0.3 | Yes | `Paper Tone##woodblock` |

### Constants

- Enum: `TRANSFORM_WOODBLOCK`
- Display name: `"Woodblock"`
- Category badge: `"GFX"` (Graphic)
- Section index: `5`
- Flags: `EFFECT_FLAG_NONE`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create effect header

**Files**: `src/effects/woodblock.h` (create)
**Creates**: `WoodblockConfig`, `WoodblockEffect`, function declarations

**Do**: Follow `src/effects/risograph.h` as pattern. Define `WoodblockConfig` with all 11 fields from the Parameters table above. Define `WoodblockEffect` struct with `Shader shader`, one `int *Loc` per uniform (12 locs: resolution, levels, edgeThreshold, edgeSoftness, edgeThickness, grainIntensity, grainScale, grainAngle, registrationOffset, registrationTime, inkDensity, paperTone), and `float registrationTime` accumulator. Declare `Init`, `Setup` (takes `const WoodblockConfig*` and `float deltaTime`), `Uninit`, `ConfigDefault`, `RegisterParams`. Define `WOODBLOCK_CONFIG_FIELDS` macro listing all fields.

**Verify**: Header compiles standalone.

---

### Wave 2: All Integration

All tasks in this wave touch different files — no overlap.

#### Task 2.1: Effect implementation + UI

**Files**: `src/effects/woodblock.cpp` (create)
**Depends on**: Wave 1

**Do**: Follow `src/effects/risograph.cpp` as pattern.

- `WoodblockEffectInit`: Load `shaders/woodblock.fs`, cache all 12 uniform locations, init `registrationTime = 0.0f`.
- `WoodblockEffectSetup`: Accumulate `registrationTime += cfg->registrationSpeed * deltaTime`. Set resolution vec2, bind all uniforms (levels as INT, everything else as FLOAT). Send accumulated `registrationTime` as the `registrationTime` uniform.
- `WoodblockEffectUninit`: Unload shader.
- `WoodblockConfigDefault`: Return `WoodblockConfig{}`.
- `WoodblockRegisterParams`: Register all 11 params with `ModEngineRegisterParam`. Use ranges from the Parameters table. `levels` is int but register as float range 2–12 for modulation compatibility. `grainAngle` and `registrationSpeed` use `ROTATION_OFFSET_MAX` / `ROTATION_SPEED_MAX` from constants.h.
- Bridge function `SetupWoodblock(PostEffect*)` — NOT static.
- UI section: `DrawWoodblockParams` with signature `(EffectConfig*, const ModSources*, ImU32)`. Use `ImGui::SliderInt` for levels (2–12). Use `ModulatableSlider` for edgeThreshold, edgeSoftness, edgeThickness. Use `ModulatableSlider` for grainIntensity, grainScale. Use `ModulatableSliderAngleDeg` for grainAngle. Use `ModulatableSlider` for registrationOffset ("%.4f"), `ModulatableSliderSpeedDeg` for registrationSpeed. Use `ModulatableSlider` for inkDensity, paperTone. Group into `TreeNodeAccented("Outline##woodblock")` for edge params, and `TreeNodeAccented("Wood Grain##woodblock")` for grain params.
- Registration macro: `REGISTER_EFFECT(TRANSFORM_WOODBLOCK, Woodblock, woodblock, "Woodblock", "GFX", 5, EFFECT_FLAG_NONE, SetupWoodblock, NULL, DrawWoodblockParams)`.

**Verify**: Compiles.

#### Task 2.2: Fragment shader

**Files**: `shaders/woodblock.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the Algorithm section above as a single-pass fragment shader. Uniforms: `sampler2D texture0`, `vec2 resolution`, `int levels`, `float edgeThreshold`, `float edgeSoftness`, `float edgeThickness`, `float grainIntensity`, `float grainScale`, `float grainAngle`, `float registrationOffset`, `float registrationTime`, `float inkDensity`, `float paperTone`.

Include all noise functions from the Algorithm section: `snoise` (copy from risograph.fs verbatim), `grainRandom`, `valueNoise`, `hash`+`gnoise` (copy from toon.fs for edge thickness variation), and `woodGrain`. Follow the pipeline order: registration offsets → posterize each layer → Sobel on center sample → wood grain → composite. All spatial operations center on `fragTexCoord - 0.5` (never raw fragTexCoord for noise/scaling).

**Verify**: Shader file exists with all uniforms declared.

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/woodblock.h"` with the other effect includes.
2. Add `TRANSFORM_WOODBLOCK,` before `TRANSFORM_EFFECT_COUNT` in the enum.
3. Add `WoodblockConfig woodblock;` to `EffectConfig` struct after the risograph member, with comment `// Woodblock (ukiyo-e posterization with keyblock outlines)`.

**Verify**: Compiles.

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/woodblock.h"` with other effect includes. Add `WoodblockEffect woodblock;` member in the PostEffect struct near `RisographEffect risograph`.

**Verify**: Compiles.

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/woodblock.cpp` to `EFFECTS_SOURCES` in alphabetical position (after `src/effects/watercolor.cpp` or near `src/effects/wave_ripple.cpp`).

**Verify**: `cmake.exe -G Ninja -B build -S .` succeeds.

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/woodblock.h"` with other effect includes.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WoodblockConfig, WOODBLOCK_CONFIG_FIELDS)` with other per-config macros.
3. Add `X(woodblock) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table continuation line (after the risograph entry).

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Effect appears in Graphic section of transform panel with "GFX" badge
- [ ] Enabling shows posterized flat colors with black outlines
- [ ] Wood grain visible as directional texture in dark/inked areas
- [ ] Registration offset produces subtle color layer misalignment
- [ ] grainAngle rotates the grain direction
- [ ] Preset save/load round-trips all 10 parameters
- [ ] All params appear in modulation target list

---

## Implementation Notes

- **Removed `registrationSpeed` and `registrationTime`**: Smooth sinusoidal drift on a misregistration offset doesn't make sense — real woodblock registration error is a static per-print accident, not an animation. The offset is now fixed per-layer directions with static spatial noise warp (via `snoise` on centered coords) that varies across the image like uneven paper pressure.
- **`levels` changed from `int` to `float`**: Stored as float for modulation compatibility. Cast to `int` before sending to shader. Uses `ModulatableSliderInt` in UI.
- **Grain visibility fix**: Changed `inked` metric from `1.0 - max(r,g,b)` to luminance-based `1.0 - dot(ink, vec3(0.299, 0.587, 0.114))`. The max-RGB approach made grain invisible on saturated colors (pure red/green/blue all have max=1.0). Luminance correctly shows grain on all non-white inked areas.
- **Flat UI with SeparatorText sections**: "Outline", "Wood Grain", "Print" — no TreeNodeAccented grouping.
- **Risograph tweaks**: Posterize slider min changed from 0 to 1; added SeparatorText sections ("Grain", "Print") to match.
