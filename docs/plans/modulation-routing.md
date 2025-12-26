# Modulation Routing System

Connect audio analysis outputs (bass, mid, treb, beat) and LFOs to any visual parameter via a generic routing engine. Drop-in `ModulatableSlider` widget replaces standard sliders, showing ghost handle at modulated value and popup for route configuration.

## Current State

Hardcoded modulation in physarum shader (to be removed):
- `src/render/physarum.h:38-40` - `frequencyModulation`, `stepBeatModulation`, `sensorBeatModulation` fields
- `src/ui/imgui_effects.cpp:82-84` - corresponding sliders
- `shaders/physarum_agents.glsl:122,161,195` - modulation math in shader

Band normalization (sensitivity removed for simplicity):
- `src/ui/imgui_analysis.cpp` - `normalized = smooth/avg`, `fillRatio = normalized/2.0` clamped to 0-1
- Output: 0-1 where 1.0 = 2x the running average (loud transient)

Existing widget patterns:
- `src/ui/imgui_widgets.cpp` - `HueRangeSlider`, `GradientEditor` with `ImGuiStorage` for state
- `src/ui/theme.h:80-97` - `DrawInteractiveHandle()` for styled handles

---

## Phase 1: Modulation Sources [COMPLETE]

**Goal**: Gather all modulation source values into a single struct, normalized 0-1 matching band bar display.

**Created**:

`src/automation/mod_sources.h`:
```cpp
typedef enum {
    MOD_SOURCE_BASS = 0,
    MOD_SOURCE_MID,
    MOD_SOURCE_TREB,
    MOD_SOURCE_BEAT,
    MOD_SOURCE_LFO1,
    MOD_SOURCE_LFO2,
    MOD_SOURCE_LFO3,
    MOD_SOURCE_LFO4,
    MOD_SOURCE_COUNT
} ModSource;

typedef struct {
    float values[MOD_SOURCE_COUNT];
} ModSources;

void ModSourcesInit(ModSources* sources);
void ModSourcesUpdate(ModSources* sources, const BandEnergies* bands,
                      const BeatDetector* beat, const float lfoOutputs[4]);
const char* ModSourceGetName(ModSource source);
ImU32 ModSourceGetColor(ModSource source);
```

`src/automation/mod_sources.cpp`:
- Self-calibrating normalization (no sensitivity parameter):
  ```cpp
  const float MIN_AVG = 1e-6f;
  float norm = bands->bassSmooth / fmaxf(bands->bassAvg, MIN_AVG);
  sources->values[MOD_SOURCE_BASS] = fminf(norm / 2.0f, 1.0f);
  ```
- LFO outputs remapped from -1..1 to 0..1: `(lfo + 1.0f) * 0.5f`
- Beat uses `beat->beatIntensity` directly (already 0-1)
- `ModSourceGetColor` returns theme colors: bass=CYAN, mid=WHITE, treb=MAGENTA, beat=ORANGE, LFOs=interpolated cyan-magenta

---

## Phase 2: Modulation Engine [COMPLETE]

**Goal**: Central registry for parameter pointers and routes, evaluates modulation each frame.

**Created**:

`src/automation/modulation_engine.h`:
```cpp
typedef enum {
    MOD_CURVE_LINEAR = 0,
    MOD_CURVE_EXP,
    MOD_CURVE_SQUARED
} ModCurve;

typedef struct {
    char paramId[64];
    int source;      // ModSource enum
    float amount;    // -1.0 to +1.0, multiplied by (max-min)
    int curve;       // ModCurve enum
} ModRoute;

void ModEngineInit(void);
void ModEngineUninit(void);

void ModEngineRegisterParam(const char* paramId, float* ptr, float min, float max);
void ModEngineSetRoute(const char* paramId, const ModRoute* route);
void ModEngineRemoveRoute(const char* paramId);
bool ModEngineHasRoute(const char* paramId);
bool ModEngineGetRoute(const char* paramId, ModRoute* outRoute);

void ModEngineUpdate(float dt, const ModSources* sources);

float ModEngineGetOffset(const char* paramId);
void ModEngineSetBase(const char* paramId, float base);
```

`src/automation/modulation_engine.cpp`:
- Static `std::unordered_map<std::string, ParamMeta>` for registered params (ptr, min, max, base)
- Static `std::unordered_map<std::string, ModRoute>` for routes
- Static `std::unordered_map<std::string, float>` for current offsets
- `ModEngineUpdate`: iterate routes, compute offset = ApplyCurve(source) * amount * (max-min), write base+offset to ptr clamped
- `ApplyCurve`: linear=x, exp=x², squared=x³

---

## Phase 3: Main Loop Integration [COMPLETE]

**Goal**: Wire modulation into render pipeline before UI draw.

**Modified** `src/main.cpp`:
- Added to AppContext: `ModSources modSources`, `LFOState modLFOs[4]`, `LFOConfig modLFOConfigs[4]`
- In `AppContextInit`: call `ModEngineInit()`, `ModSourcesInit()`, init 4 LFOs
- In `AppContextUninit`: call `ModEngineUninit()`
- In render loop after audio analysis:
  ```cpp
  float lfoOutputs[4];
  for (int i = 0; i < 4; i++) {
      lfoOutputs[i] = LFOProcess(&ctx->modLFOs[i], &ctx->modLFOConfigs[i], deltaTime);
  }
  ModSourcesUpdate(&ctx->modSources, &ctx->analysis.bands,
                   &ctx->analysis.beat, lfoOutputs);
  ModEngineUpdate(deltaTime, &ctx->modSources);
  ```

**Updated** `CMakeLists.txt`: added `mod_sources.cpp`, `modulation_engine.cpp`

---

## Phase 4: ModulatableSlider Widget

**Goal**: Drop-in slider replacement with ghost handle and modulation popup.

**Build**:

Create `src/ui/modulatable_slider.h` and `src/ui/modulatable_slider.cpp` (own file like gradient_editor):

```cpp
bool ModulatableSlider(const char* label, float* value, const char* paramId,
                       float min, float max, const char* format,
                       const ModSources* sources);
```

Widget behavior:
1. First draw for this paramId: call `ModEngineRegisterParam(paramId, value, min, max)`
2. Call `ImGui::SliderFloat` normally
3. Get slider frame rect via `ImGui::GetItemRectMin/Max` (this is the TRACK, not the label)
4. If `ModEngineHasRoute(paramId)`:
   - Get route via `ModEngineGetRoute`
   - Get offset via `ModEngineGetOffset`
   - Calculate ghost position: `((*value + offset - min) / (max - min))` clamped to 0-1
   - Draw track highlight between base and ghost (10% alpha source color)
   - Draw ghost handle using `DrawInteractiveHandle` (40% alpha, source color)
5. Draw indicator after slider: `ImGui::SameLine()`, then diamond button
   - Inactive: hollow diamond, `Theme::TEXT_SECONDARY_U32`
   - Active: filled diamond, source color, pulse alpha 0.7-1.0 at 800ms period
6. If active: draw source badge `[bass]` after indicator
7. Indicator click opens popup (unique ID per paramId)

Popup contents (match gradient editor simplicity):
- Title: parameter name (extract from paramId or pass separately)
- Source grid: 2 rows x 4 cols buttons (Bass/Mid/Treb/Beat, LFO1-4)
- Selected source has colored border
- Amount slider: -100% to +100%, center at 0
- Curve radio: Linear / Exp / Squared
- Remove button (only if route exists)

**Done when**: Widget shows ghost handle tracking source, popup configures route.

---

## Phase 5: Replace Physarum Sliders

**Goal**: Convert 4 physarum sliders to ModulatableSlider, remove 3 hardcoded ones.

**Build**:

Modify `src/ui/imgui_effects.cpp`:
- Add `const ModSources* sources` parameter to `ImGuiDrawEffectsPanel` (or access via global/AppConfigs)
- Replace lines 68-71:
  ```cpp
  ModulatableSlider("Sensor Dist", &e->physarum.sensorDistance,
                    "physarum.sensorDistance", 1.0f, 100.0f, "%.1f px", sources);
  ModulatableSlider("Sensor Angle", &e->physarum.sensorAngle,
                    "physarum.sensorAngle", 0.0f, 6.28f, "%.2f", sources);
  ModulatableSlider("Turn Angle", &e->physarum.turningAngle,
                    "physarum.turningAngle", 0.0f, 6.28f, "%.2f", sources);
  ModulatableSlider("Step Size", &e->physarum.stepSize,
                    "physarum.stepSize", 0.1f, 100.0f, "%.1f px", sources);
  ```
- Delete lines 82-84 (Freq Mod, Step Beat, Sensor Beat sliders)

Modify `src/render/physarum.h`:
- Delete `frequencyModulation`, `stepBeatModulation`, `sensorBeatModulation` fields

Modify `src/render/physarum.cpp`:
- Delete uniform locations and `rlSetUniform` calls for removed fields

Modify `shaders/physarum_agents.glsl`:
- Delete uniform declarations for removed fields
- Delete modulation math at lines ~122, 161, 195
- Use raw `sensorDistance`, `stepSize` values (already modulated by engine)

Modify `src/config/preset.cpp`:
- Remove 3 deleted fields from `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT`

**Done when**: 4 physarum sliders have modulation UI, 3 old sliders gone, shader simplified.

---

## Phase 6: Preset Serialization [COMPLETE]

**Goal**: Save and load modulation routes with presets.

**Build**:

Create `src/config/modulation_config.h`:
```cpp
#define MAX_MOD_ROUTES 64

struct ModulationConfig {
    ModRoute routes[MAX_MOD_ROUTES];
    int count = 0;
};

void ModulationConfigFromEngine(ModulationConfig* config);
void ModulationConfigToEngine(const ModulationConfig* config);
```

Create `src/config/modulation_config.cpp`:
- JSON serialization for ModRoute and ModulationConfig
- `FromEngine`: iterate engine's route map, copy to array
- `ToEngine`: clear engine routes, add from array

Modify `src/config/preset.h`:
- Add `ModulationConfig modulation;` field to Preset

Modify `src/config/preset.cpp`:
- Add `modulation` to Preset JSON macro
- In `PresetFromAppConfigs`: call `ModulationConfigFromEngine`
- In `PresetToAppConfigs`: call `ModulationConfigToEngine`

**Done when**: Routes survive preset save/load cycle.

---

## Phase 7: Polish

**Goal**: Visual refinement, edge cases, quality.

**Build**:

Source colors (add to `src/ui/theme.h` if missing):
- Bass: `Theme::BAND_CYAN_U32`
- Mid: `Theme::BAND_WHITE_U32`
- Treb: `Theme::BAND_MAGENTA_U32`
- Beat: add `ACCENT_ORANGE_U32 = IM_COL32(255, 115, 13, 255)`
- LFOs: add `ModSourceGetColor` that interpolates cyan-magenta by index

Animation timing:
- Indicator pulse: 800ms sine, alpha 0.7-1.0
- Ghost handle: NO interpolation, updates every frame (direct audio response)

Edge cases:
- User drags slider while modulated: call `ModEngineSetBase(paramId, newValue)`
- Modulated value exceeds range: clamp in engine AND clamp ghost handle to track bounds
- Preset loads before widgets drawn: engine stores routes, widgets register on first draw
- Old presets: missing modulation field defaults to empty (no routes)

Tooltips:
- Inactive indicator: "Click to add modulation"
- Active indicator: "[Source] -> [Amount]%"

**Done when**: UI feels polished, ghost handles exactly match band bars, no crashes.

---

## Files Summary

**Created:**
- `src/automation/mod_sources.h` (~30 lines) [DONE]
- `src/automation/mod_sources.cpp` (~70 lines) [DONE]
- `src/automation/modulation_engine.h` (~35 lines) [DONE]
- `src/automation/modulation_engine.cpp` (~130 lines) [DONE]
- `src/ui/modulatable_slider.h` (~20 lines)
- `src/ui/modulatable_slider.cpp` (~300 lines)
- `src/config/modulation_config.h` (~20 lines) [DONE]
- `src/config/modulation_config.cpp` (~80 lines) [DONE]

**Modified:**
- `src/main.cpp` - add modulation update loop [DONE]
- `CMakeLists.txt` - add source files [DONE]
- `src/ui/imgui_effects.cpp` - replace 4 sliders, delete 3
- `src/ui/theme.h` - add ACCENT_ORANGE_U32 if missing
- `src/render/physarum.h` - delete 3 fields
- `src/render/physarum.cpp` - delete uniform handling
- `shaders/physarum_agents.glsl` - remove modulation math
- `src/config/preset.h` - add ModulationConfig field [DONE]
- `src/config/preset.cpp` - add serialization [DONE]

**Removed (sensitivity cleanup):**
- `src/config/band_config.h` - emptied (sensitivity fields removed)
- `src/config/app_configs.h` - removed `BandConfig* bands` field
- `src/config/preset.h` - removed `BandConfig bands` field
- `src/ui/imgui_analysis.cpp` - removed sensitivity sliders UI
