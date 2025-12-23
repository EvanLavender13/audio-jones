# Circular Mode to Effect

Move the linear/circular toggle from a special-case application mode to a standard effect parameter, making it persistent in presets and consistent with other effects like kaleidoscope.

## Current State

- `src/main.cpp:19-22` - `WaveformMode` enum defines LINEAR/CIRCULAR
- `src/main.cpp:36` - `mode` field in `AppContext`
- `src/main.cpp:80` - Default initialization to `WAVEFORM_LINEAR`
- `src/main.cpp:161-163` - SPACE key toggles mode
- `src/main.cpp:112` - `RenderWaveforms()` converts mode to boolean
- `src/config/effect_config.h` - Effect parameters (no circular field)
- `src/config/preset.cpp:80-85` - EffectConfig JSON serialization
- `src/ui/imgui_effects.cpp:20-30` - Core Effects UI section

## Phase 1: Add Circular to EffectConfig

**Goal**: Make circular a proper effect parameter that persists in presets.

**Build**:
- Add `bool circular = false;` to `EffectConfig` struct in `effect_config.h`
- Add `circular` to the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro in `preset.cpp`

**Done when**: Circular field exists in EffectConfig and is serialized to JSON.

---

## Phase 2: Update Rendering Pipeline

**Goal**: Use the new effect parameter instead of the application mode.

**Build**:
- Remove `WaveformMode` enum from `main.cpp`
- Remove `mode` field from `AppContext` struct
- Remove mode initialization (`ctx->mode = WAVEFORM_LINEAR`)
- Remove SPACE key toggle handler
- Update `RenderWaveforms()` to read `ctx->postEffect->effects.circular` instead of comparing `ctx->mode`

**Done when**: Rendering uses `effects.circular` and old mode code is removed.

---

## Phase 3: Add UI Control

**Goal**: Expose circular toggle in the Effects panel.

**Build**:
- Add `ImGui::Checkbox("Circular", &e->circular);` to the Core Effects section in `imgui_effects.cpp`
- Place it at the top of Core Effects (first visual control, before Blur)

**Done when**: Circular toggle appears in Effects panel and controls waveform/spectrum rendering.

---

## Phase 4: Verify Preset Compatibility

**Goal**: Ensure existing presets work correctly.

**Build**:
- Load existing presets and verify they default to linear (circular = false)
- Save a preset with circular enabled and verify JSON contains the field
- Reload and confirm circular state persists

**Done when**: Presets save/load circular state correctly, existing presets default to linear.
