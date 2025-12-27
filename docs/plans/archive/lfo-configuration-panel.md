# LFO Configuration Panel

Add a standalone dockable ImGui window to configure LFO 1-4 per-preset. Each LFO gets enable toggle, rate slider (0.001-20 Hz logarithmic), and waveform dropdown.

## Current State

LFO infrastructure exists but lacks UI:
- `src/main.cpp:41-42` - `LFOState modLFOs[4]` and `LFOConfig modLFOConfigs[4]` in AppContext
- `src/main.cpp:192` - LFOs already processed in main loop via `LFOProcess()`
- `src/config/lfo_config.h` - `LFOConfig` struct with `enabled`, `rate`, `waveform`
- `src/automation/lfo.cpp` - 5 waveform types (sine, triangle, sawtooth, square, sample-hold)
- `src/ui/modulatable_slider.cpp:269` - LFO sources shown in modulation popup but not configurable

Files to modify:
- `src/ui/imgui_panels.h` - add panel declaration
- `src/config/preset.h` - add `lfos[4]` to Preset struct
- `src/config/preset.cpp` - add JSON serialization
- `src/config/app_configs.h` - add `lfos` pointer
- `src/main.cpp` - add panel call and AppConfigs init

---

## Phase 1: UI Panel Implementation

**Goal**: Create dockable "LFOs" window with 4 collapsible sections.

**Build**:
- Create `src/ui/imgui_lfo.cpp`:
  - Static bool array for section states
  - `ImGuiDrawLFOPanel(LFOConfig* configs)` function
  - 4 sections using `DrawSectionBegin/End` with cycling accent colors (LFO1=cyan, LFO2=magenta, LFO3=orange, LFO4=cyan)
  - Per-LFO widgets:
    - `ImGui::Checkbox("Enabled##lfoN", &configs[i].enabled)`
    - `ImGui::SliderFloat("Rate##lfoN", &configs[i].rate, 0.001f, 20.0f, "%.3f Hz", ImGuiSliderFlags_Logarithmic)`
    - `ImGui::Combo("Waveform##lfoN", &configs[i].waveform, waveformNames, 5)`
  - Waveform names array: `{"Sine", "Triangle", "Sawtooth", "Square", "Sample & Hold"}`
- `src/ui/imgui_panels.h`:
  - Line 15: Add `struct LFOConfig;` forward declaration
  - Line 58: Add `void ImGuiDrawLFOPanel(LFOConfig* configs);`
- `src/main.cpp`:
  - After line 279: Add `ImGuiDrawLFOPanel(ctx->modLFOConfigs);`

**Done when**: Run app, Tab to show UI, "LFOs" window appears with 4 sections and functional controls.

---

## Phase 2: AppConfigs Integration

**Goal**: Wire LFO configs through AppConfigs for preset panel access.

**Build**:
- `src/config/app_configs.h`:
  - Add include: `#include "config/lfo_config.h"`
  - Add field to AppConfigs struct: `LFOConfig* lfos;`
- `src/main.cpp`:
  - Around line 268 in AppConfigs initialization: Add `.lfos = ctx->modLFOConfigs,`

**Done when**: Code compiles, preset panel has access to LFO configs via `configs->lfos`.

---

## Phase 3: Preset Struct Extension

**Goal**: Add LFO configs to Preset struct for serialization.

**Build**:
- `src/config/preset.h`:
  - Add include: `#include "lfo_config.h"`
  - Add field to Preset struct after `modulation`: `LFOConfig lfos[4];`

**Done when**: Preset struct contains LFO storage, ready for JSON serialization.

---

## Phase 4: JSON Serialization

**Goal**: Save and load LFO configs with presets.

**Build**:
- `src/config/preset.cpp`:
  - After line 14: Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LFOConfig, enabled, rate, waveform)`
  - In `to_json()` after modulation (line 100 area):
    ```cpp
    j["lfos"] = json::array();
    for (int i = 0; i < 4; i++) {
        j["lfos"].push_back(p.lfos[i]);
    }
    ```
  - In `from_json()` after modulation (line 121 area):
    ```cpp
    if (j.contains("lfos")) {
        const auto& arr = j["lfos"];
        for (int i = 0; i < 4 && i < (int)arr.size(); i++) {
            p.lfos[i] = arr[i].get<LFOConfig>();
        }
    }
    ```
  - In `PresetFromAppConfigs()` (line 198 area):
    ```cpp
    for (int i = 0; i < 4; i++) {
        preset->lfos[i] = configs->lfos[i];
    }
    ```
  - In `PresetToAppConfigs()` (line 209 area):
    ```cpp
    for (int i = 0; i < 4; i++) {
        configs->lfos[i] = preset->lfos[i];
    }
    ```

**Done when**: Configure LFOs, save preset, load preset - settings restored. Old presets load with LFOs disabled (defaults).

---

## Phase 5: Verification

**Goal**: Validate complete implementation.

**Build**:
- Test each waveform type displays correctly in dropdown
- Test rate slider logarithmic scaling (0.001 Hz to 20 Hz)
- Test enable/disable toggle affects modulation output
- Test preset save/load preserves all LFO settings
- Test backward compatibility: load old preset without "lfos" field
- Test UI theme consistency with Effects panel

**Done when**: All LFO controls functional, presets persist settings, old presets load safely.
