# Byzantine

Self-generating reaction-diffusion texture with infinite fractal zoom. Alternates diffusion and sharpening each frame to create labyrinthine patterns, with periodic zoom reseeds that recycle the field's own output as perturbation. Two-shader generator: a simulation pass (ping-pong) produces a raw [-1,1] field, and a display pass transforms it into colored output with counter-zoom smoothing, gradient LUT coloring, and FFT audio reactivity.

**Research**: `docs/research/byzantine-buffering.md`

## Design

### Types

**`ByzantineConfig`** (in `src/effects/byzantine.h`):

```cpp
struct ByzantineConfig {
  bool enabled = false;

  // Simulation
  float diffusionWeight = 0.368f; // Even-frame center weight (0.1-0.9)
  float sharpenWeight = 3.0f;     // Odd-frame center weight (1.5-5.0)
  float cycleLength = 360.0f;     // Frames between zoom reseeds (60-600)
  float zoomAmount = 2.0f;        // Zoom factor per reseed (1.2-4.0)
  float centerX = 0.5f;           // Zoom focus X in UV (0.0-1.0)
  float centerY = 0.5f;           // Zoom focus Y in UV (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;    // Low FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f;  // High FFT frequency Hz (1000-16000)
  float gain = 1.0f;         // FFT energy multiplier (0.1-10.0)
  float curve = 1.0f;        // FFT contrast curve exponent (0.1-3.0)
  float baseBright = 0.3f;   // Minimum brightness floor (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};
```

**`ByzantineEffect`** (in `src/effects/byzantine.h`):

```cpp
typedef struct ByzantineEffect {
  Shader shader;         // Simulation shader (ping-pong)
  Shader displayShader;  // Display pass (counter-zoom + LUT + FFT)
  RenderTexture2D pingPong[2];
  int readIdx;
  int frameCount;
  ColorLUT *gradientLUT;
  Texture2D currentFFTTexture;

  // Sim shader locations
  int simResolutionLoc;
  int simFrameCountLoc;
  int simCycleLengthLoc;
  int simDiffusionWeightLoc;
  int simSharpenWeightLoc;
  int simZoomAmountLoc;
  int simCenterLoc;

  // Display shader locations
  int dispResolutionLoc;
  int dispFrameCountLoc;
  int dispCycleLengthLoc;
  int dispZoomAmountLoc;
  int dispCenterLoc;
  int dispGradientLUTLoc;
  int dispFftTextureLoc;
  int dispSampleRateLoc;
  int dispBaseFreqLoc;
  int dispMaxFreqLoc;
  int dispGainLoc;
  int dispCurveLoc;
  int dispBaseBrightLoc;
} ByzantineEffect;
```

**Config fields macro**:
```cpp
#define BYZANTINE_CONFIG_FIELDS                                                \
  enabled, diffusionWeight, sharpenWeight, cycleLength, zoomAmount, centerX,  \
      centerY, baseFreq, maxFreq, gain, curve, baseBright, gradient,          \
      blendMode, blendIntensity
```

### Algorithm

#### Simulation Shader (`shaders/byzantine_sim.fs`)

```glsl
// Based on "Byzantine Buffering" by paniq
// https://www.shadertoy.com/view/7tBGz1
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced yin-yang SDF seed with concentric sine rings,
//           parameterized kernel weights and cycle length for AudioJones

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // Previous ping-pong buffer
uniform vec2 resolution;
uniform int frameCount;
uniform int cycleLength;
uniform float diffusionWeight;
uniform float sharpenWeight;
uniform float zoomAmount;
uniform vec2 center;

void main() {
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    if (frameCount == 0) {
        // Seed: concentric sine rings centered at zoom focus
        vec2 centered = (uv - center) * vec2(resolution.x / resolution.y, 1.0);
        float d = length(centered);
        float w = sin(d * 80.0);
        finalColor = vec4(w);
    } else if (frameCount % cycleLength == 0) {
        // Zoom reseed: resample at reduced scale toward center
        // Upscaling interpolation artifacts feed fresh perturbation
        vec2 resampled = (uv - center) / zoomAmount + center;
        finalColor = texture(texture0, resampled);
    } else {
        // 5-tap von Neumann kernel with alternating weights
        // Even frames: diffusion (w < 1, k > 0 => blur)
        // Odd frames: sharpening (w > 1, k < 0 => edge enhance)
        vec4 v0 = texture(texture0, uv + vec2(-texel.x, 0.0));
        vec4 v1 = texture(texture0, uv + vec2( texel.x, 0.0));
        vec4 v2 = texture(texture0, uv + vec2(0.0, -texel.y));
        vec4 v3 = texture(texture0, uv + vec2(0.0,  texel.y));
        vec4 v4 = texture(texture0, uv);

        float w = ((frameCount % 2) == 0) ? diffusionWeight : sharpenWeight;
        float k = (1.0 - w) / 4.0;

        finalColor = clamp(w * v4 + k * (v0 + v1 + v2 + v3), -1.0, 1.0);
    }
}
```

#### Display Shader (`shaders/byzantine_display.fs`)

```glsl
// Based on "Byzantine Buffering" by paniq
// https://www.shadertoy.com/view/7tBGz1
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced RGB offset-zoom colorization with gradient LUT,
//           added FFT audio reactivity, removed barrel distortion

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // Sim buffer (current read)
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform vec2 resolution;
uniform int frameCount;
uniform int cycleLength;
uniform float zoomAmount;
uniform vec2 center;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    // Counter-zoom: smooth compensation for discrete reseed
    // scale goes from 1.0 (just after reseed) to 1/zoomAmount (just before next)
    float m = float(frameCount % cycleLength) / float(cycleLength);
    float scale = pow(zoomAmount, -m);

    // Apply zoom with aspect-correct isotropic scaling
    vec2 centered = fragTexCoord - center;
    centered.x *= resolution.x / resolution.y;
    centered *= scale;
    centered.x /= resolution.x / resolution.y;
    vec2 uv = centered + center;

    // Sample simulation field [-1, 1] -> t [0, 1]
    float field = texture(texture0, uv).r;
    float t = field * 0.5 + 0.5;

    // Gradient LUT color
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

    // FFT audio reactivity: map t to frequency, look up energy
    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) {
        energy = texture(fftTexture, vec2(bin, 0.5)).r;
        energy = pow(clamp(energy * gain, 0.0, 1.0), curve);
    }
    float brightness = baseBright + energy;

    finalColor = vec4(color * brightness, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| diffusionWeight | float | 0.1-0.9 | 0.368 | Yes | "Diffusion" |
| sharpenWeight | float | 1.5-5.0 | 3.0 | Yes | "Sharpening" |
| cycleLength | float | 60-600 | 360 | Yes | "Cycle Length" |
| zoomAmount | float | 1.2-4.0 | 2.0 | Yes | "Zoom Amount" |
| centerX | float | 0.0-1.0 | 0.5 | Yes | "Center X" |
| centerY | float | 0.0-1.0 | 0.5 | Yes | "Center Y" |
| baseFreq | float | 27.5-440 | 55.0 | Yes | "Base Freq (Hz)" |
| maxFreq | float | 1000-16000 | 14000 | Yes | "Max Freq (Hz)" |
| gain | float | 0.1-10.0 | 1.0 | Yes | "Gain" |
| curve | float | 0.1-3.0 | 1.0 | Yes | "Contrast" |
| baseBright | float | 0.0-1.0 | 0.3 | Yes | "Base Bright" |
| gradient | ColorConfig | — | COLOR_MODE_GRADIENT | No | — |
| blendMode | EffectBlendMode | — | EFFECT_BLEND_SCREEN | No | "Blend Mode" |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | "Blend Intensity" |

### Constants

- Enum: `TRANSFORM_BYZANTINE_BLEND`
- Display name: `"Byzantine"`
- Category badge: `"GEN"`, section 12 (Texture)
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE` (auto-set by `REGISTER_GENERATOR_FULL`)
- Macro: `REGISTER_GENERATOR_FULL`

### Rendering Flow

1. **Pipeline** calls `scratchSetup` → `SetupByzantine` binds all uniforms for both shaders
2. **Pipeline** calls `render` → `RenderByzantine`:
   - **Sim pass**: ping-pong with `shader` (sim). Reads `pingPong[readIdx]`, writes to `pingPong[writeIdx]`. Flips `readIdx`, increments `frameCount`.
   - **Display pass**: renders through `displayShader` into `pe->generatorScratch`. Reads `pingPong[readIdx]` (sim output), binds `gradientLUT` and `fftTexture`.
3. **Pipeline** runs blend pass → `SetupByzantineBlend` feeds `pe->generatorScratch.texture` to `BlendCompositorApply`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create ByzantineConfig and ByzantineEffect header

**Files**: `src/effects/byzantine.h` (create)
**Creates**: Config struct, Effect struct, function declarations needed by all Wave 2 tasks

**Do**: Create the header following `muons.h` as pattern. Define `ByzantineConfig` and `ByzantineEffect` structs exactly per the Types section above. Name the simulation shader field `shader` (not `simShader`) — `REGISTER_GENERATOR_FULL` generates `GetScratchShader` returning `&pe->field.shader`. Define `BYZANTINE_CONFIG_FIELDS` macro per the Design section. Declare lifecycle functions:
- `ByzantineEffectInit(ByzantineEffect *e, const ByzantineConfig *cfg, int width, int height)` → bool
- `ByzantineEffectSetup(ByzantineEffect *e, const ByzantineConfig *cfg, float deltaTime, Texture2D fftTexture)` → void
- `ByzantineEffectRender(ByzantineEffect *e, PostEffect *pe)` → void
- `ByzantineEffectResize(ByzantineEffect *e, int width, int height)` → void
- `ByzantineEffectUninit(ByzantineEffect *e)` → void
- `ByzantineConfigDefault(void)` → ByzantineConfig
- `ByzantineRegisterParams(ByzantineConfig *cfg)` → void

Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `typedef struct ColorLUT ColorLUT;` and `typedef struct PostEffect PostEffect;`.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Implement Byzantine effect module

**Files**: `src/effects/byzantine.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement following `muons.cpp` as the primary pattern.

Includes (in order): `"byzantine.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"render/render_utils.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<math.h>`, `<stddef.h>`.

Static helpers:
- `InitPingPong(ByzantineEffect *e, int w, int h)`: two HDR RTs via `RenderUtilsInitTextureHDR`, label `"BYZANTINE"`
- `UnloadPingPong(ByzantineEffect *e)`: unload both
- `CacheLocations(ByzantineEffect *e)`: cache all uniform locations for both `e->shader` (sim) and `e->displayShader`. Sim locations: `resolution`, `frameCount`, `cycleLength`, `diffusionWeight`, `sharpenWeight`, `zoomAmount`, `center`. Display locations: `resolution`, `frameCount`, `cycleLength`, `zoomAmount`, `center`, `gradientLUT`, `fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`.

`ByzantineEffectInit`: Load `shaders/byzantine_sim.fs` into `e->shader`. If id==0 return false. Load `shaders/byzantine_display.fs` into `e->displayShader`. If id==0, unload sim shader, return false. Call `CacheLocations`. Init `gradientLUT` via `ColorLUTInit(&cfg->gradient)`. If null, unload both shaders, return false. Call `InitPingPong`, clear both via `RenderUtilsClearTexture`. Set `readIdx = 0`, `frameCount = 0`.

`ByzantineEffectSetup`: Call `ColorLUTUpdate`. Set resolution as vec2. Cast `cycleLength` to int before sending as `SHADER_UNIFORM_INT` to both shaders. Send `frameCount` as int to both shaders. Bind sim uniforms: `diffusionWeight`, `sharpenWeight`, `zoomAmount` (float), `center` as `vec2{cfg->centerX, cfg->centerY}`. Bind display uniforms: `zoomAmount`, `center`, `sampleRate` (from `AUDIO_SAMPLE_RATE`), `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`. Store `e->currentFFTTexture = fftTexture`.

`ByzantineEffectRender(ByzantineEffect *e, PostEffect *pe)`:
- **Sim pass**: `writeIdx = 1 - e->readIdx`. `BeginTextureMode(e->pingPong[writeIdx])`, `BeginShaderMode(e->shader)`, `RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, pe->screenWidth, pe->screenHeight)`. End shader/texture mode. `e->readIdx = writeIdx`. `e->frameCount++`.
- **Display pass**: `BeginTextureMode(pe->generatorScratch)`, `BeginShaderMode(e->displayShader)`. Bind textures AFTER Begin calls: `SetShaderValueTexture` for `gradientLUT` (via `ColorLUTGetTexture`) and `fftTexture` (from `e->currentFFTTexture`). `RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, pe->screenWidth, pe->screenHeight)`. End shader/texture mode.

`ByzantineEffectResize`: `UnloadPingPong`, `InitPingPong` at new size, `readIdx = 0`.

`ByzantineEffectUninit`: `UnloadShader` both, `ColorLUTUninit`, `UnloadPingPong`.

`ByzantineConfigDefault`: return `ByzantineConfig{}`.

`ByzantineRegisterParams`: Register all modulatable params per the Parameters table. Use dot-separated IDs: `"byzantine.diffusionWeight"`, etc.

**UI section** (`// === UI ===`):
- `static void DrawByzantineParams(EffectConfig *e, const ModSources *ms, ImU32 glow)`:
  - `(void)glow;`
  - Simulation section: `ImGui::SeparatorText("Simulation")`. Sliders for diffusionWeight (`"%.3f"`), sharpenWeight (`"%.1f"`), cycleLength (`"%.0f"`), zoomAmount (`"%.1f"`), centerX (`"%.2f"`), centerY (`"%.2f"`). All `ModulatableSlider`.
  - Audio section: standard order per conventions — Base Freq (`"%.1f"`), Max Freq (`"%.0f"`), Gain (`"%.1f"`), Contrast (`"%.2f"`), Base Bright (`"%.2f"`).

**Bridge functions** (non-static — required by `REGISTER_GENERATOR_FULL` macro forward declarations):
- `void SetupByzantine(PostEffect *pe)`: calls `ByzantineEffectSetup(&pe->byzantine, &pe->effects.byzantine, pe->currentDeltaTime, pe->fftTexture)`
- `void SetupByzantineBlend(PostEffect *pe)`: calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.byzantine.blendIntensity, pe->effects.byzantine.blendMode)`
- `void RenderByzantine(PostEffect *pe)`: calls `ByzantineEffectRender(&pe->byzantine, pe)`

**Registration** (bottom of file):
```
// clang-format off
STANDARD_GENERATOR_OUTPUT(byzantine)
REGISTER_GENERATOR_FULL(TRANSFORM_BYZANTINE_BLEND, Byzantine, byzantine,
                        "Byzantine", SetupByzantineBlend, SetupByzantine,
                        RenderByzantine, 12, DrawByzantineParams,
                        DrawOutput_byzantine)
// clang-format on
```

**Verify**: Compiles (after all Wave 2 tasks).

---

#### Task 2.2: Create simulation shader

**Files**: `shaders/byzantine_sim.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the simulation shader exactly as specified in the Algorithm section above. The shader file starts with the attribution comment block (before `#version`), then the full GLSL.

Key points:
- `texture0` is the previous ping-pong buffer (passed implicitly via `RenderUtilsDrawFullscreenQuad`)
- Frame 0: concentric sine rings seed `sin(d * 80.0)` — 80.0 matches the original
- Zoom reseed: `(uv - center) / zoomAmount + center`
- Kernel: `vec4` operations (all channels evolve identically)
- Clamp to `[-1.0, 1.0]`

**Verify**: File exists and is valid GLSL.

---

#### Task 2.3: Create display shader

**Files**: `shaders/byzantine_display.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the display shader exactly as specified in the Algorithm section above. Starts with attribution comment block.

Key points:
- Counter-zoom: `scale = pow(zoomAmount, -m)` with isotropic aspect correction
- `t = field * 0.5 + 0.5` maps [-1,1] field to [0,1] for LUT
- Gradient LUT: `texture(gradientLUT, vec2(t, 0.5)).rgb`
- FFT: frequency spread from `baseFreq` to `maxFreq` mapped by `t`
- Output: `vec4(color * brightness, 1.0)`

**Verify**: File exists and is valid GLSL.

---

#### Task 2.4: Register in effect config

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Three changes following existing patterns:

1. Add `#include "effects/byzantine.h"` in the includes section (alphabetical order, near other effect includes)

2. Add `TRANSFORM_BYZANTINE_BLEND,` to the `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`. Place after existing Texture-category generators (after `TRANSFORM_SLIT_SCAN_CORRIDOR` or nearby generators).

3. Add config member to `EffectConfig` struct:
   ```cpp
   // Byzantine (reaction-diffusion labyrinthine texture with fractal zoom)
   ByzantineConfig byzantine;
   ```
   Place near other generator configs (after `SlitScanCorridorConfig` or at end before `TransformOrderConfig`).

Note: `TransformOrderConfig` constructor auto-populates by enum index — no manual order array edit needed.

**Verify**: Compiles.

---

#### Task 2.5: Register in PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Two changes:

1. Add `#include "effects/byzantine.h"` in the includes section (alphabetical order)

2. Add effect member to the `PostEffect` struct alongside other generator effect members:
   ```cpp
   ByzantineEffect byzantine;
   ```

**Verify**: Compiles.

---

#### Task 2.6: Add to build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/byzantine.cpp` to the `EFFECTS_SOURCES` list (alphabetical order).

**Verify**: Compiles.

---

#### Task 2.7: Add preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Three changes:

1. Add `#include "effects/byzantine.h"` in the includes section (alphabetical order)

2. Add JSON macro with other per-config macros:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ByzantineConfig,
                                                   BYZANTINE_CONFIG_FIELDS)
   ```

3. Add `X(byzantine)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table (append to end, before the closing line).

**Verify**: Compiles.

---

## Implementation Notes

Changes from the original plan made during implementation:

### Removed: Diffusion/Sharpening params, Audio/FFT
- `diffusionWeight` and `sharpenWeight` removed from config — hardcoded in shader as `DIFFUSION_W = 0.367879` and `SHARPEN_W = 3.0`. Exposing them was unintuitive and small changes broke the sim.
- All audio/FFT fields removed (`baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`, `fftTexture`, `sampleRate`). Display shader has no FFT reactivity.
- `ByzantineEffectSetup` signature simplified: no `fftTexture` param, no `screenWidth`/`screenHeight`.

### Added: Rotation and twist as infinite spiral zoom
- `rotationSpeed` and `twistSpeed` (rad/s) replace diffusion/sharpening as user controls.
- CPU accumulation: `perStepRotation = rotationSpeed * deltaTime`, sent to sim shader as `rotation` uniform.
- Per-cycle values (`perStepRotation * cycleLength`) sent to display shader as `cycleRotation`/`cycleTwist` for counter-rotation.
- Sim shader: `process()` rotates kernel sampling by `rotation + twist * dist`. Reseed rotates by full cycle amount alongside zoom. Both integrated into the infinite zoom illusion.
- Display shader: counter-rotates by `(cycleRotation + cycleTwist * dist) * cycleProgress`, interpolated alongside counter-zoom.
- UI: `ModulatableSliderSpeedDeg` with `ROTATION_SPEED_MAX` bounds.

### Display shader divergences from plan
- Kept barrel distortion and chromatic zoom offset from reference (plan said "removed barrel distortion").
- Counter-zoom uses `exp(mix(log(1.0), log(1.0/zoomAmount), m))` form, not `pow(zoomAmount, -m)`.
- Coloring: `pow(vec3(r,g,b), vec3(0.5))` base with gradient LUT tint via luminance multiply (`tint * bw`), not direct LUT sampling (which causes flicker from even/odd kernel alternation).
- `cycleProgress` computed in C++ render function from `(frameCount-1) % cachedCycleLen`, not from `frameCount` uniform in shader.

### Sim shader divergences from plan
- Seed pattern uses `sin(d * 40.0)` (not 80.0), with `(uv - center) * 2.0` scaling.
- Reseed injects 5% seed pattern (`sin(length(p) * 40.0) * 0.05`) to prevent convergence to uniform dead zones.
- Alpha blending disabled during sim pass via `rlDisableColorBlend()`/`rlEnableColorBlend()` — raylib's default blend corrupts signed [-1,1] sim values. All sim outputs use `alpha = 1.0`.
- Clamp applied in main(), not inside process(): `clamp(process(uv).rgb, -1.0, 1.0)`.

### Config/struct changes
- `cachedCycleLen` field added to `ByzantineEffect` — caches `(int)cycleLength` so render can use it without config pointer.
- `currentFFTTexture` removed from effect struct.
- Zoom range: 1.0–4.0 (not 1.2–4.0).

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Effect appears in GEN > Texture category (section 12) with "GEN" badge
- [ ] Enabling the effect shows labyrinthine patterns evolving
- [ ] Zoom reseed occurs every `cycleLength` frames (default ~6 seconds at 60fps)
- [ ] Counter-zoom produces smooth continuous zoom appearance
- [ ] Diffusion/Sharpening sliders change pattern character
- [ ] Audio reactivity modulates brightness with music
- [ ] Gradient LUT changes color palette
- [ ] Blend mode and intensity work correctly
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
