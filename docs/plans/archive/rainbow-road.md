# Rainbow Road

Frequency-mapped light bars receding in perspective from one screen edge to the other. Each bar corresponds to an FFT frequency band — low frequencies get big bold bars up front, high frequencies shimmer as thin lines in the distance. Bars glow and swell with audio energy, swaying and curving with configurable wobble. A neon highway that breathes with the music.

**Research**: `docs/research/rainbow_road.md`

## Design

### Types

```c
// rainbow_road.h

struct RainbowRoadConfig {
  bool enabled = false;

  // Geometry
  int layers = 32;            // Number of bars / frequency bands (4-64)
  int direction = 0;          // 0 = recede upward, 1 = recede downward
  float perspective = 1.0f;   // Depth multiplier per bar (0.1-3.0)
  float maxWidth = 4.0f;      // Bar half-width ceiling in world units (0.5-8.0)
  float sway = 1.0f;          // Lateral offset amplitude (0.0-3.0)
  float curvature = 0.1f;     // Per-bar tilt (0.0-1.0)
  float phaseSpread = 0.8f;   // Sway/curvature phase multiplier (0.1-3.0)

  // Glow
  float glowIntensity = 1.0f; // Glow brightness (0.1-5.0)

  // Audio
  float baseFreq = 55.0f;     // Lowest mapped frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f;   // Highest mapped frequency in Hz (1000-16000)
  float gain = 2.0f;          // FFT energy multiplier (0.1-10.0)
  float curve = 1.5f;         // Energy response curve (0.1-3.0)
  float baseBright = 0.15f;   // Brightness floor (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct RainbowRoadEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;               // Animation accumulator for sway
  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int layersLoc;
  int directionLoc;
  int perspectiveLoc;
  int maxWidthLoc;
  int swayLoc;
  int curvatureLoc;
  int phaseSpreadLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} RainbowRoadEffect;
```

### Algorithm

Attribution: Based on "Rainbow Road" by XorDev (https://www.shadertoy.com/view/NlGfzz), CC BY-NC-SA 3.0.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform int layers;
uniform int direction;
uniform float perspective;
uniform float maxWidth;
uniform float sway;
uniform float curvature;
uniform float phaseSpread;
uniform float glowIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    // Centered, aspect-corrected UV: (0,0) at center, Y spans -0.5 to +0.5
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

    // Direction: 0 = recede upward (big at bottom), 1 = recede downward (big at top)
    if (direction == 1) uv.y = -uv.y;

    vec3 color = vec3(0.0);

    for (int i = 0; i < layers; i++) {
        float fi = float(i);

        // --- FFT energy (standard BAND_SAMPLES pattern) ---
        float t0 = fi / float(layers - 1);
        float t1 = float(i + 1) / float(layers);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // --- Perspective depth ---
        // depth = 1 at bar 0 (nearest), grows linearly per bar
        float depth = 1.0 + fi * perspective;

        // --- Position in bar's local space ---
        vec2 p;
        p.x = uv.x * depth + sway * cos(fi * phaseSpread + time);
        p.y = (uv.y + 0.5) * depth - fi;
        // +0.5: shifts UV origin to bottom edge before scaling
        // Bar i center at screen-Y = fi / depth - 0.5

        // --- Bar segment: horizontal with per-bar tilt ---
        float hw = maxWidth * brightness;                   // half-width scales with energy
        float tilt = curvature * sin(fi * phaseSpread);     // vertical component at endpoints
        vec2 barEnd = vec2(hw, tilt * hw);                  // endpoint relative to bar center

        // --- Segment SDF ---
        float proj = clamp(dot(p, barEnd) / dot(barEnd, barEnd), -1.0, 1.0);
        float dist = length(p - proj * barEnd);

        // --- Glow with depth fade ---
        float depthFade = 1.0 / max(depth * depth, 5.0);
        float glow = glowIntensity * depthFade / (dist / depth + 0.001);

        // --- Color from gradient LUT ---
        float t = fi / float(layers - 1);
        vec3 barColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
        color += glow * barColor * brightness;
    }

    finalColor = vec4(color, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| direction | int | 0-1 | 0 | No | Direction (Combo: "Up" / "Down") |
| perspective | float | 0.1-3.0 | 1.0 | Yes | Perspective |
| maxWidth | float | 0.5-8.0 | 4.0 | Yes | Max Width |
| sway | float | 0.0-3.0 | 1.0 | Yes | Sway |
| curvature | float | 0.0-1.0 | 0.1 | Yes | Curvature |
| phaseSpread | float | 0.1-3.0 | 0.8 | Yes | Phase Spread |
| glowIntensity | float | 0.1-5.0 | 1.0 | Yes | Glow Intensity |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| layers | int | 4-64 | 32 | No | Layers |

### Constants

- Enum name: `TRANSFORM_RAINBOW_ROAD_BLEND`
- Display name: `"Rainbow Road"`
- Field name: `rainbowRoad`
- Category: Texture (section 12)
- Badge: `"GEN"` (auto-set by REGISTER_GENERATOR)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by REGISTER_GENERATOR)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Config and Effect Structs

**Files**: `src/effects/rainbow_road.h` (create)
**Creates**: Config struct, Effect struct, function declarations needed by all Wave 2 tasks

**Do**: Create the header following `filaments.h` pattern. Use the struct layouts from the Design section. Include `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`. Forward-declare `ColorLUT`. Define `RAINBOW_ROAD_CONFIG_FIELDS` macro listing all serializable fields. Declare standard generator lifecycle: `Init(Effect*, Config*)`, `Setup(Effect*, Config*, deltaTime, fftTexture)`, `Uninit`, `ConfigDefault`, `RegisterParams`.

**Verify**: Header compiles when included.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Shader

**Files**: `shaders/rainbow_road.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from this plan verbatim. Add attribution comment block before `#version`:
```
// Based on "Rainbow Road" by XorDev
// https://www.shadertoy.com/view/NlGfzz
// License: CC BY-NC-SA 3.0
// Modified: FFT-reactive bars, configurable perspective/sway/curvature, gradient LUT coloring
```

**Verify**: Shader parses (no syntax errors at runtime).

---

#### Task 2.2: Effect Module

**Files**: `src/effects/rainbow_road.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement following `filaments.cpp` as the structural pattern:
- `RainbowRoadEffectInit`: Load shader, cache all uniform locations, init ColorLUT. Cascade cleanup on failure.
- `RainbowRoadEffectSetup`: Accumulate `time += deltaTime`, update ColorLUT, bind all uniforms including `fftTexture` and `gradientLUT`.
- `RainbowRoadEffectUninit`: Unload shader, free LUT.
- `RainbowRoadConfigDefault`: Return `RainbowRoadConfig{}`.
- `RainbowRoadRegisterParams`: Register all modulatable params (all floats from Design section). Use `"rainbowRoad.<field>"` IDs. Bounds must match the ranges in the config struct.
- Bridge functions: `SetupRainbowRoad(PostEffect*)` for generator shader setup, `SetupRainbowRoadBlend(PostEffect*)` for blend compositor.
- `direction` uses `ImGui::Combo` with items `"Up\0Down\0"`, not a slider.
- `layers` uses `ImGui::SliderInt`.
- UI section order: Audio (Base Freq, Max Freq, Gain, Contrast, Base Bright), Geometry (Layers, Direction combo, Perspective, Max Width, Sway, Curvature, Phase Spread), Glow (Glow Intensity).
- `STANDARD_GENERATOR_OUTPUT(rainbowRoad)` macro before registration.
- `REGISTER_GENERATOR(TRANSFORM_RAINBOW_ROAD_BLEND, RainbowRoad, rainbowRoad, "Rainbow Road", SetupRainbowRoadBlend, SetupRainbowRoad, 12, DrawRainbowRoadParams, DrawOutput_rainbowRoad)` at bottom.
- Includes: follow generator `.cpp` include groups from conventions.md.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Config Registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/rainbow_road.h"` with other effect includes
2. Add `TRANSFORM_RAINBOW_ROAD_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT` (place after the last texture generator, near `TRANSFORM_SCRAWL_BLEND`)
3. Add `RainbowRoadConfig rainbowRoad;` to `EffectConfig` struct

**Verify**: Header compiles.

---

#### Task 2.4: PostEffect Struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/rainbow_road.h"` with other effect includes
2. Add `RainbowRoadEffect rainbowRoad;` to `PostEffect` struct

**Verify**: Header compiles.

---

#### Task 2.5: Build System

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/rainbow_road.cpp` to `EFFECTS_SOURCES`.

**Verify**: CMake configure succeeds.

---

#### Task 2.6: Serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/rainbow_road.h"`
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RainbowRoadConfig, RAINBOW_ROAD_CONFIG_FIELDS)`
3. Add `X(rainbowRoad) \` to `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Geometric generator section of UI
- [ ] Direction combo switches recession direction
- [ ] FFT energy drives bar brightness (not width)
- [ ] Gradient LUT colors bars from low to high frequency
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters

---

## Implementation Notes

### Critical: Stay close to the reference

The reference shader is 10 lines and produces the correct visual. Every attempt to "improve" or "generalize" the coordinate math broke the effect. The final working shader is a near-verbatim transcription of the reference with only these substitutions:

| Reference | Final Implementation | Why |
|-----------|---------------------|-----|
| `cos(i+vec4(0,2,4,0))+1.` | `gradientLUT * brightness` | LUT coloring + FFT reactivity |
| `cos(i*vec2(.8,.5)+iTime)` | `sway * cos(i*phaseSpread+time)` with 0.625 y ratio | Configurable sway |
| `vec2(4, sin(i)*.4)` | `vec2(width, curvature*sin(i*phaseSpread))` | Configurable bar width and tilt |
| `fract(-iTime)` | `scroll` (CPU-accumulated) | Configurable speed + direction |
| Fixed step `0.5` | `25.0/layers` | Configurable bar count |

Everything else — the `4.0-i` offset, `max(i*i,5.)`, `0.1` scale, `dist/i + i/1e3` attenuation, `(I+I-r)/r.y*i` coordinate transform — is **verbatim from the reference**. Do not change these.

### What failed and why

1. **Custom perspective projection** (`depth = 1 + i * perspective`, explicit screen positioning with `pow(normI, perspective)`): Broke bar spacing, caused bars to blow out or disappear. The reference's `i` as both depth and position is what makes it work — decoupling them requires retuning every constant.

2. **Reactive width** (`hw = maxWidth * brightness`): Looked bad in practice. Width is now fixed at `width`; only brightness reacts to FFT energy.

3. **sqrt/linear depth fade** (replacing `1/max(i²,5)`): Either too bright (blown out near bars) or wrong falloff character. The reference's quadratic fade with `max(...,5)` floor is tuned for this specific coordinate system.

4. **Normalized fade indices** (decoupling fade from geometry): Created mismatches between glow radius and bar size, producing halos and artifacts.

### Parameters removed from original plan

- **`perspective`**: Removed entirely. The reference has no separate perspective control — perspective is intrinsic to the `uv*i` coordinate scaling. Adding one required retuning every constant and never worked.

### Parameters changed from original plan

- **`maxWidth` → `width`**: Renamed. No longer a "ceiling" since reactive width was removed. Just the bar half-width.
- **`curvature`**: Default changed 0.1 → 0.4 to match reference's `sin(i)*0.4`.
- **`speed`**: Added post-plan. CPU-accumulated (`e->scroll += deltaTime * speed`), passed as `fract` to shader. Positive = toward viewer, negative = away.

### Category

Changed from Texture (section 12) to Geometric (section 10).

### Key shader math (do not modify)

```glsl
// These values are tuned together. Changing one breaks the others.
o = (I + I - r) / r.y * i + sway;   // perspective via i multiplication
o.y += 4.0 - i;                      // bar vertical positioning
glow = G * 0.2 / max(i*i, 5.0)       // quadratic depth fade, floored at 5
     / (dist / i + i / 1000.0);       // distance atten normalized by depth
```
