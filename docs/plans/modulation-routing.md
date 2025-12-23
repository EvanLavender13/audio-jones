# Modulation Routing System

A data-driven system for connecting audio analysis outputs (bass, mid, treb, beat, LFOs) to visual effect parameters without hardcoded wiring. Users create routes via UI; presets store routing configurations.

## Current State

Audio-reactive behavior is currently hardcoded in specific locations:
- `src/render/post_effect.cpp:377` - beat → blur scale
- `src/render/post_effect.cpp:432` - beat → chromatic aberration
- `src/render/post_effect.cpp:379-385` - single LFO → rotation
- `shaders/physarum_agents.glsl:122` - FFT → agent repulsion

Band energies (`bassSmooth`, `midSmooth`, `trebSmooth`) are computed in `src/analysis/bands.cpp` but only displayed in UI - not connected to visual parameters.

## Phase 1: Core Data Structures

**Goal**: Define modulation config types and source aggregation.

**Build**:
- Create `src/config/modulation_config.h`:
  - `ModSources` struct - runtime container for all source values (bass, mid, treb, smooth variants, beat, lfo1-4, time)
  - `ModTarget` enum - compile-time list of routable parameters
  - `ModCurve` enum - linear, exponential, squared
  - `ModRoute` struct - enabled, sourceIndex, target, amount, curve, waveformIndex
  - `ModulationConfig` struct - route array + count + 4 LFO configs

**Done when**: Header compiles and can be included from other files.

---

## Phase 2: Modulation Engine

**Goal**: Implement source gathering and route evaluation.

**Build**:
- Create `src/automation/modulation.h` - function declarations
- Create `src/automation/modulation.cpp`:
  - `ModulationUpdateSources()` - copy band energies, beat intensity, LFO outputs into ModSources
  - `ModulationApplyRoutes()` - iterate routes, apply curve, modify target via switch statement
  - `ApplyCurve()` helper - linear/exponential/squared transforms

**Initial targets** (10 most useful):
| Target Enum | Config Field | Range |
|-------------|--------------|-------|
| `MOD_TARGET_EFFECT_ZOOM` | `feedbackZoom` | 0.9-1.0 |
| `MOD_TARGET_EFFECT_ROTATION` | `feedbackRotation` | -0.05-0.05 |
| `MOD_TARGET_EFFECT_WARP_STRENGTH` | `warpStrength` | 0.0-1.0 |
| `MOD_TARGET_EFFECT_CHROMATIC` | `chromaticMaxOffset` | 0-30 |
| `MOD_TARGET_EFFECT_BLUR` | `baseBlurScale` | 0-4 |
| `MOD_TARGET_WAVEFORM_AMPLITUDE` | `amplitudeScale` | 0.0-2.0 |
| `MOD_TARGET_WAVEFORM_RADIUS` | `radius` | 0.0-1.0 |
| `MOD_TARGET_PHYSARUM_DEPOSIT` | `depositAmount` | 0.0-1.0 |
| `MOD_TARGET_PHYSARUM_STEP_SIZE` | `stepSize` | 0.0-5.0 |
| `MOD_TARGET_PHYSARUM_SENSOR_ANGLE` | `sensorAngle` | 0.0-90.0 |

**Done when**: Can manually create a route in code that modulates zoom from bass.

---

## Phase 3: Main Loop Integration

**Goal**: Wire modulation into the render pipeline.

**Build**:
- Modify `src/main.cpp`:
  - Add `ModulationConfig` and `ModSources` to `AppContext`
  - Add 4 `LFOState` instances for modulation LFOs
  - In render loop (before `PostEffectBeginAccum`):
    1. Process all 4 LFOs with deltaTime
    2. Call `ModulationUpdateSources()`
    3. Call `ModulationApplyRoutes()`
- Update `src/config/app_configs.h` - add modulation pointers

**Done when**: A hardcoded test route (bass → zoom) visibly affects the visualization.

---

## Phase 4: Modulation Panel UI

**Goal**: Let users create and edit routes visually.

**Build**:
- Create `src/ui/imgui_modulation.cpp`:
  - LFO configuration section (4 LFOs with rate/waveform controls)
  - Route list with add/remove buttons
  - Per-route row: enable checkbox, source dropdown, target dropdown, amount slider, curve dropdown
  - For waveform targets: waveform index selector
- Add declaration to `src/ui/imgui_panels.h`
- Register panel in main.cpp dockspace

**Source names**: Bass, Mid, Treb, Bass Smooth, Mid Smooth, Treb Smooth, Beat, LFO 1-4, Time

**Target names**: Effect: Zoom, Effect: Rotation, Effect: Warp, etc.

**Done when**: Can add/remove routes via UI and see them affect visuals in real-time.

---

## Phase 5: Preset Serialization

**Goal**: Save and load modulation routes with presets.

**Build**:
- Modify `src/config/preset.h`:
  - Add `ModulationConfig modulation` field to `Preset` struct
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `ModRoute`, `ModulationConfig`
  - Include modulation in `to_json`/`from_json` for `Preset`
- Modify `PresetFromAppConfigs`/`PresetToAppConfigs` to copy modulation config

**Done when**: Can save preset with routes, reload it, and routes still work.

---

## Phase 6: Parameter Indicators

**Goal**: Show which parameters are being modulated in existing panels.

**Build**:
- Add `IsParameterModulated()` helper function
- Modify `src/ui/imgui_effects.cpp`:
  - Before modulated sliders, draw `~` indicator in accent color
- Optionally add right-click context menu: "Add Modulation Route"

**Done when**: Modulated parameters are visually distinguished in Effects panel.

---

## Phase 7: Event Triggers (Optional Extension)

**Goal**: Add threshold-based event firing (beat → spawn waveform).

**Build**:
- Add to `modulation_config.h`:
  - `ModEventType` enum - SPAWN_WAVEFORM, RESET_PHYSARUM
  - `ModEventTrigger` struct - sourceIndex, threshold, hysteresis, eventType
  - Add trigger array to `ModulationConfig`
- Add `ModulationProcessEvents()` - returns bitmask of fired events
- In main.cpp: check returned events and call appropriate actions
- Extend UI panel with trigger editor section

**Done when**: Can configure "spawn waveform when beat > 0.8" via UI.

---

## Files Summary

**Create**:
- `src/config/modulation_config.h` (~80 lines)
- `src/automation/modulation.h` (~30 lines)
- `src/automation/modulation.cpp` (~200 lines)
- `src/ui/imgui_modulation.cpp` (~150 lines)

**Modify**:
- `src/main.cpp` - add fields, init, render loop calls
- `src/config/app_configs.h` - add modulation pointers
- `src/config/preset.h` - add modulation field
- `src/config/preset.cpp` - add JSON macros
- `src/ui/imgui_panels.h` - add panel declaration
- `CMakeLists.txt` - add new source files
