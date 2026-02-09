# Signal Frames

Layered rotating geometric outlines — rectangles and triangles alternating in concentric rings, each mapped to an FFT semitone. A brightness sweep rolls through the layer stack like a strobe. Active notes flare in gradient LUT color; quiet notes dim to embers.

**Research**: `docs/research/signal_frames.md`

## Design

### Types

**SignalFramesConfig** (user-facing parameters):

```
enabled          bool           false
numOctaves       int            3          (1-5, runtime slider)
baseFreq         float          55.0       (27.5-440.0 Hz)
gain             float          5.0        (1.0-20.0)
curve            float          1.5        (0.5-3.0)
baseBright       float          0.05       (0.0-0.5)
rotationSpeed    float          0.5        (rad/s, -3.0 to 3.0)
orbitRadius      float          0.3        (0.0-1.5)
orbitSpeed       float          0.4        (0.0-3.0)
sizeMin          float          0.05       (0.01-0.5)
sizeMax          float          0.6        (0.1-1.5)
aspectRatio      float          1.5        (0.2-5.0)
outlineThickness float          0.01       (0.002-0.05)
glowWidth        float          0.005      (0.001-0.05)
glowIntensity    float          2.0        (0.5-10.0)
sweepSpeed       float          0.5        (0.0-3.0)
sweepIntensity   float          0.02       (0.0-0.1)
gradient         ColorConfig    COLOR_MODE_GRADIENT
blendMode        EffectBlendMode EFFECT_BLEND_SCREEN
blendIntensity   float          1.0
```

**SignalFramesEffect** (runtime state):

```
shader           Shader
gradientLUT      ColorLUT*
rotationAccum    float          // CPU-accumulated rotation phase (rotationSpeed * dt)
sweepAccum       float          // CPU-accumulated sweep phase (sweepSpeed * dt)
time             float          // CPU-accumulated elapsed time (1:1 with real time)
resolutionLoc    int
fftTextureLoc    int
sampleRateLoc    int
timeLoc          int
+ one int loc per uniform (numOctavesLoc, baseFreqLoc, gainLoc, curveLoc,
  baseBrightLoc, rotationAccumLoc, orbitRadiusLoc, orbitSpeedLoc,
  sizeMinLoc, sizeMaxLoc, aspectRatioLoc, outlineThicknessLoc,
  glowWidthLoc, glowIntensityLoc, sweepAccumLoc, sweepIntensityLoc,
  gradientLUTLoc)
```

### Algorithm

The shader loops over `numOctaves * 12` layers. Each layer `i` maps to one semitone. Normalized index `t = float(i) / float(totalLayers)` drives per-layer scaling.

Per layer:
1. Rotate UV by `t * rotationAccum + sin(time * orbitSpeed)` — `time` is the independent elapsed-time accumulator, keeping orbital wobble decoupled from rotation speed
2. Apply orbital offset: `uv += vec2(sin, cos)(time * orbitSpeed + t * TAU) * orbitRadius * t` — same independent `time`
3. Compute size as `mix(sizeMin, sizeMax, t)`
4. Even layers evaluate `sdBox(uv, vec2(size * aspectRatio, size))`, odd layers evaluate `sdEquilateralTriangle(uv, size)` — aspect ratio applies only to box layers
5. Extract outline: `abs(sdf) - outlineThickness`
6. Compute Lorentzian glow: `glowWidth^2 / (glowWidth^2 + d^2) * glowIntensity`
7. Sweep boost: `sweepPhase = fract(sweepAccum + t)`, boost = `sweepIntensity / (sweepPhase + 0.0001)`
8. FFT lookup: standard semitone mapping `baseFreq * pow(2, semitone/12)` -> bin -> texture sample
9. Composite: `glow * (baseBright + mag * pow(gain, curve)) * (1.0 + sweepBoost)`
10. Color from `gradientLUT` at `vec2(pitchClass / 12.0, 0.5)`

SDF functions (`sdBox`, `sdEquilateralTriangle`) come directly from the research doc (IQ reference). Additive layer accumulation with `1 - exp(-total)` tonemap.

CPU side accumulates three values: `rotationAccum += rotationSpeed * dt`, `sweepAccum += sweepSpeed * dt`, and `time += dt`. The `time` accumulator drives orbital offset and rotation wobble independently from rotation speed. All other uniforms pass through directly.

<!-- Intentional deviation: tonemap `1 - exp(-total)` added to prevent additive clipping across 60 layers. Research doc says "accumulate additively" but does not address HDR overflow. Plasma uses the same tonemap. -->
<!-- Intentional deviation: glowIntensity multiplied into Lorentzian. Research doc lists the parameter but does not specify integration point. -->
<!-- Intentional deviation: blendMode/blendIntensity/enabled are standard generator framework fields, not in research doc. -->

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| numOctaves | int | 1-5 | 3 | No | Octaves |
| baseFreq | float | 27.5-440.0 | 55.0 | No | Base Freq (Hz) |
| gain | float | 1.0-20.0 | 5.0 | No | Gain |
| curve | float | 0.5-3.0 | 1.5 | No | Contrast |
| baseBright | float | 0.0-0.5 | 0.05 | No | Base Bright |
| rotationSpeed | float | -3.0-3.0 | 0.5 | Yes | Rotation Speed |
| orbitRadius | float | 0.0-1.5 | 0.3 | Yes | Orbit Radius |
| orbitSpeed | float | 0.0-3.0 | 0.4 | No | Orbit Speed |
| sizeMin | float | 0.01-0.5 | 0.05 | Yes | Size Min |
| sizeMax | float | 0.1-1.5 | 0.6 | Yes | Size Max |
| aspectRatio | float | 0.2-5.0 | 1.5 | Yes | Aspect Ratio |
| outlineThickness | float | 0.002-0.05 | 0.01 | Yes | Outline Thickness |
| glowWidth | float | 0.001-0.05 | 0.005 | Yes | Glow Width |
| glowIntensity | float | 0.5-10.0 | 2.0 | Yes | Glow Intensity |
| sweepSpeed | float | 0.0-3.0 | 0.5 | Yes | Sweep Speed |
| sweepIntensity | float | 0.0-0.1 | 0.02 | Yes | Sweep Intensity |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_SIGNAL_FRAMES_BLEND`
- Display name: `"Signal Frames Blend"`
- Category: `{"GEN", 10}`
- Config member: `signalFrames`
- Effect member: `signalFrames`
- Shader file: `shaders/signal_frames.fs`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/signal_frames.h` (create)
**Creates**: `SignalFramesConfig` struct, `SignalFramesEffect` struct, function declarations

**Do**: Follow `spectral_arcs.h` pattern exactly. Include `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`. Define `SignalFramesConfig` with all fields from the Parameters table above. Define `SignalFramesEffect` with shader, gradientLUT pointer, three accumulators (rotationAccum, sweepAccum, time), `timeLoc`, and one `int` location per uniform. Declare Init (takes effect + config pointers), Setup (takes effect + config + deltaTime + fftTexture), Uninit, ConfigDefault, RegisterParams.

**Verify**: Header parses without errors.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create effect implementation

**Files**: `src/effects/signal_frames.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `spectral_arcs.cpp` pattern. Include `signal_frames.h`, `audio/audio.h`, `automation/modulation_engine.h`, `config/constants.h`, `render/color_lut.h`, `<stddef.h>`. Init loads shader, caches all uniform locations (including `timeLoc` for the `"time"` uniform), creates ColorLUT, zeroes all three accumulators. Setup accumulates `rotationAccum += rotationSpeed * dt`, `sweepAccum += sweepSpeed * dt`, and `time += dt`, updates LUT, binds all uniforms including `fftTexture`, `sampleRate = (float)AUDIO_SAMPLE_RATE`, and `time`. Uninit unloads shader and LUT. RegisterParams registers the 10 modulatable params (9 candidates + blendIntensity) using ranges from the Parameters table.

**Verify**: Compiles (after all wave 2 tasks complete).

---

#### Task 2.2: Create fragment shader

**Files**: `shaders/signal_frames.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section. Uniforms match every config field plus `resolution`, `fftTexture`, `sampleRate`, `rotationAccum`, `sweepAccum`, `time`. Include `sdBox` and `sdEquilateralTriangle` from the research doc verbatim. Layer loop over `numOctaves * 12`, even/odd shape alternation (aspect ratio applies only to box layers), Lorentzian glow, sweep strobe, FFT semitone lookup, additive accumulation, `1 - exp(-total)` tonemap. Use `time` (not `rotationAccum`) for orbital offset and rotation wobble sin() term. Color from gradientLUT at `pitchClass / 12.0`.

**Verify**: Shader file exists and has correct uniform declarations.

---

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow the add-effect checklist Phase 2. Add `#include "effects/signal_frames.h"`. Add `TRANSFORM_SIGNAL_FRAMES_BLEND` before `TRANSFORM_EFFECT_COUNT`. Add name case returning `"Signal Frames Blend"`. Add to `TransformOrderConfig::order` array. Add `SignalFramesConfig signalFrames;` member to `EffectConfig`. Add `IsTransformEnabled` case.

**Verify**: Compiles (after all wave 2 tasks complete).

---

#### Task 2.4: Integrate in PostEffect

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: In `post_effect.h`: add `#include "effects/signal_frames.h"` and `SignalFramesEffect signalFrames;` member in `PostEffect` struct (near other generator effects like spectralArcs). In `post_effect.cpp`: add `SignalFramesEffectInit` call in `PostEffectInit()` with error handling and TraceLog on failure, add `SignalFramesRegisterParams` call in `PostEffectRegisterParams()`, add `SignalFramesEffectUninit` call in `PostEffectUninit()`. Follow exact placement pattern of spectralArcs.

**Verify**: Compiles (after all wave 2 tasks complete).

---

#### Task 2.5: Shader setup dispatch

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: In `shader_setup_generators.h`: declare `SetupSignalFrames(PostEffect *pe)` and `SetupSignalFramesBlend(PostEffect *pe)`. In `shader_setup_generators.cpp`: add `#include "effects/signal_frames.h"`, implement `SetupSignalFrames` delegating to `SignalFramesEffectSetup(&pe->signalFrames, &pe->effects.signalFrames, pe->currentDeltaTime, pe->fftTexture)`, implement `SetupSignalFramesBlend` calling `BlendCompositorApply`. Add `TRANSFORM_SIGNAL_FRAMES_BLEND` case to `GetGeneratorScratchPass` returning `{pe->signalFrames.shader, SetupSignalFrames}`. In `shader_setup.cpp`: add dispatch case `TRANSFORM_SIGNAL_FRAMES_BLEND` returning `{&pe->signalFrames.shader, SetupSignalFramesBlend, &pe->effects.signalFrames.enabled}` in `GetTransformEffect`.

**Verify**: Compiles (after all wave 2 tasks complete).

---

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_generators.cpp` (modify), `src/ui/imgui_effects.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: In `imgui_effects_generators.cpp`: add `#include "effects/signal_frames.h"`, add `static bool sectionSignalFrames = false;`. Create `DrawSignalFramesParams` helper with sections: FFT (Octaves slider int 1-5, Base Freq, Gain, Contrast), Geometry (Orbit Radius, Orbit Speed, Size Min, Size Max, Aspect Ratio), Outline (Outline Thickness, Glow Width, Glow Intensity), Sweep (Sweep Speed, Sweep Intensity), Animation (Rotation Speed via `ModulatableSliderSpeedDeg`). Create `DrawSignalFramesOutput` helper with gradient + blend output. Create `DrawGeneratorsSignalFrames` orchestrator using `DrawSectionBegin("Signal Frames", ...)`, enable checkbox with `MoveTransformToEnd(&e->transformOrder, TRANSFORM_SIGNAL_FRAMES_BLEND)`. Call from `DrawGeneratorsCategory` with spacing (add before SolidColor at bottom). In `imgui_effects.cpp`: add `TRANSFORM_SIGNAL_FRAMES_BLEND` case to `GetTransformCategory` returning `{"GEN", 10}`.

**Verify**: Compiles (after all wave 2 tasks complete).

---

#### Task 2.7: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SignalFramesConfig, enabled, numOctaves, baseFreq, gain, curve, baseBright, rotationSpeed, orbitRadius, orbitSpeed, sizeMin, sizeMax, aspectRatio, outlineThickness, glowWidth, glowIntensity, sweepSpeed, sweepIntensity, gradient, blendMode, blendIntensity)`. Add `to_json` entry: `if (e.signalFrames.enabled) { j["signalFrames"] = e.signalFrames; }`. Add `from_json` entry: `e.signalFrames = j.value("signalFrames", e.signalFrames);`.

**Verify**: Compiles (after all wave 2 tasks complete).

---

#### Task 2.8: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/signal_frames.cpp` to `EFFECTS_SOURCES` list.

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Generators category with "GEN" badge
- [ ] Effect appears in transform order pipeline and can be reordered
- [ ] Enabling shows concentric rotating rectangle/triangle outlines
- [ ] FFT input drives layer brightness per semitone
- [ ] Sweep strobe rolls through layer stack
- [ ] Gradient LUT colors layers by pitch class
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to 10 registered parameters
