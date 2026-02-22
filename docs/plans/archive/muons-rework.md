# Muons Rework

Add trail persistence, FFT audio reactivity, and fix blown-out pixels. Upgrades Muons from `REGISTER_GENERATOR` to `REGISTER_GENERATOR_FULL` (ping-pong trail persistence), adds per-depth-layer FFT frequency mapping, and clamps the accumulation divisor to prevent pixel blowout.

**Research**: `docs/research/muons-rework.md`

## Design

### Types

**MuonsConfig** — add 6 new fields after the existing `cameraDistance` field:

```
float decayHalfLife = 2.0f;  // Trail persistence duration in seconds (0.1-10.0)
float baseFreq = 55.0f;     // Lowest FFT frequency Hz (27.5-440.0)
float maxFreq = 14000.0f;   // Highest FFT frequency Hz (1000-16000)
float gain = 2.0f;          // FFT sensitivity multiplier (0.1-10.0)
float curve = 1.0f;         // FFT contrast curve exponent (0.1-3.0)
float baseBright = 0.1f;    // Minimum brightness floor when silent (0.0-1.0)
```

Add these to `MUONS_CONFIG_FIELDS` macro.

**MuonsEffect** — replace current struct with ping-pong pattern:

```
typedef struct MuonsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];  // Trail persistence pair
  int readIdx;                   // Which pingPong to read from (0 or 1)
  float time;
  // Existing uniform locations (unchanged)
  int resolutionLoc;
  int timeLoc;
  int marchStepsLoc;
  int turbulenceOctavesLoc;
  int turbulenceStrengthLoc;
  int ringThicknessLoc;
  int cameraDistanceLoc;
  int colorFreqLoc;
  int colorSpeedLoc;
  int brightnessLoc;
  int exposureLoc;
  int gradientLUTLoc;
  // New uniform locations
  int previousFrameLoc;
  int decayFactorLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} MuonsEffect;
```

**Function signatures change**:
- `MuonsEffectInit(MuonsEffect *e, const MuonsConfig *cfg, int width, int height)` — add width/height for ping-pong allocation
- `MuonsEffectSetup(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime, Texture2D fftTexture)` — add fftTexture param
- Add `MuonsEffectRender(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime, int screenWidth, int screenHeight)` — ping-pong render pass
- Add `MuonsEffectResize(MuonsEffect *e, int width, int height)` — reallocate ping-pong textures

### Algorithm

#### Trail Persistence (C++ side)

Init allocates ping-pong HDR textures via `RenderUtilsInitTextureHDR`. Decay factor computed on CPU each frame:
```cpp
const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
```

Render function (same pattern as `AttractorLinesEffectRender`):
```cpp
const int writeIdx = 1 - e->readIdx;
BeginTextureMode(e->pingPong[writeIdx]);
BeginShaderMode(e->shader);
SetShaderValueTexture(e->shader, e->previousFrameLoc, e->pingPong[e->readIdx].texture);
SetShaderValueTexture(e->shader, e->gradientLUTLoc, ColorLUTGetTexture(e->gradientLUT));
SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth, screenHeight);
EndShaderMode();
EndTextureMode();
e->readIdx = writeIdx;
```

Note: `fftTexture` must be stored in the effect struct during Setup so Render can access it. Add `Texture2D currentFFTTexture` to `MuonsEffect` for this handoff (same pattern used by other generators that split setup/render).

#### Trail Persistence (shader side)

After the existing raymarch loop and tonemap, composite with previous frame using `max()`:
```glsl
uniform sampler2D previousFrame;
uniform float decayFactor;

// ... after existing tonemap: color = tanh(color * brightness / exposure) ...
vec3 prev = texelFetch(previousFrame, ivec2(gl_FragCoord.xy), 0).rgb;
finalColor = vec4(max(color, prev * decayFactor), 1.0);
```

#### FFT Audio Reactivity (shader side)

New uniforms: `fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`.

Inside the march loop, after computing `sampleColor` but before accumulating, scale by FFT energy for that depth layer:
```glsl
// Map step index to log-spaced frequency
float t = float(i) / float(marchSteps - 1);
float freq = baseFreq * exp(t * log(maxFreq / baseFreq));
float bin = freq / (sampleRate * 0.5);
float fft = texture(fftTexture, vec2(bin, 0.5)).r;
float audio = baseBright + gain * pow(fft, curve);

color += sampleColor * audio / max(d * s, 0.01);
```

Note the `max(d * s, 0.01)` also addresses the blown pixel fix (raised from `1e-6`).

#### Blown Pixel Fix

Change `max(d * s, 1e-6)` to `max(d * s, 0.01)` — hardcoded, no config param. The `max()` compositing in the trail pass provides a second safety net against accumulation blowout.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| decayHalfLife | float | 0.1-10.0 | 2.0 | Yes | Decay Half-Life |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.0 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.1 | Yes | Base Bright |

### Constants

- Enum: `TRANSFORM_MUONS_BLEND` (unchanged)
- Display name: `"Muons Blend"` (unchanged)
- Registration: upgrade from `REGISTER_GENERATOR` to `REGISTER_GENERATOR_FULL`

---

## Tasks

### Wave 1: Config & Header

#### Task 1.1: Update MuonsConfig and MuonsEffect structs

**Files**: `src/effects/muons.h`
**Creates**: Updated types that Wave 2 tasks depend on

**Do**:
1. Add the 6 new config fields (decayHalfLife, baseFreq, maxFreq, gain, curve, baseBright) after `cameraDistance` in `MuonsConfig`, with range comments matching the Parameters table
2. Add all 6 new fields to the `MUONS_CONFIG_FIELDS` macro (before `gradient`)
3. Update `MuonsEffect` struct:
   - Add `RenderTexture2D pingPong[2]` and `int readIdx`
   - Add `Texture2D currentFFTTexture` for setup→render handoff
   - Add uniform location ints: `previousFrameLoc`, `decayFactorLoc`, `fftTextureLoc`, `sampleRateLoc`, `baseFreqLoc`, `maxFreqLoc`, `gainLoc`, `curveLoc`, `baseBrightLoc`
4. Change `MuonsEffectInit` signature: add `int width, int height` params
5. Change `MuonsEffectSetup` signature: add `Texture2D fftTexture` as last param
6. Add declaration: `void MuonsEffectRender(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime, int screenWidth, int screenHeight);`
7. Add declaration: `void MuonsEffectResize(MuonsEffect *e, int width, int height);`

**Verify**: `cmake.exe --build build` compiles (header-only changes won't cause link errors yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Update muons.cpp — C++ lifecycle and registration

**Files**: `src/effects/muons.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. Add includes: `"audio/audio.h"` (for `AUDIO_SAMPLE_RATE`), `"render/render_utils.h"`, `<math.h>`
2. Add static helpers `InitPingPong` and `UnloadPingPong` (same pattern as `attractor_lines.cpp` lines 50-58)
3. Update `MuonsEffectInit`:
   - Add `int width, int height` params
   - Cache new uniform locations (previousFrame, decayFactor, fftTexture, sampleRate, baseFreq, maxFreq, gain, curve, baseBright)
   - Call `InitPingPong(e, width, height)` after LUT init
   - Set `e->readIdx = 0`
4. Update `MuonsEffectSetup`:
   - Add `Texture2D fftTexture` param
   - Store `e->currentFFTTexture = fftTexture`
   - Compute `decayFactor` from `cfg->decayHalfLife` and `deltaTime` (same formula as attractor_lines.cpp line 143-144)
   - Bind `decayFactor` uniform
   - Bind `sampleRate` as `(float)AUDIO_SAMPLE_RATE`
   - Bind `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright` uniforms
5. Add `MuonsEffectRender` function — same pattern as `AttractorLinesEffectRender` (lines 183-208):
   - `writeIdx = 1 - e->readIdx`
   - `BeginTextureMode(e->pingPong[writeIdx])`
   - `BeginShaderMode(e->shader)`
   - Bind `previousFrame`, `gradientLUT`, and `fftTexture` textures AFTER BeginTextureMode/BeginShaderMode (they flush batches)
   - `RenderUtilsDrawFullscreenQuad`
   - End shader/texture mode
   - Swap `readIdx`
6. Add `MuonsEffectResize` — same as attractor_lines.cpp lines 210-215
7. Update `MuonsEffectUninit` — add `UnloadPingPong(e)` call
8. Update `MuonsRegisterParams` — register all 6 new params with correct ranges
9. Update `SetupMuons` bridge function — pass `pe->fftTexture` as last arg
10. Update `SetupMuonsBlend` — read from `pe->muons.pingPong[pe->muons.readIdx].texture` instead of `pe->generatorScratch.texture`
11. Add `RenderMuons` bridge function (same pattern as `RenderAttractorLines`):
    ```
    void RenderMuons(PostEffect *pe) {
      MuonsEffectRender(&pe->muons, &pe->effects.muons,
                        pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
    }
    ```
12. Change registration macro from `REGISTER_GENERATOR` to `REGISTER_GENERATOR_FULL`:
    ```
    REGISTER_GENERATOR_FULL(TRANSFORM_MUONS_BLEND, Muons, muons, "Muons Blend",
                            SetupMuonsBlend, SetupMuons, RenderMuons)
    ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Update muons.fs — shader changes

**Files**: `shaders/muons.fs`
**Depends on**: Wave 1 complete

**Do**:
1. Add new uniforms after `gradientLUT`:
   ```glsl
   uniform sampler2D previousFrame;
   uniform float decayFactor;
   uniform sampler2D fftTexture;
   uniform float sampleRate;
   uniform float baseFreq;
   uniform float maxFreq;
   uniform float gain;
   uniform float curve;
   uniform float baseBright;
   ```
2. Inside the march loop, REPLACE the accumulation line. Change:
   ```glsl
   color += sampleColor / max(d * s, 1e-6);
   ```
   To:
   ```glsl
   float t = float(i) / float(marchSteps - 1);
   float freq = baseFreq * exp(t * log(maxFreq / baseFreq));
   float bin = freq / (sampleRate * 0.5);
   float fft = texture(fftTexture, vec2(bin, 0.5)).r;
   float audio = baseBright + gain * pow(fft, curve);
   color += sampleColor * audio / max(d * s, 0.01);
   ```
   Guard `marchSteps - 1` with `max(marchSteps - 1, 1)` to avoid division by zero when `marchSteps == 1`.
3. After the tonemap line (`color = tanh(color * brightness / exposure);`), add trail compositing:
   ```glsl
   vec3 prev = texelFetch(previousFrame, ivec2(gl_FragCoord.xy), 0).rgb;
   finalColor = vec4(max(color, prev * decayFactor), 1.0);
   ```
   Remove the old `finalColor = vec4(color, 1.0);` line.

**Verify**: Shader changes are runtime-compiled — no build needed. Visual verification at runtime.

---

#### Task 2.3: Update Muons UI — add Audio and Trails sections

**Files**: `src/ui/imgui_effects_gen_filament.cpp`
**Depends on**: Wave 1 complete

**Do**:
Update `DrawGeneratorsMuons` function. Add two new sections after the existing "Raymarching" section:

1. **Trails section** (after Raymarching, before Color):
   ```
   ImGui::SeparatorText("Trails");
   ModulatableSlider("Decay Half-Life##muons", &m->decayHalfLife,
                     "muons.decayHalfLife", "%.1f", modSources);
   ```

2. **Audio section** (after Trails, before Color) — follows standard FFT Audio UI conventions:
   ```
   ImGui::SeparatorText("Audio");
   ModulatableSlider("Base Freq (Hz)##muons", &m->baseFreq,
                     "muons.baseFreq", "%.1f", modSources);
   ModulatableSlider("Max Freq (Hz)##muons", &m->maxFreq,
                     "muons.maxFreq", "%.0f", modSources);
   ModulatableSlider("Gain##muons", &m->gain, "muons.gain", "%.1f", modSources);
   ModulatableSlider("Contrast##muons", &m->curve, "muons.curve", "%.2f", modSources);
   ModulatableSlider("Base Bright##muons", &m->baseBright,
                     "muons.baseBright", "%.2f", modSources);
   ```

Section order becomes: Raymarching → Trails → Audio → Color → Tonemap → Output.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Muons renders with persistent trails (visible glow lingering between frames)
- [ ] Different march depths respond to different frequency bands
- [ ] Bass-heavy audio lights up near shells, treble lights up deep volume
- [ ] Silent audio shows `baseBright` floor glow, not black
- [ ] `decayHalfLife` slider controls trail persistence duration
- [ ] No blown-out white pixels at ring surface grazes
- [ ] Preset save/load preserves all 6 new parameters
- [ ] Modulation routes to all 6 new params
- [ ] Resize window doesn't crash (ping-pong textures reallocate)
