# feedbackPhase Implementation

Add per-drawable feedbackPhase slider (0.0-1.0) controlling how integrated vs crisp waveforms/spectrum appear in the feedback effects.

- **0.0** = Fully integrated (drawn before feedback shader, gets warped and blurred)
- **1.0** = Fully crisp on top (current behavior, drawn after feedback)
- **0.5** = 50/50 blend (half integrated, half crisp)

## Current State

Render order in `main.cpp:157-168`:
```
RenderPipelineApplyFeedback()  // voronoi, flow field, blur, decay
RenderWaveformsToPhysarum()    // draw to physarum trail map
RenderWaveformsToAccum()       // draw fresh waveforms ON TOP
```

Waveforms always render after feedback, appearing crisp. No way to integrate them into the feedback warp/blur.

Key files:
- `src/config/waveform_config.h:7` - WaveformConfig struct
- `src/config/spectrum_bars_config.h:8` - SpectrumConfig struct
- `src/config/preset.cpp:84-88` - JSON serialization macros
- `src/render/waveform.cpp:17` - GetSegmentColor (computes per-segment color)
- `src/render/spectrum_bars.cpp:116` - GetBandColor (computes per-band color)
- `src/main.cpp:149-154` - RenderWaveformsToAccum
- `src/main.cpp:157-168` - RenderStandardPipeline

---

## Phase 1: Config & Helper

**Goal**: Add feedbackPhase field and create color opacity helper.

**Build**:
- Add `float feedbackPhase = 1.0f;` to WaveformConfig after rotationOffset
- Add `float feedbackPhase = 1.0f;` to SpectrumConfig after rotationOffset
- Add `feedbackPhase` to JSON serialization macros in preset.cpp
- Create `src/render/draw_utils.h/cpp` with helper to multiply Color alpha by a 0-1 value

**Done when**: Project builds, presets save/load with feedbackPhase (defaults to 1.0).

---

## Phase 2: Draw Functions Accept Opacity

**Goal**: Draw functions can render at reduced opacity for split-pass rendering.

**Build**:
- Modify GetSegmentColor in waveform.cpp to accept opacity parameter and reduce output alpha
- Modify GetBandColor in spectrum_bars.cpp to accept opacity parameter and reduce output alpha
- Update draw function signatures to pass opacity through
- Temporarily pass 1.0 everywhere (visual output unchanged)

**Done when**: Project builds, visual output identical to before.

---

## Phase 3: Split Rendering Pipeline

**Goal**: Draw waveforms in two passes - before and after feedback.

**Build**:
- Replace RenderWaveformsToAccum with two functions:
  - RenderWaveformsPreFeedback: draws at opacity `(1.0 - feedbackPhase)` BEFORE feedback shader
  - RenderWaveformsPostFeedback: draws at opacity `feedbackPhase` AFTER feedback shader
- Skip pass entirely if opacity < 0.001 (optimization)
- Update RenderStandardPipeline order:
  1. RenderWaveformsPreFeedback
  2. RenderPipelineApplyFeedback
  3. RenderWaveformsToPhysarum (unchanged, always full opacity)
  4. RenderWaveformsPostFeedback
- Physarum trail map always receives full opacity (agent sensing unaffected)

**Done when**: feedbackPhase=1.0 matches current behavior, feedbackPhase=0.0 shows integrated waveforms.

---

## Phase 4: UI Sliders

**Goal**: Expose feedbackPhase in UI.

**Build**:
- Add "Feedback Phase" slider to waveform panel (imgui_waveforms.cpp)
- Add "Feedback Phase" slider to spectrum panel (imgui_spectrum.cpp)

**Done when**: Sliders appear, adjusting them changes waveform integration in real-time.

---

## Files Changed

| File | Change |
|------|--------|
| `src/config/waveform_config.h` | Add feedbackPhase field |
| `src/config/spectrum_bars_config.h` | Add feedbackPhase field |
| `src/config/preset.cpp` | Add feedbackPhase to serialization |
| `src/render/draw_utils.h` | New - opacity helper |
| `src/render/draw_utils.cpp` | New - opacity helper |
| `src/render/waveform.h` | Add opacity parameter |
| `src/render/waveform.cpp` | Modify GetSegmentColor, draw functions |
| `src/render/spectrum_bars.h` | Add opacity parameter |
| `src/render/spectrum_bars.cpp` | Modify GetBandColor, draw functions |
| `src/render/waveform_pipeline.h` | Add opacity parameter |
| `src/render/waveform_pipeline.cpp` | Thread opacity to draw calls |
| `src/main.cpp` | Split into pre/post feedback passes |
| `src/ui/imgui_waveforms.cpp` | Add slider |
| `src/ui/imgui_spectrum.cpp` | Add slider |
| `CMakeLists.txt` | Add draw_utils.cpp |
