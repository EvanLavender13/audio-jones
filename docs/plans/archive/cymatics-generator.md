# Cymatics Generator Reclassification

Refactor Cymatics from a compute-shader simulation (`src/simulation/`) to a fragment-shader generator (`src/effects/`) using the `REGISTER_GENERATOR_FULL` pattern. Visual output unchanged: vibrating-plate interference patterns responding to audio. Motivation: Cymatics is a pure per-pixel function with no agents, no scatter writes, no shared memory. It belongs in the generator pipeline.

**Research**: `docs/research/cymatics-generator.md`

## Design

### Types

**CymaticsConfig** (unchanged fields, removed `diffusionScale` and `debugOverlay`):

```c
struct CymaticsConfig {
  bool enabled = false;
  float waveScale = 10.0f;       // Pattern scale (1-50)
  float falloff = 1.0f;          // Distance attenuation (0-5)
  float visualGain = 2.0f;       // Output intensity (0.5-5)
  int contourCount = 0;          // Banding (0=smooth, 1-10)
  float decayHalfLife = 0.5f;    // Trail persistence seconds (0.1-5)
  int sourceCount = 5;           // Number of sources (1-8)
  float baseRadius = 0.4f;       // Source orbit radius (0.0-0.5)
  DualLissajousConfig lissajous; // Source motion pattern
  bool boundaries = false;       // Enable edge reflections
  float reflectionGain = 1.0f;   // Mirror source attenuation (0.0-1.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;   // Blend compositor strength (0.0-5.0)
  ColorConfig color;
};
```

Fields removed vs current: `diffusionScale` (diffusion dropped), `debugOverlay` (debug overlay dropped), `boostIntensity` (renamed to `blendIntensity` to match generator convention).

Config fields macro:
```c
#define CYMATICS_CONFIG_FIELDS                                                 \
  enabled, waveScale, falloff, visualGain, contourCount, decayHalfLife,        \
      sourceCount, baseRadius, lissajous, boundaries, reflectionGain,          \
      blendMode, blendIntensity, color
```

**CymaticsEffect** (replaces heap-allocated `Cymatics` struct):

```c
typedef struct CymaticsEffect {
  Shader shader;
  ColorLUT *colorLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  float time;          // Not used for animation — reserved for future

  // Uniform locations
  int resolutionLoc;
  int aspectLoc;
  int waveScaleLoc;
  int falloffLoc;
  int visualGainLoc;
  int contourCountLoc;
  int bufferSizeLoc;
  int writeIndexLoc;
  int valueLoc;
  int sourcesLoc;
  int sourceCountLoc;
  int boundariesLoc;
  int reflectionGainLoc;
  int previousFrameLoc;
  int decayFactorLoc;
  int colorLUTLoc;
} CymaticsEffect;
```

### Algorithm

Single fragment shader replacing the compute shader. The interference math is identical.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // waveform ring buffer
uniform sampler2D previousFrame;  // ping-pong read buffer (trail persistence)
uniform sampler2D colorLUT;       // color mapping
uniform vec2 resolution;
uniform float aspect;
uniform float waveScale;
uniform float falloff;
uniform float visualGain;
uniform int contourCount;
uniform int bufferSize;
uniform int writeIndex;
uniform float value;              // brightness from color config HSV
uniform vec2 sources[8];          // animated source positions (CPU-computed)
uniform int sourceCount;
uniform int boundaries;           // bool as int
uniform float reflectionGain;
uniform float decayFactor;        // exp(-0.693147 * dt / halfLife)

// Fetch waveform with linear interpolation at ring buffer offset
float fetchWaveform(float delay) {
    delay = min(delay, float(bufferSize) * 0.9);
    float idx = mod(float(writeIndex) - delay + float(bufferSize), float(bufferSize));
    float u = clamp(idx / float(bufferSize), 0.001, 0.999);
    float val = texture(texture0, vec2(u, 0.5)).r;
    return val * 2.0 - 1.0;
}

void main() {
    // Map to centered, aspect-corrected coordinates [-1,1] (y) [-aspect,aspect] (x)
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= aspect;

    // Sum interference from all sources with Gaussian falloff
    float totalWave = 0.0;
    for (int i = 0; i < sourceCount; i++) {
        vec2 sourcePos = sources[i];
        sourcePos.x *= aspect;

        float dist = length(uv - sourcePos);
        float delay = dist * waveScale;
        float attenuation = exp(-dist * dist * falloff);
        totalWave += fetchWaveform(delay) * attenuation;

        // Mirror sources for boundary reflections (4 per source)
        if (boundaries != 0) {
            vec2 mirrors[4];
            mirrors[0] = vec2(-2.0 * aspect - sourcePos.x, sourcePos.y);
            mirrors[1] = vec2( 2.0 * aspect - sourcePos.x, sourcePos.y);
            mirrors[2] = vec2(sourcePos.x, -2.0 - sourcePos.y);
            mirrors[3] = vec2(sourcePos.x,  2.0 - sourcePos.y);

            for (int m = 0; m < 4; m++) {
                float mDist = length(uv - mirrors[m]);
                float mDelay = mDist * waveScale;
                float mAtten = exp(-mDist * mDist * falloff) * reflectionGain;
                totalWave += fetchWaveform(mDelay) * mAtten;
            }
        }
    }

    // Optional contour banding
    float wave = totalWave;
    if (contourCount > 0) {
        wave = floor(totalWave * float(contourCount) + 0.5) / float(contourCount);
    }

    // tanh compression
    float intensity = tanh(wave * visualGain);

    // Color LUT mapping: [-1,1] → [0,1]
    float t = intensity * 0.5 + 0.5;
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
    float brightness = abs(intensity) * value;

    // New frame color
    vec4 newColor = vec4(color * brightness, brightness);

    // Trail persistence: decay previous frame, keep brighter of old/new
    vec4 existing = texture(previousFrame, fragTexCoord) * decayFactor;
    finalColor = max(existing, newColor);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveScale | float | 1-50 | 10.0 | yes | Wave Scale |
| falloff | float | 0-5 | 1.0 | yes | Falloff |
| visualGain | float | 0.5-5 | 2.0 | yes | Gain |
| contourCount | int | 0-10 | 0 | no | Contours |
| decayHalfLife | float | 0.1-5 | 0.5 | no | Decay |
| sourceCount | int | 1-8 | 5 | no | Source Count |
| baseRadius | float | 0.0-0.5 | 0.4 | yes | Base Radius |
| lissajous | DualLissajousConfig | — | defaults | partial | (widget) |
| boundaries | bool | — | false | no | Boundaries |
| reflectionGain | float | 0.0-1.0 | 1.0 | yes | Reflection Gain |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |
| blendMode | enum | — | SCREEN | no | Blend Mode |
| color | ColorConfig | — | defaults | no | (widget) |

### Constants

- Enum name: `TRANSFORM_CYMATICS` (keep existing — no `_BLEND` suffix needed since the enum value is already established and presets reference it)
- Display name: `"Cymatics"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR_FULL`)
- Section index: `13` (Field)

### Plumbing: waveformWriteIndex

Add `int waveformWriteIndex` field to `PostEffect` struct (next to `waveformTexture`). Store it in `RenderPipelineExecute` before simulation and transform passes execute. This makes it available to the cymatics setup function via `pe->waveformWriteIndex`.

---

## Tasks

### Wave 1: Header + Plumbing

#### Task 1.1: Create cymatics effect header

**Files**: `src/effects/cymatics.h` (create)
**Creates**: `CymaticsConfig`, `CymaticsEffect` types and function declarations

**Do**: Create the header following the Design > Types section above. Follow `muons.h` as pattern for structure. Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `"config/dual_lissajous_config.h"`, `<stdbool.h>`.

Declare these functions:
- `bool CymaticsEffectInit(CymaticsEffect *e, const CymaticsConfig *cfg, int width, int height);`
- `void CymaticsEffectSetup(CymaticsEffect *e, const CymaticsConfig *cfg, float deltaTime, Texture2D waveformTexture, int waveformWriteIndex);`
- `void CymaticsEffectRender(CymaticsEffect *e, const CymaticsConfig *cfg, float deltaTime, int screenWidth, int screenHeight);`
- `void CymaticsEffectResize(CymaticsEffect *e, int width, int height);`
- `void CymaticsEffectUninit(CymaticsEffect *e);`
- `CymaticsConfig CymaticsConfigDefault(void);`
- `void CymaticsRegisterParams(CymaticsConfig *cfg);`

Forward-declare `ColorLUT`.

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

#### Task 1.2: Add waveformWriteIndex to PostEffect

**Files**: `src/render/post_effect.h` (modify), `src/render/render_pipeline.cpp` (modify)

**Do**:
1. In `post_effect.h`: add `int waveformWriteIndex;` field after the `waveformTexture` field (line ~282).
2. In `render_pipeline.cpp` `RenderPipelineExecute()`: add `pe->waveformWriteIndex = waveformWriteIndex;` after the `UpdateWaveformTexture(pe, waveformHistory);` call (line ~250).

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create cymatics fragment shader

**Files**: `shaders/cymatics.fs` (create)

**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section verbatim. The full GLSL is provided in the Design section above.

**Verify**: File exists at `shaders/cymatics.fs`.

#### Task 2.2: Create cymatics effect source

**Files**: `src/effects/cymatics.cpp` (create)

**Depends on**: Wave 1 complete

**Do**: Create the full effect module following `muons.cpp` as the structural pattern. Include order per conventions: own header, then project headers (`audio/audio.h` NOT needed — no FFT here), automation, config, render, imgui/UI, system headers.

Key implementation details:

1. **`CymaticsEffectInit`**: Load `shaders/cymatics.fs`, cache all uniform locations matching the Design > Types `CymaticsEffect` fields. Init `ColorLUT` from `cfg->color`. Init ping-pong with `RenderUtilsInitTextureHDR`. Clear both ping-pong textures. Set `readIdx = 0`. Return false on shader or LUT failure with cleanup cascade.

2. **`CymaticsEffectSetup`**: Compute source positions via `DualLissajousUpdateCircular(&cfg->lissajous, deltaTime, cfg->baseRadius, 0.0f, 0.0f, count, sources)`. Compute `value` from `ColorConfig` mode (same logic as current `cymatics.cpp` lines 162-176). Compute `decayFactor = expf(-0.693147f * deltaTime / fmaxf(cfg->decayHalfLife, 0.001f))`. Compute `aspect = resolution[0] / resolution[1]`. Bind all uniforms via `SetShaderValue`. Store `waveformTexture` for use in render. Update `ColorLUT`.

3. **`CymaticsEffectRender`**: Follow `MuonsEffectRender` pattern exactly. `BeginTextureMode(pingPong[writeIdx])` → `BeginShaderMode(shader)` → bind `previousFrame` (pingPong[readIdx].texture), `colorLUT`, and `waveformTexture` via `SetShaderValueTexture` → `RenderUtilsDrawFullscreenQuad` → `EndShaderMode` → `EndTextureMode` → flip `readIdx`.

4. **`CymaticsEffectResize`**: Unload and reinit ping-pong textures, reset `readIdx = 0`.

5. **`CymaticsEffectUninit`**: Unload shader, ColorLUT, ping-pong textures.

6. **`CymaticsRegisterParams`**: Carry over all existing registrations from current `cymatics.cpp`. Add `"cymatics.blendIntensity"` (0.0-5.0). Remove `"cymatics.boostIntensity"` (renamed).

7. **Bridge functions** (non-static):
   - `SetupCymatics(PostEffect *pe)`: calls `CymaticsEffectSetup` passing `pe->cymatics`, `pe->effects.cymatics`, `pe->currentDeltaTime`, `pe->waveformTexture`, `pe->waveformWriteIndex`.
   - `SetupCymaticsBlend(PostEffect *pe)`: calls `BlendCompositorApply` with `pe->cymatics.pingPong[pe->cymatics.readIdx].texture`, `pe->effects.cymatics.blendIntensity`, `pe->effects.cymatics.blendMode`.
   - `RenderCymatics(PostEffect *pe)`: calls `CymaticsEffectRender` passing `pe->cymatics`, `pe->effects.cymatics`, `pe->currentDeltaTime`, `pe->screenWidth`, `pe->screenHeight`.

8. **UI section** (`// === UI ===`): Port `DrawCymaticsParams` from current `cymatics.cpp`. Remove `diffusionScale` slider, remove `debugOverlay` checkbox, rename "Boost" to "Blend Intensity" in the Output section. Keep all other sections (Wave, Boundaries, Sources, Trail, Output) and all modulatable sliders.

9. **Registration**:
   ```
   STANDARD_GENERATOR_OUTPUT(cymatics)
   REGISTER_GENERATOR_FULL(TRANSFORM_CYMATICS, Cymatics, cymatics, "Cymatics",
                           SetupCymaticsBlend, SetupCymatics, RenderCymatics, 13,
                           DrawCymaticsParams, DrawOutput_cymatics)
   ```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Integration cleanup — remove simulation infrastructure

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify), `src/render/render_pipeline.cpp` (modify), `src/config/effect_serialization.cpp` (modify), `CMakeLists.txt` (modify)

**Depends on**: Wave 1 complete

**Do**:

1. **`src/config/effect_config.h`**: Change `#include "simulation/cymatics.h"` to `#include "effects/cymatics.h"`.

2. **`src/render/post_effect.h`**:
   - Remove the forward declaration `typedef struct Cymatics Cymatics;`
   - Remove the `Cymatics *cymatics;` pointer field
   - Add `#include "effects/cymatics.h"` in the alphabetical include list
   - Add `CymaticsEffect cymatics;` member in the effect members section (after other generator effects like `muons`, `fireworks`, etc.)

3. **`src/render/post_effect.cpp`**:
   - Remove `#include "simulation/cymatics.h"`
   - Remove `pe->cymatics = CymaticsInit(...)` from init
   - Remove `CymaticsUninit(pe->cymatics)` from uninit
   - Remove `CymaticsResize(pe->cymatics, ...)` from resize
   - Remove `CymaticsReset(pe->cymatics)` from reset

4. **`src/render/render_pipeline.cpp`**:
   - Remove `#include "simulation/cymatics.h"`
   - Remove the entire `ApplyCymaticsPass` static function
   - Remove the `ApplyCymaticsPass(...)` call from `ApplySimulationPasses`
   - Update the comment on line ~252 to remove "cymatics" from the list

5. **`src/config/effect_serialization.cpp`**: Change `#include "simulation/cymatics.h"` to `#include "effects/cymatics.h"`.

6. **`CMakeLists.txt`**: Remove `src/simulation/cymatics.cpp` from `SIMULATION_SOURCES`. Add `src/effects/cymatics.cpp` to `EFFECTS_SOURCES` in alphabetical order.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 3: Cleanup

#### Task 3.1: Delete old simulation files

**Files**: `src/simulation/cymatics.cpp` (delete), `src/simulation/cymatics.h` (delete), `shaders/cymatics.glsl` (delete)

**Depends on**: Wave 2 complete (build must pass first)

**Do**: Delete the three old files. They are fully replaced by `src/effects/cymatics.h`, `src/effects/cymatics.cpp`, and `shaders/cymatics.fs`.

**Verify**: `cmake.exe --build build` compiles with no references to deleted files.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Cymatics appears in the Field generator section (section 13) of the UI
- [ ] Cymatics shows "GEN" badge
- [ ] Effect can be enabled and displays interference patterns
- [ ] Trail persistence works (patterns fade over time via decay)
- [ ] Source positions animate via lissajous controls
- [ ] Boundary reflections toggle on/off correctly
- [ ] Color modes (solid/gradient/palette/rainbow) work
- [ ] Blend mode and blend intensity controls work
- [ ] Preset save/load round-trips correctly (existing `CYMATICBOB.json` preset loads — `boostIntensity` field was renamed to `blendIntensity`, so old presets will get the default `blendIntensity=1.0` which matches the old `boostIntensity` default. Also `diffusionScale` and `debugOverlay` are dropped — both are harmlessly ignored by `from_json` with defaults.)
- [ ] Modulation routes to all registered parameters
- [ ] No remaining references to `src/simulation/cymatics.*` or `shaders/cymatics.glsl`
- [ ] No OpenGL 4.3 requirement — works on GL 3.3

---

## Implementation Notes

Deviations and fixes discovered during implementation:

1. **`ColorConfig` field renamed to `gradient`**: The plan spec used `color` but the `STANDARD_GENERATOR_OUTPUT` macro and all other generators use `gradient`. Renamed throughout header, source, and preset.

2. **Waveform texture binding**: The plan had the waveform ring buffer drawn as the fullscreen quad (`texture0`), but the waveform is a 1D texture (Nx1 pixels). `RenderUtilsDrawFullscreenQuad` draws at native texture dimensions, producing a single pixel row. Fix: the ping-pong read buffer is the fullscreen quad (`texture0`), and the waveform is bound via `SetShaderValueTexture` to a named `waveformTexture` uniform. The shader's `previousFrame` sampler was removed — `texture0` now serves that role (matching the muons pattern).

3. **File-static `s_waveformTexture` replaced with struct member**: The agent used a file-scope static to pass the waveform texture from Setup to Render. Replaced with `Texture2D currentWaveformTexture` in the `CymaticsEffect` struct to match the muons pattern (`currentFFTTexture`).

4. **`diffusionScale` restored**: The plan dropped diffusion, but without it the temporal-only `max()` decay was too weak — trails persisted indefinitely at high half-life because continuous audio signal kept refreshing pixels via `max(existing, newColor)`. Restored as a single-pass cross-shaped 5-tap Gaussian blur in the fragment shader (weights `[0.0625, 0.25, 0.375, 0.25, 0.0625]`), approximating the old separable two-pass compute shader diffusion. Range extended to 0-8 (from old 0-4) since the single-pass cross approximation is softer than true separable blur.

5. **CYMATICBOB preset updated**: Renamed `boostIntensity`→`blendIntensity`, `color`→`gradient`, mapped old flat source motion fields into `lissajous` sub-object, removed dropped fields (`debugOverlay`, `patternAngle`), added `diffusionScale`.

6. **Unused `waveformWriteIndex` parameter removed from `ApplySimulationPasses`**: After removing the `ApplyCymaticsPass` call, this parameter was dead code.

7. **Include ordering fixed**: The integration agent placed `#include "effects/cymatics.h"` among the `simulation/` includes in `effect_config.h` and `effect_serialization.cpp`. Moved to alphabetical position in the `effects/` block.
