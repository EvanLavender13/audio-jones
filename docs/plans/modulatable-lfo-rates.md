# Modulatable LFO Rates

Make LFO rate parameters modulation targets, enabling LFO-to-LFO modulation for evolving patterns. Lower max frequency from 20 Hz to 5 Hz. Add waveform preview curves to LFO UI.

## Current State

- `src/config/lfo_config.h:13-17` - LFOConfig stores rate, enabled, waveform
- `src/ui/imgui_lfo.cpp:41` - Rate slider with 0.001–20 Hz range
- `src/automation/param_registry.cpp` - Static PARAM_TABLE for modulatable params
- `src/main.cpp:168-174` - LFO processing before ModEngineUpdate
- `src/automation/lfo.cpp:7-35` - Static `GenerateWaveform()` for waveform math
- `src/ui/modulatable_slider.cpp:21-78` - `DrawCurvePreview()` pattern for reference

## Phase 1: Register LFO Rate Parameters

**Goal**: Expose LFO rates to the modulation engine.

**Build**:

1. **Add LFO rate constant** to `src/ui/ui_units.h`:
   - `LFO_RATE_MAX = 5.0f` (Hz)

2. **Add param entries** to `src/automation/param_registry.cpp`:
   - Add to PARAM_TABLE: `{"lfo1.rate", {0.001f, LFO_RATE_MAX}}` through `lfo4.rate`
   - Add to targets array: pointers to `ctx->modLFOConfigs[0..3].rate`
   - Note: Registration happens via `ModEngineRegisterParam`, not `ParamRegistryInit` (configs live in AppContext, not EffectConfig)

3. **Register LFO params in main.cpp**:
   - After LFO state initialization (line ~94), call `ModEngineRegisterParam` for each LFO rate
   - Use param IDs: `"lfo1.rate"`, `"lfo2.rate"`, `"lfo3.rate"`, `"lfo4.rate"`

**Done when**: `ModEngineGetRoute("lfo1.rate", &route)` compiles and runs.

---

## Phase 2: Update LFO UI Panel

**Goal**: Replace static slider with modulatable slider, enforce 5 Hz max.

**Build**:

1. **Update imgui_lfo.cpp signature** to accept ModSources pointer:
   - Change `ImGuiDrawLFOPanel(LFOConfig* configs)` to `ImGuiDrawLFOPanel(LFOConfig* configs, const ModSources* sources)`

2. **Replace SliderFloat with ModulatableSlider**:
   - Include `ui/modulatable_slider.h`
   - Change rate slider from:
     ```cpp
     ImGui::SliderFloat(rateLabel, &configs[i].rate, 0.001f, 20.0f, "%.3f Hz", ImGuiSliderFlags_Logarithmic);
     ```
   - To:
     ```cpp
     char paramId[16];
     snprintf(paramId, sizeof(paramId), "lfo%d.rate", i + 1);
     ModulatableSlider(rateLabel, &configs[i].rate, paramId, "%.3f Hz", sources);
     ```
   - Note: ModulatableSlider reads min/max from registry, so range automatically becomes 0.001–5 Hz

3. **Update call site** in `src/main.cpp`:
   - Pass `&ctx->modSources` to `ImGuiDrawLFOPanel`

4. **Update header** `src/ui/imgui_panels.h`:
   - Forward-declare ModSources or include header
   - Update function signature

**Done when**: LFO rate sliders show modulation diamonds; clicking opens source selection popup.

---

## Phase 3: Add Smooth Random Waveform Type

**Goal**: Add a new LFO waveform that smoothly interpolates between random values.

**Build**:

1. **Add enum value** in `src/config/lfo_config.h`:
   - Add `LFO_WAVE_SMOOTH_RANDOM` before `LFO_WAVE_COUNT`

2. **Extend LFOState** in `src/automation/lfo.h`:
   - Add `float prevHeldValue` to track interpolation start point
   - Comment: "Previous random value for smooth random interpolation"

3. **Update LFOStateInit** in `src/automation/lfo.cpp`:
   - Initialize `prevHeldValue` to a random value (same as `heldValue`)

4. **Update phase wrap logic** in `LFOProcess`:
   - Before picking new `heldValue`, copy current to `prevHeldValue`
   - This shifts the interpolation window each cycle

5. **Add waveform case** in `GenerateWaveform`:
   ```cpp
   case LFO_WAVE_SMOOTH_RANDOM:
       // Linear interpolation from previous to current target
       return *prevHeldValue + (*heldValue - *prevHeldValue) * phase;
   ```
   - Update function signature to accept `prevHeldValue` pointer

6. **Update UI waveform names** in `src/ui/imgui_lfo.cpp`:
   - Add "Smooth Random" to `waveformNames[]` array

**Done when**: Selecting "Smooth Random" produces continuously drifting output that changes direction each cycle.

---

## Phase 4: Add Waveform Preview Curve

**Goal**: Show visual preview of selected LFO waveform shape.

**Build**:

1. **Expose waveform evaluation** in `src/automation/lfo.h`:
   - Add `float LFOEvaluateWaveform(int waveform, float phase)` declaration
   - In `lfo.cpp`, rename static `GenerateWaveform` to public `LFOEvaluateWaveform`
   - Update `LFOProcess` to call the renamed function

2. **Add preview drawing function** to `src/ui/imgui_lfo.cpp`:
   - Create `DrawLFOWaveformPreview(ImVec2 size, int waveform, ImU32 color)`
   - Follow `DrawCurvePreview` pattern from modulatable_slider.cpp:
     - Reserve space with `ImGui::Dummy(size)`
     - Draw background rect with `Theme::WIDGET_BG_BOTTOM`
     - Draw center line at y=0 (LFO output range is -1 to +1)
     - Sample 32 points across phase 0→1 using `LFOEvaluateWaveform`
     - Draw polyline with glow effect using LFO accent color

3. **Integrate preview into LFO section**:
   - After waveform combo, call `DrawLFOWaveformPreview`
   - Use `ImVec2(ImGui::GetContentRegionAvail().x, 40.0f)` for size
   - Pass `lfoAccentColors[i]` for the curve color

**Done when**: Each LFO section shows a preview curve matching the selected waveform type.

---

## Phase 5: Verify and Test

**Goal**: Confirm LFO-to-LFO modulation, new waveform, and preview work correctly.

**Build**:

1. **Test waveform preview**:
   - Cycle through all 6 waveform types (Sine, Triangle, Sawtooth, Square, Sample & Hold, Smooth Random)
   - Verify preview updates to show correct shape
   - Sample & Hold: stepped pattern or placeholder
   - Smooth Random: connected line segments (random slopes)

2. **Test Smooth Random behavior**:
   - Select Smooth Random waveform
   - Verify output drifts continuously without sudden jumps
   - Verify direction changes each cycle

3. **Test basic modulation**:
   - Set LFO1 to 0.5 Hz sine
   - Route LFO1 → LFO2.rate at +50%
   - Verify LFO2 rate oscillates between base and base+2.5 Hz

4. **Test self-modulation**:
   - Route LFO1 → LFO1.rate
   - Verify stable behavior (rate changes but doesn't cause runaway)

5. **Test preset save/load**:
   - Create preset with LFO rate modulation and Smooth Random waveform
   - Save, reload, verify routes and waveform type persist

**Done when**: All five test scenarios work as expected.
