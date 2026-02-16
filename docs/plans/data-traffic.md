# Data Traffic

Horizontal lanes of scrolling rectangular data cells — a flat 2D data highway where each cell's color maps to a frequency band and pulses with FFT energy. Quiet passages show dim grayscale filler drifting by; loud moments light up scattered constellations of color across the grid. All animation is stateless (hash + time), no ping-pong buffers needed.

**Research**: `docs/research/data-traffic.md`

## Design

### Types

**DataTrafficConfig** (`src/effects/data_traffic.h`):

```
struct DataTrafficConfig {
  bool enabled = false;

  // Geometry
  int lanes = 20;               // Number of horizontal lanes (4-60)
  float cellWidth = 0.08f;      // Base cell width before random variation (0.01-0.3)
  float spacing = 3.0f;         // Cell spacing multiplier: slot = cellWidth * spacing (1.5-6.0)
  float gapSize = 0.08f;        // Dark gap between lanes as fraction of lane height (0.02-0.3)

  // Animation
  float scrollSpeed = 0.8f;     // Global scroll speed multiplier (0.0-3.0)
  float widthVariation = 0.6f;  // Cell width randomization amount (0.0-1.0)
  float colorMix = 0.5f;        // Fraction of cells that are colored/reactive vs grayscale (0.0-1.0)
  float jitter = 0.3f;          // Gentle positional jitter amplitude (0.0-1.0)
  float changeRate = 0.15f;     // How often cell widths and lane speeds re-randomize (0.05-0.5)

  // Audio
  float baseFreq = 55.0f;      // FFT low frequency bound Hz (27.5-440.0)
  float maxFreq = 14000.0f;    // FFT high frequency bound Hz (1000-16000)
  float gain = 2.0f;           // FFT magnitude amplification (0.1-10.0)
  float curve = 1.0f;          // FFT magnitude contrast curve (0.1-3.0)
  float baseBright = 0.05f;    // Minimum brightness for reactive cells (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

**DataTrafficEffect** (`src/effects/data_traffic.h`):

```
typedef struct DataTrafficEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;                   // Animation time accumulator
  int resolutionLoc;
  int timeLoc;
  int lanesLoc;
  int cellWidthLoc;
  int spacingLoc;
  int gapSizeLoc;
  int scrollSpeedLoc;
  int widthVariationLoc;
  int colorMixLoc;
  int jitterLoc;
  int changeRateLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} DataTrafficEffect;
```

Note: Unlike Scan Bars which accumulates scroll/color phases on the CPU, Data Traffic passes raw `time` to the shader. All per-lane scroll offsets and epoch timing are computed statelessly in the shader via hash functions.

### Algorithm

The shader is the core of this effect. All animation is stateless — deterministic from `time` and cell/lane indices via hash functions.

#### Utility functions (verbatim from research doc)

```glsl
float boxCov(float lo, float hi, float pc, float pw) {
    return clamp((min(hi, pc + pw) - max(lo, pc - pw)) / (2.0 * pw), 0.0, 1.0);
}

float h11(float p) { p = fract(p * 443.8975); p *= p + 33.33; return fract(p * p); }
float h21(vec2 p) { float n = dot(p, vec2(127.1, 311.7)); return fract(sin(n) * 43758.5453); }
```

#### Main loop (per pixel)

```glsl
void main() {
    vec2 uv = fragTexCoord;
    float aspect = resolution.x / resolution.y;

    // --- Lane identification ---
    float laneHeight = 1.0 / float(lanes);
    float laneF = uv.y / laneHeight;
    int laneIdx = int(floor(laneF));
    float withinLane = fract(laneF);

    // Lane gap mask: dark separator between lanes
    float gapMask = smoothstep(0.0, gapSize * 0.5, withinLane)
                  * smoothstep(1.0, 1.0 - gapSize * 0.5, withinLane);

    // --- Per-lane scroll ---
    // Each lane: hash-derived speed and direction, with epoch-based variation
    float laneHash = h11(float(laneIdx) * 7.31);
    float laneDir = (laneHash > 0.5) ? 1.0 : -1.0;
    float laneBaseSpeed = 0.3 + laneHash * 0.7;  // 0.3-1.0 range

    // Epoch-based speed variation: lanes periodically accelerate/coast/brake
    float speedEpoch = floor(time * changeRate + laneHash * 100.0);
    float speedProgress = fract(time * changeRate + laneHash * 100.0);
    float epochSpeedMul = mix(
        h21(vec2(speedEpoch, float(laneIdx) + 0.5)),
        h21(vec2(speedEpoch + 1.0, float(laneIdx) + 0.5)),
        smoothstep(0.0, 1.0, speedProgress)  // smooth easing between epochs
    );
    // ~20% of epochs are pause epochs (speed near zero)
    epochSpeedMul = (epochSpeedMul < 0.2) ? epochSpeedMul * 0.1 : epochSpeedMul;

    float laneSpeed = laneDir * laneBaseSpeed * epochSpeedMul * scrollSpeed;
    float scrolledX = uv.x * aspect + time * laneSpeed;

    // --- Cell identification ---
    float slotWidth = cellWidth * spacing;
    float cellIdxF = floor(scrolledX / slotWidth);

    vec3 color = vec3(0.0);
    float totalCov = 0.0;

    // Check current cell and neighbors for coverage
    for (int dc = -1; dc <= 1; dc++) {
        float idx = cellIdxF + float(dc);
        float cellCenter = (idx + 0.5) * slotWidth;

        // Epoch-based cell width variation
        float widthEpoch = floor(time * changeRate + h21(vec2(idx, float(laneIdx))) * 100.0);
        float widthProgress = fract(time * changeRate + h21(vec2(idx, float(laneIdx))) * 100.0);
        float widthRand = mix(
            h21(vec2(widthEpoch + idx, float(laneIdx) + 1.0)),
            h21(vec2(widthEpoch + idx + 1.0, float(laneIdx) + 1.0)),
            smoothstep(0.0, 1.0, widthProgress)
        );
        float halfW = cellWidth * 0.5 * (1.0 - widthVariation + widthVariation * widthRand);

        // Jitter: gentle positional offset
        float jitterOffset = (h21(vec2(idx * 3.7, float(laneIdx) * 2.3 + widthEpoch)) - 0.5) * jitter * cellWidth;
        float jitteredCenter = cellCenter + jitterOffset;

        // Pixel half-width for analytical AA
        float pw = 0.5 * aspect / resolution.x;

        // Coverage: x-axis cell rect * y-axis lane bounds
        float covX = boxCov(jitteredCenter - halfW, jitteredCenter + halfW, scrolledX, pw);
        float covY = gapMask;
        float cov = covX * covY;

        if (cov > 0.001) {
            // LUT coordinate from cell hash
            float t = h21(vec2(idx + 0.1, float(laneIdx) + 0.2));

            // Is this a colored/reactive cell or grayscale filler?
            float colorGate = h21(vec2(idx * 1.7, float(laneIdx) * 3.1));

            if (colorGate < colorMix) {
                // Colored cell: LUT color + FFT brightness
                vec3 cellColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
                float freq = baseFreq * pow(maxFreq / baseFreq, t);
                float bin = freq / (sampleRate * 0.5);
                float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
                mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
                float brightness = baseBright + mag;
                color += cellColor * cov * brightness;
            } else {
                // Grayscale filler: dim, no FFT
                float gray = 0.03 + 0.02 * h11(idx * 13.7 + float(laneIdx));
                color += vec3(gray) * cov;
            }
            totalCov += cov;
        }
    }

    finalColor = vec4(color, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| lanes | int | 4-60 | 20 | No | "Lanes" |
| cellWidth | float | 0.01-0.3 | 0.08 | Yes | "Cell Width" |
| spacing | float | 1.5-6.0 | 3.0 | Yes | "Spacing" |
| gapSize | float | 0.02-0.3 | 0.08 | Yes | "Gap Size" |
| scrollSpeed | float | 0.0-3.0 | 0.8 | Yes | "Scroll Speed" |
| widthVariation | float | 0.0-1.0 | 0.6 | Yes | "Width Variation" |
| colorMix | float | 0.0-1.0 | 0.5 | Yes | "Color Mix" |
| jitter | float | 0.0-1.0 | 0.3 | Yes | "Jitter" |
| changeRate | float | 0.05-0.5 | 0.15 | Yes | "Change Rate" |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | "Base Freq (Hz)" |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | "Max Freq (Hz)" |
| gain | float | 0.1-10.0 | 2.0 | Yes | "Gain" |
| curve | float | 0.1-3.0 | 1.0 | Yes | "Contrast" |
| baseBright | float | 0.0-1.0 | 0.05 | Yes | "Base Bright" |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | "Blend Intensity" |
| blendMode | EffectBlendMode | enum | EFFECT_BLEND_SCREEN | No | "Blend Mode" |
| gradient | ColorConfig | — | COLOR_MODE_GRADIENT | No | (color mode widget) |

### Constants

- Enum: `TRANSFORM_DATA_TRAFFIC_BLEND`
- Display name: `"Data Traffic Blend"`
- Macro: `REGISTER_GENERATOR`
- Category: `"GEN"` (section 10, auto-set by macro)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create effect header

**Files**: `src/effects/data_traffic.h` (create)
**Creates**: `DataTrafficConfig`, `DataTrafficEffect`, `DATA_TRAFFIC_CONFIG_FIELDS` macro, function declarations

**Do**: Create header following `scan_bars.h` pattern. Include `"render/color_config.h"` and `"render/blend_mode.h"` for `ColorConfig` and `EffectBlendMode`. Config and Effect structs defined in the Design section above. Forward-declare `ColorLUT`. Declare Init (takes `Effect*` and `const Config*`), Setup (takes `Effect*`, `const Config*`, `float deltaTime`, `Texture2D fftTexture`), Uninit, ConfigDefault, RegisterParams.

**Verify**: Header compiles when included.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect module implementation

**Files**: `src/effects/data_traffic.cpp` (create)
**Depends on**: Wave 1

**Do**: Follow `scan_bars.cpp` as pattern. Init loads shader, caches all uniform locations, creates `ColorLUT`. Setup accumulates `time += deltaTime`, calls `ColorLUTUpdate`, binds all uniforms including `time` (raw, not phase-accumulated like scan bars — all per-lane state is computed in shader). Uninit unloads shader and LUT. RegisterParams registers all float params from the Parameters table. Bottom: `SetupDataTraffic` bridge and `SetupDataTrafficBlend` bridge, then `REGISTER_GENERATOR` macro with `TRANSFORM_DATA_TRAFFIC_BLEND, DataTraffic, dataTraffic, "Data Traffic Blend", SetupDataTrafficBlend, SetupDataTraffic`.

**Verify**: `cmake.exe --build build` compiles (after all Wave 2 tasks complete).

#### Task 2.2: Fragment shader

**Files**: `shaders/data_traffic.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the Algorithm section above. Uniforms match the config fields plus `resolution`, `time`, `gradientLUT`, `fftTexture`, `sampleRate`. Use `fragTexCoord` (not `texture0` — this is a generator, not a post-process).

**Verify**: Shader file exists (runtime-compiled, no build step).

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/data_traffic.h"`. Add `TRANSFORM_DATA_TRAFFIC_BLEND` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`. Add to `TransformOrderConfig::order` array. Add `DataTrafficConfig dataTraffic;` to `EffectConfig`.

**Verify**: Compiles.

#### Task 2.4: PostEffect integration

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/data_traffic.h"`. Add `DataTrafficEffect dataTraffic;` member to `PostEffect` struct.

**Verify**: Compiles.

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/data_traffic.cpp` to `EFFECTS_SOURCES`.

**Verify**: Compiles.

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_gen_texture.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/data_traffic.h"` at top. Add `static bool sectionDataTraffic = false;`. Create `DrawGeneratorsDataTraffic()` following `DrawGeneratorsScanBars()` pattern. UI sections: Audio (Base Freq, Max Freq, Gain, Contrast, Base Bright), then Geometry (Lanes as `ImGui::SliderInt`, Cell Width, Spacing, Gap Size), then Animation (Scroll Speed, Width Variation, Color Mix, Jitter, Change Rate), then Color mode widget, then Output (Blend Intensity, Blend Mode combo). Call it in `DrawGeneratorsTextureCategory()` with `ImGui::Spacing()` before it.

**Verify**: Compiles.

#### Task 2.7: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/data_traffic.h"`. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DataTrafficConfig, DATA_TRAFFIC_CONFIG_FIELDS)`. Add `X(dataTraffic) \` to `EFFECT_CONFIG_FIELDS(X)` table.

**Verify**: Compiles.

---

## Implementation Notes

### Scroll Angle

Added `scrollAngle` config field (radians, ±PI). Shader rotates UV around screen center before lane/cell computation, allowing vertical or diagonal lane layouts. Uses `ModulatableSliderAngleDeg` in the Geometry UI section.

### Sparks

Added `sparkIntensity` config field (0.0–2.0). The cell neighbor loop was widened from `[-1, +1]` to `[-2, +2]` so adjacent cell pairs can be checked for proximity. After the main cell loop, a second pass detects gaps smaller than `slotWidth * 0.7` between adjacent cells and renders a flickering spark in the gap. Spark color blends from neighboring cells (or defaults to blue for grayscale pairs). Overlapping cells get 2x brightness. Flicker at 12fps via `h31`. Added `h31` hash function to the shader.

### Scroll Speed (non-modulatable)

`scrollSpeed` is passed directly to the shader and multiplied into `time * laneSpeed`. CPU phase accumulation was attempted (`scrollPhase += scrollSpeed * dt`) but reverted — the epoch speed variation uses wall-clock `time` to compute `epochSpeedMul`, so `frozen_scrollPhase * changing_epochSpeed` still caused drift at speed=0. Tying epochs to `scrollPhase` instead of `time` fixed the drift but made epoch transitions sluggish at low speeds, losing the effect's visual character. Kept the simple `time * scrollSpeed` approach; scroll speed is a plain `ImGui::SliderFloat` (not modulatable) since modulating it would cause the same jump artifacts.

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in generator texture UI section
- [ ] Enabling shows scrolling lanes with colored cells
- [ ] FFT reactivity: cells brighten with audio
- [ ] Lanes scroll at different speeds and directions
- [ ] Cell widths vary and re-randomize over time
- [ ] `colorMix` slider controls ratio of colored vs grayscale cells
- [ ] Gradient LUT colors cells correctly
- [ ] Sparks flash between adjacent cells when they drift close
- [ ] Scroll angle rotates the entire lane grid
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
