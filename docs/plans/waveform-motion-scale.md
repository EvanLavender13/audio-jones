# Waveform Motion Scale

Per-drawable temporal smoothing for waveform shape. Replaces instant-snap updates with EMA interpolation, creating slow-motion morphing between audio frames while preserving existing spatial smoothing and drawInterval behavior.

## Current State

- `src/config/drawable_config.h:21` — `WaveformData` struct (amplitudeScale, thickness, smoothness, radius)
- `src/render/drawable.h:14` — `DrawableState` holds shared `waveform[1024]` and per-drawable `waveformExtended[16][2048]`
- `src/render/drawable.cpp:66` — `DrawableProcessWaveforms` calls `ProcessWaveformBase` then per-drawable `ProcessWaveformSmooth`
- `src/ui/drawable_type_controls.cpp:29` — `DrawWaveformControls` renders Geometry/Animation/Color sections
- `src/config/preset.cpp:405` — `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `WaveformData`
- `src/automation/drawable_params.cpp:7` — `DrawableParamsRegister` registers modulatable params

## Technical Implementation

**Core algorithm**: Exponential moving average (EMA) on raw waveform samples per drawable.

```
smoothedWaveform[i] = lerp(smoothedWaveform[i], rawWaveform[i], alpha)
                    = smoothedWaveform[i] + alpha * (rawWaveform[i] - smoothedWaveform[i])
```

Where `alpha = waveformMotionScale` (0.0 = frozen, 1.0 = instant response).

<!-- Intentional deviation from research: logarithmic slider suggested for global motionScale, applied here to waveformMotionScale for same UX reason (interesting range clusters at low values) -->
**Power-curve UI mapping**: The slider stores `waveformMotionScale` as-is (0.0–1.0), but the UI applies a power curve so the left portion of the slider covers the useful slow-motion range (0.01–0.3) with finer resolution. Use `ImGui::SliderFloat` with `ImGuiSliderFlags_Logarithmic` flag for built-in log-scale behavior.

**Buffer lifecycle**: Per-drawable `smoothedWaveform[MAX_DRAWABLES][WAVEFORM_SAMPLES]` persists across frames in `DrawableState`. Initialized to zero by `memset` in `DrawableStateInit`. First frame with `waveformMotionScale = 1.0` (default) copies raw data directly, so no startup artifact.

**Pipeline insertion point**: After `ProcessWaveformBase` writes `state->waveform`, before each per-drawable `ProcessWaveformSmooth` call. The EMA reads from `state->waveform` (raw) and writes to `state->smoothedWaveform[index]`, then `ProcessWaveformSmooth` reads from the smoothed buffer instead of the raw buffer.

---

## Phase 1: Config + Buffer + Processing

**Goal**: Add `waveformMotionScale` field, persistent buffer, and EMA pass.
**Depends on**: —
**Files**: `src/config/drawable_config.h`, `src/render/drawable.h`, `src/render/drawable.cpp`

**Build**:
- Add `float waveformMotionScale = 1.0f;` to `WaveformData`
- Add `float smoothedWaveform[MAX_DRAWABLES][WAVEFORM_SAMPLES];` to `DrawableState`
- In `DrawableProcessWaveforms`, after `ProcessWaveformBase`, iterate per waveform drawable:
  - Apply EMA from `state->waveform` into `state->smoothedWaveform[waveformIndex]` using drawable's `waveformMotionScale`
  - Pass `state->smoothedWaveform[waveformIndex]` to `ProcessWaveformSmooth` instead of `state->waveform`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → waveform renders identically (default 1.0 = no change).

**Done when**: Waveform displays normally with no visual difference at default settings.

---

## Phase 2: UI + Serialization + Modulation

**Goal**: Expose slider, persist to presets, register for modulation.
**Depends on**: Phase 1
**Files**: `src/ui/drawable_type_controls.cpp`, `src/config/preset.cpp`, `src/automation/drawable_params.cpp`

**Build**:
<!-- Intentional deviation from research: slider min 0.01 (not 0.0) prevents UX dead-end of frozen waveform; modulation range [0.0, 1.0] still allows freeze via automation -->
- Add logarithmic slider in `DrawWaveformControls` Geometry section (after Smooth): `ImGui::SliderFloat("Motion", &d->waveform.waveformMotionScale, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic)`
- Add `waveformMotionScale` to the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `WaveformData`
- Register `drawable.<id>.waveformMotionScale` in `DrawableParamsRegister` with range `[0.0f, 1.0f]` (waveform-specific section, gated on `d->type == DRAWABLE_WAVEFORM`)

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → slider appears, drag to 0.1 and observe slow morphing. Save/load preset preserves value. LFO routes to param.

**Done when**: Slider at 0.1 produces visible slow-motion waveform morphing; value round-trips through save/load.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1 | — |
| 2 | Phase 2 | Wave 1 |

---

## Implementation Notes

**Amplitude normalization** (post-plan fix): EMA on phase-shifting waveform data averages toward zero, squishing amplitude at low alpha. After the EMA pass, the smoothed buffer peak is rescaled to match the raw waveform peak each frame. Preserves slow-morphing shape without height loss.

**Logarithmic modulatable slider**: Added `ModulatableDrawableSliderLog` wrapper (`modulatable_drawable_slider.h/.cpp`) and optional `int flags` parameter to `ModulatableSlider`. Passes `ImGuiSliderFlags_Logarithmic` through to `ImGui::SliderFloat`. Motion slider uses this variant.

**All waveform params modulatable**: Converted `thickness` from `int` to `float`. Replaced all five geometry sliders (Radius, Height, Thickness, Smooth, Motion) with `ModulatableDrawableSlider` / `ModulatableDrawableSliderLog`. Registered in both `drawable_params.cpp` and `DRAWABLE_FIELD_TABLE` in `param_registry.cpp`.
