# Laser Dance

Volumetric raymarched generator: glowing laser-like ribbons weaving through 3D space. Two cosine fields at different frequencies create sharp creases via component-wise minimum — the viewer floats inside a slowly morphing light structure. FFT drives overall brightness so the lasers breathe with the music. Gradient LUT colors by ray depth.

**Research**: `docs/research/laser_dance.md`

## Design

### Types

**`LaserDanceConfig`** (`src/effects/laser_dance.h`):

```cpp
struct LaserDanceConfig {
  bool enabled = false;

  // Geometry
  float speed = 1.0f;        // Animation rate (0.1-5.0)
  float freqRatio = 0.6f;    // Cosine field ratio (0.3-1.5)
  float cameraOffset = 0.8f; // Camera position offset (0.2-2.0)
  float brightness = 1.0f;   // Output brightness (0.5-3.0)

  // Audio
  float baseFreq = 55.0f;    // Lowest FFT frequency (27.5-440.0)
  float maxFreq = 14000.0f;  // Highest FFT frequency (1000-16000)
  float gain = 2.0f;         // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;        // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f;  // Min brightness when silent (0.0-1.0)

  // Color
  float colorSpeed = 0.5f;   // Gradient cycle speed (0.0-3.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

**`LaserDanceEffect`** (`src/effects/laser_dance.h`):

```cpp
typedef struct LaserDanceEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int freqRatioLoc;
  int cameraOffsetLoc;
  int brightnessLoc;
  int colorPhaseLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  float time;
  float colorPhase;
  ColorLUT *gradientLUT;
} LaserDanceEffect;
```

### Algorithm

Single-pass volumetric raymarcher. 100 fixed steps, no ping-pong, no floor reflection.

**Ray setup** — centered coords, aspect-corrected:

```glsl
vec2 uv = (fragTexCoord - 0.5) * 2.0;
uv.x *= resolution.x / resolution.y;
vec3 rayDir = normalize(vec3(uv, -1.0));
```

**Raymarch loop:**

```glsl
vec3 color = vec3(0.0);
float z = 0.0;

for (int i = 0; i < STEPS; i++) {
    // Sample point along ray
    vec3 p = z * rayDir + vec3(cameraOffset);

    // Laser distance field (two cosine fields + crease)
    vec3 q = cos(p + time) + cos(p / freqRatio).yzx;
    float dist = length(min(q, q.zxy));

    // Step forward
    z += 0.3 * (0.01 + dist);

    // Accumulate color from gradient LUT by depth
    vec3 sc = textureLod(gradientLUT, vec2(z / MAX_DEPTH + colorPhase, 0.5), 0.0).rgb;
    color += sc / max(dist, 0.001) / max(z, 0.001);
}
```

Core distance field anatomy:

- `cos(p + time)` — primary wave field, animated
- `cos(p / freqRatio).yzx` — secondary field at lower frequency, swizzled for asymmetry
- `min(q, q.zxy)` — component-wise minimum creates sharp crease lines (the "lasers")
- `length(...)` — converts to scalar distance
- Step size `0.3 * (0.01 + dist)` — original floor-reflection terms (`0.1*d`, `/(1+d+d+d*d)`) removed

**FFT brightness** — average energy across `baseFreq → maxFreq` in log space:

```glsl
const int BAND_SAMPLES = 8;
float energy = 0.0;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float t = (float(s) + 0.5) / float(BAND_SAMPLES);
    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
}
float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float fftBright = baseBright + mag;
```

**Tonemapping:**

```glsl
color *= fftBright;
color = tanh(color / (700.0 / brightness));
finalColor = vec4(color, 1.0);
```

**Shader constants:**

- `MAX_DEPTH = 12.0` — depth normalization for gradient sampling. With 100 steps at `0.3 * (0.01 + dist)`, rays through moderate-density regions (dist ~0.3-0.5) reach z ≈ 10-15. 12.0 maps the visually interesting near-field to the full gradient range; distant accumulation clamps to the end of the LUT.
- `STEPS = 100` — raymarch iterations (fixed, not a parameter)
- `BAND_SAMPLES = 8` — FFT sub-samples for broadband average

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| speed | float | 0.1-5.0 | 1.0 | yes | Speed |
| freqRatio | float | 0.3-1.5 | 0.6 | yes | Freq Ratio |
| cameraOffset | float | 0.2-2.0 | 0.8 | yes | Camera Offset |
| brightness | float | 0.5-3.0 | 1.0 | yes | Brightness |
| baseFreq | float | 27.5-440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |
| colorSpeed | float | 0.0-3.0 | 0.5 | yes | Color Speed |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_LASER_DANCE_BLEND`
- Display name: `"Laser Dance"`
- Category: Filament (section 11)
- Macro: `REGISTER_GENERATOR`
- Badge: `"GEN"` (auto-set by macro)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by macro)

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Header

**Files**: `src/effects/laser_dance.h` (create)
**Creates**: `LaserDanceConfig`, `LaserDanceEffect`, function declarations

**Do**:
- Create header with include guard `LASER_DANCE_H`
- Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`
- Forward-declare `typedef struct ColorLUT ColorLUT;`
- Define `LaserDanceConfig` struct exactly as shown in Design section
- Define `LASER_DANCE_CONFIG_FIELDS` macro listing all serializable fields: `enabled, speed, freqRatio, cameraOffset, brightness, baseFreq, maxFreq, gain, curve, baseBright, colorSpeed, gradient, blendMode, blendIntensity`
- Define `LaserDanceEffect` typedef struct exactly as shown in Design section
- Declare functions:
  - `bool LaserDanceEffectInit(LaserDanceEffect *e, const LaserDanceConfig *cfg);`
  - `void LaserDanceEffectSetup(LaserDanceEffect *e, const LaserDanceConfig *cfg, float deltaTime, Texture2D fftTexture);`
  - `void LaserDanceEffectUninit(LaserDanceEffect *e);`
  - `LaserDanceConfig LaserDanceConfigDefault(void);`
  - `void LaserDanceRegisterParams(LaserDanceConfig *cfg);`
- Follow `constellation.h` for layout and style

**Verify**: Header parses (included by later tasks).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Implementation

**Files**: `src/effects/laser_dance.cpp` (create)
**Depends on**: Wave 1 complete

**Do**:
- Follow `constellation.cpp` for structure and includes
- Include: own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `imgui.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`
- `LaserDanceEffectInit(LaserDanceEffect* e, const LaserDanceConfig* cfg)`:
  - Load `shaders/laser_dance.fs`, cache all uniform locations
  - Init `gradientLUT` via `ColorLUTInit(&cfg->gradient)`
  - Init `time = 0.0f`, `colorPhase = 0.0f`
  - Cascade cleanup on failure: LUT fail → unload shader
- `LaserDanceEffectSetup(LaserDanceEffect* e, const LaserDanceConfig* cfg, float deltaTime, Texture2D fftTexture)`:
  - Accumulate `time += speed * deltaTime`, wrap with `fmodf(e->time, 6.2831853f)`
  - Accumulate `colorPhase += colorSpeed * deltaTime`, wrap with `fmodf(e->colorPhase, 1.0f)`
  - `ColorLUTUpdate(e->gradientLUT, &cfg->gradient)`
  - Set `resolution` as `float[2] = {GetScreenWidth(), GetScreenHeight()}`
  - Bind all scalar uniforms via `SetShaderValue`
  - Bind `sampleRate` as `(float)AUDIO_SAMPLE_RATE`
  - Bind texture uniforms: `SetShaderValueTexture` for gradientLUT (`ColorLUTGetTexture(e->gradientLUT)`) and fftTexture
- `LaserDanceEffectUninit`: `UnloadShader`, `ColorLUTUninit`
- `LaserDanceConfigDefault`: Return `LaserDanceConfig{}`
- `LaserDanceRegisterParams`: Register all modulatable params from Parameters table. Use `"laserDance."` prefix.
- Bridge functions (**non-static**):
  - `SetupLaserDance(PostEffect* pe)` — calls `LaserDanceEffectSetup(&pe->laserDance, &pe->effects.laserDance, pe->currentDeltaTime, pe->fftTexture)`
  - `SetupLaserDanceBlend(PostEffect* pe)` — calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.laserDance.blendIntensity, pe->effects.laserDance.blendMode)`
- `// === UI ===` section:
  - `static void DrawLaserDanceParams(EffectConfig*, const ModSources*, ImU32)`:
    - Geometry section (no separator — first section): Speed, Freq Ratio, Camera Offset, Brightness
    - `ImGui::SeparatorText("Audio")`: Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Base Bright (standard audio order/formatting per conventions)
  - `STANDARD_GENERATOR_OUTPUT(laserDance)`
  - Registration macro:
    ```
    REGISTER_GENERATOR(TRANSFORM_LASER_DANCE_BLEND, LaserDance, laserDance,
                       "Laser Dance", SetupLaserDanceBlend, SetupLaserDance,
                       11, DrawLaserDanceParams, DrawOutput_laserDance)
    ```

**Verify**: Compiles after Task 2.3 integration.

---

#### Task 2.2: Shader

**Files**: `shaders/laser_dance.fs` (create)
**Depends on**: Wave 1 complete (uniform names must match)

**Do**:
- Attribution block before `#version`:
  ```
  // Based on "Laser Dance" by @XorDev
  // https://www.shadertoy.com/view/tct3Rf
  // License: CC BY-NC-SA 3.0 Unported
  // Modified: Removed floor reflection, gradient LUT coloring, FFT audio reactivity
  ```
- `#version 330`, standard `in vec2 fragTexCoord`, `out vec4 finalColor`
- Uniforms: `resolution` (vec2), `time` (float), `freqRatio` (float), `cameraOffset` (float), `brightness` (float), `colorPhase` (float), `gradientLUT` (sampler2D), `fftTexture` (sampler2D), `sampleRate` (float), `baseFreq` (float), `maxFreq` (float), `gain` (float), `curve` (float), `baseBright` (float)
- Constants: `MAX_DEPTH = 12.0`, `STEPS = 100`, `BAND_SAMPLES = 8`
- Implement the Algorithm section from Design — transcribe the GLSL blocks directly:
  1. FFT brightness (broadband average)
  2. Ray setup (centered, aspect-corrected)
  3. Raymarch loop (distance field, step, accumulate with gradient LUT)
  4. Apply fftBright multiplier and `tanh` tonemapping
- Note: `cameraOffset` is float — use `vec3(cameraOffset)` to broadcast when adding to vec3
- The `.yzx` swizzle in `cos(p / freqRatio).yzx` is load-bearing — it breaks cubic symmetry. Do NOT change it.

**Verify**: Shader file has correct uniform names matching the `.cpp` location cache.

---

#### Task 2.3: Integration

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `effect_config.h`:
  - Add `#include "effects/laser_dance.h"` with other effect includes
  - Add `TRANSFORM_LASER_DANCE_BLEND` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
  - Add `TRANSFORM_LASER_DANCE_BLEND` at end of `TransformOrderConfig::order` array
  - Add `LaserDanceConfig laserDance;` to `EffectConfig` struct
- `post_effect.h`:
  - Add `#include "effects/laser_dance.h"` with other effect includes
  - Add `LaserDanceEffect laserDance;` to `PostEffect` struct
- `CMakeLists.txt`:
  - Add `src/effects/laser_dance.cpp` to `EFFECTS_SOURCES`
- `effect_serialization.cpp`:
  - Add `#include "effects/laser_dance.h"`
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LaserDanceConfig, LASER_DANCE_CONFIG_FIELDS)`
  - Add `X(laserDance) \` to `EFFECT_CONFIG_FIELDS(X)` macro
- Follow constellation entries in each file for placement

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Filament generator section
- [ ] Effect shows "GEN" badge
- [ ] Laser structure visible when enabled
- [ ] freqRatio slider changes beam density
- [ ] speed slider controls animation rate
- [ ] cameraOffset slider shifts viewpoint
- [ ] FFT brightness responds to audio
- [ ] Gradient LUT colors the beams by depth
- [ ] Preset save/load round-trips all settings
- [ ] Modulation routes to all registered parameters
