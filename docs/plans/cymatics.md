# Cymatics

Cymatics visualizes sound as interference patterns by placing 5 virtual speakers in 2D space and sampling an audio history buffer at distance-based delays. Patterns persist via TrailMap with diffusion/decay. Drums produce expanding rings; sustained tones create stable interference grids.

## Current State

Relevant files to hook into:
- `src/analysis/analysis_pipeline.h:9-16` - Add waveform history buffer here
- `src/analysis/analysis_pipeline.cpp:63-109` - Append samples after AudioCaptureRead
- `src/render/post_effect.h:260-266` - Add Cymatics pointer and waveformTexture
- `src/render/post_effect.cpp:378-386` - Init Cymatics following Physarum pattern
- `src/render/render_pipeline.cpp:48-87` - ApplyCymaticsPass pattern
- `src/render/render_pipeline.cpp:298-331` - Trail boost output stage
- `src/config/effect_config.h:204-220` - Add CymaticsConfig

## Technical Implementation

### Waveform Ring Buffer Sampling

The shader samples audio at distance-based delays from 5 fixed sources:

```glsl
// Source positions (normalized, center-relative)
const vec2 sources[5] = vec2[](
    vec2(0.0, 0.0),     // Center
    vec2(-0.4, 0.0),    // Left
    vec2(0.4, 0.0),     // Right
    vec2(0.0, -0.4),    // Bottom
    vec2(0.0, 0.4)      // Top
);

// Sample waveform at ring buffer offset
float sampleWaveform(float delay) {
    float idx = mod(float(writeIndex) - delay + float(bufferSize), float(bufferSize));
    return texelFetch(waveformBuffer, ivec2(int(idx), 0), 0).r * 2.0 - 1.0;
}

// Sum interference from all sources
float totalWave = 0.0;
for (int i = 0; i < 5; i++) {
    float dist = length(uv - sources[i]);
    float delay = dist * waveSpeed;
    float amplitude = 1.0 / (1.0 + dist * falloff);
    totalWave += sampleWaveform(delay) * amplitude;
}
```

### Contour Banding (Optional)

```glsl
// contourCount = 0: smooth gradient
// contourCount > 0: discrete bands
float wave = totalWave;
if (contourCount > 0) {
    wave = floor(totalWave * float(contourCount) + 0.5) / float(contourCount);
}
```

### Output Compression

```glsl
float intensity = tanh(wave * visualGain);
float t = intensity * 0.5 + 0.5;  // Map [-1,1] to [0,1]
vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb * value;
```

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| waveSpeed | float | 1-50 | 10.0 | Samples per unit distance |
| falloff | float | 0-5 | 1.0 | Distance attenuation |
| visualGain | float | 0.5-5 | 2.0 | Output intensity |
| contourCount | int | 0-10 | 0 | Banding (0=smooth) |
| decayHalfLife | float | 0.1-5 | 0.5 | Trail persistence |
| diffusionScale | int | 0-4 | 1 | Blur kernel size |

---

## Phase 1: Audio History Infrastructure

**Goal**: Persistent waveform ring buffer with GPU texture upload.

**Build**:
- Add to `analysis_pipeline.h`: `float waveformHistory[2048]` and `int waveformWriteIndex` fields in AnalysisPipeline struct
- Modify `analysis_pipeline.cpp`: After AudioCaptureRead, mono-mix samples and append to ring buffer: `mono * 0.5f + 0.5f` (pack to [0,1])
- Add to `post_effect.h`: `Texture2D waveformTexture` field
- Add to `post_effect.cpp`: Init waveformTexture as 2048x1 R32 (follow InitFFTTexture pattern)
- Add to `render_pipeline.cpp`: UpdateWaveformTexture function (upload ring buffer each frame)

**Done when**: Waveform texture uploads each frame with valid audio data.

---

## Phase 2: Cymatics Simulation Core

**Goal**: Compute shader deposits interference patterns to TrailMap.

**Build**:
- Create `src/simulation/cymatics.h`:
  - `CymaticsConfig` struct with: enabled, waveSpeed, falloff, visualGain, contourCount, decayHalfLife, diffusionScale, boostIntensity, blendMode, debugOverlay, ColorConfig
  - `Cymatics` struct with: computeProgram, trailMap, colorLUT, debugShader, width, height, uniform locations, config, supported
  - Function declarations: CymaticsSupported, CymaticsInit, CymaticsUninit, CymaticsUpdate, CymaticsProcessTrails, CymaticsResize, CymaticsReset, CymaticsApplyConfig, CymaticsDrawDebug

- Create `src/simulation/cymatics.cpp`:
  - CymaticsInit: Load compute shader, create TrailMap via TrailMapInit, create ColorLUT via ColorLUTInit, cache uniform locations
  - CymaticsUpdate: Bind waveformTexture (unit 0), bind trailMap (image binding 1), bind colorLUT (unit 3), set uniforms, dispatch 2D compute (16x16 workgroups), memory barrier
  - CymaticsProcessTrails: Call TrailMapProcess with decayHalfLife and diffusionScale
  - Other functions follow curl_advection.cpp patterns

- Create `shaders/cymatics.glsl`:
  - Compute shader with layout(local_size_x=16, local_size_y=16)
  - Bindings: sampler2D waveformBuffer (0), image2D trailMap (1), sampler2D colorLUT (3)
  - Uniforms: resolution, waveSpeed, falloff, visualGain, contourCount, bufferSize, writeIndex, value
  - Algorithm: For each pixel, sample 5 sources at distance-based delays, sum with falloff, apply contours and tanh, map through ColorLUT, deposit to trailMap via imageStore

**Done when**: Cymatics compiles and runs with visible interference patterns.

---

## Phase 3: Pipeline Integration

**Goal**: Cymatics runs each frame and blends to output.

**Build**:
- Add to `post_effect.h`: `Cymatics* cymatics` field, forward declare `typedef struct Cymatics Cymatics;`
- Modify `post_effect.cpp`:
  - Include `simulation/cymatics.h`
  - Init: `pe->cymatics = CymaticsInit(screenWidth, screenHeight, NULL);`
  - Uninit: `CymaticsUninit(pe->cymatics);`
  - Resize: `CymaticsResize(pe->cymatics, width, height);`
  - ClearFeedback: Add `CymaticsReset(pe->cymatics);`

- Modify `render_pipeline.cpp`:
  - Add `ApplyCymaticsPass(PostEffect*, float deltaTime, Texture2D waveformTexture, int writeIndex)` following ApplyCurlFlowPass pattern
  - Call in ApplySimulationPasses
  - Add trail boost block in RenderPipelineApplyOutput (check cymatics != NULL && enabled && boostIntensity > 0)

- Modify `shader_setup.h`: Declare `SetupCymaticsTrailBoost(PostEffect*)`
- Modify `shader_setup.cpp`: Implement SetupCymaticsTrailBoost calling BlendCompositorApply with TrailMapGetTexture

**Done when**: Cymatics patterns composite to final output via BlendCompositor.

---

## Phase 4: Config & UI

**Goal**: Full preset support and UI controls.

**Build**:
- Modify `effect_config.h`:
  - Add `#include "simulation/cymatics.h"`
  - Add `CymaticsConfig cymatics;` to EffectConfig struct

- Modify `preset.cpp`:
  - Add JSON macro: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CymaticsConfig, enabled, waveSpeed, falloff, visualGain, contourCount, decayHalfLife, diffusionScale, boostIntensity, blendMode, debugOverlay, color)`
  - Add to to_json: `if (e.cymatics.enabled) { j["cymatics"] = e.cymatics; }`
  - Add to from_json: `e.cymatics = j.value("cymatics", e.cymatics);`

- Modify `param_registry.cpp`:
  - Add entries to PARAM_TABLE for modulatable params: cymatics.waveSpeed, cymatics.falloff, cymatics.visualGain, cymatics.boostIntensity
  - Add matching entries to targets array

- Modify `imgui_effects.cpp`:
  - Add `static bool sectionCymatics = false;` with other section states
  - Add Cymatics section in SIMULATIONS group (DrawSectionBegin, Checkbox, ModulatableSliders for params, ColorModeCombo, Debug Checkbox, DrawSectionEnd)

- Update CMakeLists.txt: Add cymatics.cpp to sources

**Done when**: Cymatics appears in UI, presets save/load, modulation works.

---

## Verification Checklist

- [ ] Waveform texture uploads each frame (validate with RenderDoc)
- [ ] Compute shader compiles on OpenGL 4.3
- [ ] Interference patterns visible with test audio
- [ ] TrailMap diffusion/decay working (patterns persist and blur)
- [ ] BlendCompositor output blending functional
- [ ] UI sliders modify effect in real-time
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
- [ ] Build succeeds with no warnings
- [ ] No memory leaks (valgrind/sanitizers)

## File Summary

| File | Action |
|------|--------|
| `src/analysis/analysis_pipeline.h` | Add waveformHistory buffer fields |
| `src/analysis/analysis_pipeline.cpp` | Append samples to ring buffer |
| `src/simulation/cymatics.h` | Create config struct and API |
| `src/simulation/cymatics.cpp` | Create full lifecycle implementation |
| `shaders/cymatics.glsl` | Create compute shader |
| `src/render/post_effect.h` | Add Cymatics pointer, waveformTexture |
| `src/render/post_effect.cpp` | Init/Uninit/Resize waveformTexture and Cymatics |
| `src/render/render_pipeline.cpp` | Add UpdateWaveformTexture, ApplyCymaticsPass, trail boost |
| `src/render/shader_setup.h` | Declare SetupCymaticsTrailBoost |
| `src/render/shader_setup.cpp` | Implement SetupCymaticsTrailBoost |
| `src/config/effect_config.h` | Include and add CymaticsConfig member |
| `src/config/preset.cpp` | JSON serialization |
| `src/automation/param_registry.cpp` | Modulation parameter entries |
| `src/ui/imgui_effects.cpp` | UI section |
| `CMakeLists.txt` | Add source file |
