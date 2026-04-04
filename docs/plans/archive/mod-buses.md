# Modulation Buses

Virtual named buses that combine or process modulation sources, producing composite signals usable as modulation sources throughout the engine. 8 bus slots with 11 operator types: 7 combinators (2-input), 2 envelope processors (1-input), and 2 slew processors (1-input).

**Research**: `docs/research/mod_buses.md`, `docs/research/mod_envelopes.md`, `docs/research/mod_slew.md`

## Design

### Types

#### BusOp Enum (`src/config/mod_bus_config.h`)

```c
typedef enum {
  BUS_OP_ADD = 0,        // clamp(A + B, -1, 1)
  BUS_OP_MULTIPLY,       // A * B
  BUS_OP_MIN,            // min(A, B)
  BUS_OP_MAX,            // max(A, B)
  BUS_OP_GATE,           // A > 0 ? B : 0
  BUS_OP_CROSSFADE,      // lerp(A, B, 0.5)
  BUS_OP_DIFFERENCE,     // abs(A - B)
  BUS_OP_ENV_FOLLOW,     // Continuous envelope follower (1-input)
  BUS_OP_ENV_TRIGGER,    // Triggered one-shot envelope (1-input)
  BUS_OP_SLEW_EXP,       // Exponential lag (1-input)
  BUS_OP_SLEW_LINEAR,    // Linear slew rate clamp (1-input)
  BUS_OP_COUNT
} BusOp;
```

Inline helper predicates in the same header:

```c
static inline bool BusOpIsSingleInput(int op) {
  return op >= BUS_OP_ENV_FOLLOW;
}
static inline bool BusOpIsEnvelope(int op) {
  return op == BUS_OP_ENV_FOLLOW || op == BUS_OP_ENV_TRIGGER;
}
static inline bool BusOpIsSlew(int op) {
  return op == BUS_OP_SLEW_EXP || op == BUS_OP_SLEW_LINEAR;
}
```

#### ModBusConfig Struct (`src/config/mod_bus_config.h`)

```c
#define NUM_MOD_BUSES 8

struct ModBusConfig {
  bool enabled = false;
  char name[32] = {};
  int inputA = 0;            // ModSource index (MOD_SOURCE_BASS)
  int inputB = 4;            // ModSource index (MOD_SOURCE_LFO1)
  int op = BUS_OP_MULTIPLY;

  // Envelope params (ENV_FOLLOW, ENV_TRIGGER)
  float attack = 0.01f;     // 0.001-2.0 s
  float release = 0.3f;     // 0.01-5.0 s
  float hold = 0.0f;        // 0.0-2.0 s (ENV_TRIGGER only)
  float threshold = 0.3f;   // 0.0-1.0 (ENV_TRIGGER only)

  // Slew params (SLEW_EXP, SLEW_LINEAR)
  float lagTime = 0.2f;     // 0.01-5.0 s
  float riseTime = 0.2f;    // 0.01-5.0 s
  float fallTime = 0.2f;    // 0.01-5.0 s
  bool asymmetric = false;
};
```

No CONFIG_FIELDS macro. Requires manual JSON `to_json`/`from_json` due to `char name[32]` (same reason as `Preset::name` in `preset.cpp:95,111-113`).

#### BusEnvPhase Enum (`src/automation/mod_bus.h`)

```c
typedef enum {
  BUS_ENV_IDLE,
  BUS_ENV_ATTACK,
  BUS_ENV_HOLD,
  BUS_ENV_DECAY
} BusEnvPhase;
```

#### ModBusState Struct (`src/automation/mod_bus.h`)

```c
typedef struct ModBusState {
  float output;       // Current processed output (all modes)
  float prevInput;    // Previous frame input (trigger threshold detection)
  int envPhase;       // BusEnvPhase for triggered envelope state machine
  float holdTimer;    // Seconds remaining in hold phase
} ModBusState;
```

Not serialized. Resets on preset load.

#### ModSource Extension (`src/automation/mod_sources.h`)

Insert before `MOD_SOURCE_COUNT`:

```c
  MOD_SOURCE_BUS1,     // 18
  MOD_SOURCE_BUS2,     // 19
  MOD_SOURCE_BUS3,     // 20
  MOD_SOURCE_BUS4,     // 21
  MOD_SOURCE_BUS5,     // 22
  MOD_SOURCE_BUS6,     // 23
  MOD_SOURCE_BUS7,     // 24
  MOD_SOURCE_BUS8,     // 25
  MOD_SOURCE_COUNT     // 26 (was 18)
```

`ModSources::values[]` grows automatically since it is sized by `MOD_SOURCE_COUNT`.

### Algorithm

#### Processing Order

In the main frame loop, between existing steps:

```
ModSourcesUpdate()     // fills values[0..17] with audio + LFO outputs (existing)
ModBusEvaluate()       // fills values[18..25] with bus outputs (NEW)
ModEngineUpdate()      // applies routes to params (existing)
```

`ModBusEvaluate` loops i = 0..7, skips disabled buses, reads inputs from `sources->values[]`, applies the operator, writes result to `sources->values[MOD_SOURCE_BUS1 + i]`. Later buses can read earlier bus outputs from the current frame. Cycles produce a one-frame delay (the later bus reads previous frame's value from the earlier one).

#### Combinator Operators (2-input)

Read `A = sources->values[cfg.inputA]`, `B = sources->values[cfg.inputB]`:

| Op | Formula |
|----|---------|
| ADD | `fminf(fmaxf(A + B, -1.0f), 1.0f)` |
| MULTIPLY | `A * B` |
| MIN | `fminf(A, B)` |
| MAX | `fmaxf(A, B)` |
| GATE | `A > 0.0f ? B : 0.0f` |
| CROSSFADE | `A + (B - A) * 0.5f` |
| DIFFERENCE | `fabsf(A - B)` |

Combinators are stateless. Compute fresh each frame.

#### Envelope Follower (1-input, from `mod_envelopes.md`)

Per-frame update. Coefficients derived from `powf(0.01f, deltaTime / timeSec)` for frame-rate independence. Rectifies input with `fabsf`. Output is unipolar [0, 1].

```c
float v = fabsf(sources->values[cfg.inputA]);
float coef;
if (v > state->output) {
  coef = (cfg.attack > 0.0f) ? powf(0.01f, deltaTime / cfg.attack) : 0.0f;
} else {
  coef = (cfg.release > 0.0f) ? powf(0.01f, deltaTime / cfg.release) : 0.0f;
}
state->output = coef * (state->output - v) + v;
state->output = fminf(fmaxf(state->output, 0.0f), 1.0f);
```

A coefficient of 0 means instant response (time parameter is 0 or near-0).

#### Envelope Trigger (1-input, from `mod_envelopes.md`)

State machine with exponential segments. `targetRatio` is a constant 0.01f.

```
IDLE:
  output = 0
  If (input > threshold AND prevInput <= threshold) -> ATTACK, holdTimer = hold

ATTACK:
  coef = expf(-logf((1.0f + TR) / TR) * deltaTime / attack)
  base = (1.0f + TR) * (1.0f - coef)
  output = base + output * coef
  If output >= 1.0 -> HOLD (or DECAY if hold <= 0)

HOLD:
  output = 1.0
  holdTimer -= deltaTime
  If holdTimer <= 0 -> DECAY

DECAY:
  coef = expf(-logf((1.0f + TR) / TR) * deltaTime / release)
  base = -TR * (1.0f - coef)
  output = base + output * coef
  If output <= 0.001 -> IDLE, output = 0
```

Retrigger during DECAY: jump to ATTACK from current output value (no discontinuity), reset `holdTimer = hold`. Store `prevInput = fabsf(sources->values[cfg.inputA])` each frame (rectified so negative inputs can cross the positive threshold). Output is unipolar [0, 1].

#### Slew Exponential (1-input, from `mod_slew.md`)

One-pole lag. Preserves signal polarity (no rectification). Frame-rate independent via `expf(-speed * deltaTime)`.

```c
float v = sources->values[cfg.inputA];
float speed;
if (cfg.asymmetric) {
  speed = (v > state->output) ? (1.0f / cfg.riseTime) : (1.0f / cfg.fallTime);
} else {
  speed = 1.0f / cfg.lagTime;
}
float alpha = 1.0f - expf(-speed * deltaTime);
state->output += alpha * (v - state->output);
```

#### Slew Linear (1-input, from `mod_slew.md`)

Hard clamp on delta per frame. Rate = 2.0/time because full range [-1,+1] = 2 units.

```c
float v = sources->values[cfg.inputA];
float rate;
if (cfg.asymmetric) {
  rate = (v > state->output) ? (2.0f / cfg.riseTime) : (2.0f / cfg.fallTime);
} else {
  rate = 2.0f / cfg.lagTime;
}
float maxDelta = rate * deltaTime;
float delta = v - state->output;
if (delta > maxDelta) { delta = maxDelta; }
if (delta < -maxDelta) { delta = -maxDelta; }
state->output += delta;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | ParamId Pattern | UI Label | Format | Operators |
|-----------|------|-------|---------|-------------|-----------------|----------|--------|-----------|
| enabled | bool | - | false | No | - | (dot toggle) | - | All |
| name | char[32] | - | "" | No | - | (InputText) | - | All |
| inputA | int | 0-25 | 0 | No | - | "A" or "Input" | - | All |
| inputB | int | 0-25 | 4 | No | - | "B" | - | Combinators |
| op | int | 0-10 | 1 | No | - | (Combo) | - | All |
| attack | float | 0.001-2.0 | 0.01 | Yes | `busN.attack` | "Attack" | `"%.3f s"` | Envelope |
| release | float | 0.01-5.0 | 0.3 | Yes | `busN.release` | "Release" | `"%.2f s"` | Envelope |
| hold | float | 0.0-2.0 | 0.0 | Yes | `busN.hold` | "Hold" | `"%.2f s"` | ENV_TRIGGER |
| threshold | float | 0.0-1.0 | 0.3 | Yes | `busN.threshold` | "Threshold" | `"%.2f"` | ENV_TRIGGER |
| lagTime | float | 0.01-5.0 | 0.2 | Yes | `busN.lagTime` | "Lag Time" | `"%.2f s"` | Slew |
| riseTime | float | 0.01-5.0 | 0.2 | Yes | `busN.riseTime` | "Rise Time" | `"%.2f s"` | Slew (asym) |
| fallTime | float | 0.01-5.0 | 0.2 | Yes | `busN.fallTime` | "Fall Time" | `"%.2f s"` | Slew (asym) |
| asymmetric | bool | - | false | No | - | "Asymmetric" | - | Slew |

N is 1-indexed in param IDs: `bus1.attack`, `bus2.lagTime`, etc.

### UI Layout

#### Window

New docking tab "Buses" via `ImGui::Begin("Buses")`. 8 bus strips in a loop using `DrawModuleStripBegin/End`. Accent color via `Theme::GetSectionAccent(i)`. No `DrawGroupHeader`.

#### Per-Bus Strip

**Row 1** (after strip header): Name InputText + right-aligned operator Combo.
- Name: `ImGui::InputText`, minimal styling, max 31 chars. Shows "Bus N" as hint when empty. Hidden when disabled.
- Operator: `ImGui::BeginCombo/EndCombo` with 11 items. `ImGui::Separator()` before index 7 (envelope group) and before index 9 (slew group). Display names: "Add", "Multiply", "Min", "Max", "Gate", "Crossfade", "Difference", "Env Follow", "Env Trigger", "Slew (Exp)", "Slew (Linear)".

**Row 2**: Input source combos.
- Combinators: `A [combo]` + `SameLine` + `B [combo]`
- Processors (single-input): `Input [combo]` only (B hidden)
- Input combos use `BeginCombo/EndCombo` with all `MOD_SOURCE_COUNT` sources, grouped with separators and `TextDisabled` group labels between: Bands (0-2) | Spectral (3, 12) | Features (13-17) | LFOs (4-11) | Buses (18-25). Display semantic order, not enum order.

**Row 3** (processors only): Param section.
- Envelope: `SeparatorText("Envelope")` + Attack/Release sliders. ENV_TRIGGER adds Hold + Threshold.
- Slew: `SeparatorText("Slew")` + Lag Time slider. If asymmetric: Checkbox + Rise Time + Fall Time (Lag Time hidden).
- All float params use `ModulatableSlider` with paramIds `"busN.field"`.

**Row 4**: History graph + output meter on same line.
- History: 120-sample ring buffer (~2s at 60fps), rendered as scrolling polyline in ~(140, 32) region. Glow + sharp line at accent color. Background + center line (combinators) or no center line (processors). Current value dot at right edge.
- Meter: 4px wide vertical bar. Combinators: bipolar center-fill. Processors: unipolar bottom-fill. Color: accent.

**Disabled bus**: All text `ImGuiCol_TextDisabled`. Outline-only enable dot. Empty history graph. Name hidden. Meter fill skipped.

#### Mod Source Popup Extension

Add "Buses" category to `DrawModulationPopup` in `modulatable_slider.cpp`, between LFOs and the Amount slider separator:

```
ImGui::TextDisabled("Buses");
DrawSourceButtonRow(busSources1, 4, ...)  // MOD_SOURCE_BUS1-BUS4
DrawSourceButtonRow(busSources2, 4, ...)  // MOD_SOURCE_BUS5-BUS8
```

---

## Tasks

### Wave 1: Foundation Types

#### Task 1.1: Bus config header

**Files**: `src/config/mod_bus_config.h` (create)
**Creates**: `BusOp` enum, `ModBusConfig` struct, `NUM_MOD_BUSES`, helper predicates

**Do**: Create the header following `src/config/lfo_config.h` as structural pattern. Define everything from the Types section: BusOp enum with 11 operators + COUNT, ModBusConfig struct with all fields and defaults, NUM_MOD_BUSES = 8, inline helper predicates. Use `#ifndef MOD_BUS_CONFIG_H` include guard. Include `<stdbool.h>`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Extend ModSource enum

**Files**: `src/automation/mod_sources.h`, `src/automation/mod_sources.cpp`
**Creates**: `MOD_SOURCE_BUS1..BUS8` enum values, names, colors

**Do**:
- `mod_sources.h`: Add `#include "config/mod_bus_config.h"`. Insert `MOD_SOURCE_BUS1` through `MOD_SOURCE_BUS8` (values 18-25) before `MOD_SOURCE_COUNT` (now 26).
- `mod_sources.cpp` `ModSourceGetName()`: Add 8 cases returning `"Bus1"` through `"Bus8"`.
- `mod_sources.cpp` `ModSourceGetColor()`: Add case block for `MOD_SOURCE_BUS1..BUS8`. Return `Theme::GetSectionAccent(source - MOD_SOURCE_BUS1)` (cycles Cyan/Magenta/Orange matching bus strip accent colors).

**Verify**: Compiles.

---

### Wave 2: Implementation

#### Task 2.1: Bus processing engine

**Files**: `src/automation/mod_bus.h` (create), `src/automation/mod_bus.cpp` (create)
**Depends on**: Wave 1 complete

**Do**:
- `mod_bus.h`: Include `config/mod_bus_config.h`. Forward-declare `ModSources`. Define `BusEnvPhase` enum and `ModBusState` struct per Design. Declare:
  - `void ModBusStateInit(ModBusState *state)` - zero all fields
  - `void ModBusEvaluate(ModBusState states[], const ModBusConfig configs[], ModSources *sources, float deltaTime)` - evaluate all 8 buses
- `mod_bus.cpp`: Include `mod_bus.h`, `automation/mod_sources.h`, `<math.h>`. Implement `ModBusStateInit` and `ModBusEvaluate`. The evaluate function loops 0..NUM_MOD_BUSES-1, skips disabled, reads inputs from `sources->values[]`, applies operator per the Algorithm section, writes to `sources->values[MOD_SOURCE_BUS1 + i]`. Disabled buses write 0.0 to their output slot.
- Define `static const float TARGET_RATIO = 0.01f` for envelope trigger.
- Guard division by zero: if attack, release, lagTime, riseTime, or fallTime is <= 0, treat as instant (output = input or target immediately).

**Verify**: Compiles.

#### Task 2.2: Serialization and preset integration

**Files**: `src/config/preset.h`, `src/config/preset.cpp`, `src/config/app_configs.h`
**Depends on**: Wave 1 complete

**Do**:
- `preset.h`: Add `#include "mod_bus_config.h"`. Add `ModBusConfig modBuses[NUM_MOD_BUSES];` to `Preset` struct after `lfos[]`.
- `app_configs.h`: Add `#include "config/mod_bus_config.h"`. Add `ModBusConfig *modBuses;` to `AppConfigs` struct after `lfos`.
- `preset.cpp`: Write `static void to_json(json &j, const ModBusConfig &c)` and `static void from_json(const json &j, ModBusConfig &c)`. Handle `char name[32]` via `std::string` conversion (same pattern as `Preset::name` at lines 95 and 111-113). All other fields use `j["field"] = c.field` / `c.field = j.value("field", default)`.
- In `to_json(json&, const Preset&)`: Add `j["modBuses"]` array block after the lfos block (same pattern as lines 104-107).
- In `from_json(const json&, Preset&)`: Add `j.contains("modBuses")` array read block after the lfos block (same pattern as lines 127-132).
- In `PresetFromAppConfigs`: Add bus copy loop after the LFO copy (line 236 pattern).
- In `PresetToAppConfigs`: Add bus copy loop after the LFO copy (line 254-256 pattern), before `ModulationConfigToEngine` so bus param bases are captured.

**Verify**: Compiles.

#### Task 2.3: Main loop integration

**Files**: `src/main.cpp`, `CMakeLists.txt`
**Depends on**: Wave 1 complete

**Do**:
- `CMakeLists.txt`: Add `src/automation/mod_bus.cpp` to `AUTOMATION_SOURCES`. Add `src/ui/imgui_bus.cpp` to `IMGUI_UI_SOURCES`.
- `main.cpp` includes: Add `#include "automation/mod_bus.h"`.
- `AppContext` struct: Add `ModBusState modBusStates[NUM_MOD_BUSES];` and `ModBusConfig modBusConfigs[NUM_MOD_BUSES];`.
- Init section (after LFO init ~line 114): Initialize bus states with `ModBusStateInit(&ctx->modBusStates[i])` and configs with `ModBusConfig{}` in a loop.
- Param registration (after LFO phase offset registration ~line 150): Register 7 modulatable params per bus in a loop using `snprintf` for paramId generation. Format: `"bus%d.attack"`, `"bus%d.release"`, `"bus%d.hold"`, `"bus%d.threshold"`, `"bus%d.lagTime"`, `"bus%d.riseTime"`, `"bus%d.fallTime"` (N is i+1). Bounds match the Parameters table.
- Frame loop (between `ModSourcesUpdate` line 268 and `ModEngineUpdate` line 269): Insert `ModBusEvaluate(ctx->modBusStates, ctx->modBusConfigs, &ctx->modSources, deltaTime);`
- UI block (after `ImGuiDrawLFOPanel` line 325): Add `ImGuiDrawBusPanel(ctx->modBusConfigs, ctx->modBusStates, &ctx->modSources);`
- `AppConfigs` initializer (after `.lfos` line 304): Add `.modBuses = ctx->modBusConfigs`.

**Verify**: Compiles with all Wave 2 tasks.

#### Task 2.4: Bus panel UI

**Files**: `src/ui/imgui_bus.cpp` (create), `src/ui/imgui_panels.h`, `audiojones_layout.ini`
**Depends on**: Wave 1 complete

**Do**:
- `imgui_panels.h`: Add forward declarations `struct ModBusConfig;` and `struct ModBusState;` (after `LFOState` at line 19). Add `void ImGuiDrawBusPanel(ModBusConfig *configs, const ModBusState *states, const ModSources *sources);` after `ImGuiDrawLFOPanel` at line 79.
- `audiojones_layout.ini`: Add `[Window][Buses]` entry with same `DockId=0x00000002` as other panels. Place after the LFOs entry.
- `imgui_bus.cpp`: Follow `src/ui/imgui_lfo.cpp` as structural template. Includes: `automation/mod_bus.h`, `config/mod_bus_config.h`, `automation/mod_sources.h`, `imgui.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/theme.h`, `<stdio.h>`, `<string.h>`.

Implementation structure for `imgui_bus.cpp`:

**Static data**: `BUS_HISTORY_SIZE = 120`. Ring buffer `busHistory[NUM_MOD_BUSES][BUS_HISTORY_SIZE]`. Write index array. Operator name strings.

**`DrawBusHistoryPreview(ImVec2 size, int busIndex, bool enabled, bool bipolar, ImU32 accentColor)`**: Same rendering as `DrawLFOHistoryPreview` in imgui_lfo.cpp:69-123, but with 120 samples and size ~(140, 32). If bipolar: draw center line. If unipolar: no center line, map [0,1] to full height.

**`DrawBusOutputMeter(float value, bool enabled, bool bipolar, ImU32 accentColor, float height)`**: 4px wide. If bipolar: center-fill (same as LFO meter imgui_lfo.cpp:126-159). If unipolar: bottom-fill (fill rect from bottom up proportional to value).

**`DrawOperatorCombo(int *op, ImU32 accentColor)`**: `BeginCombo/EndCombo`. 11 items with `Separator()` before index 7 and 9. Right-align using `ImGui::SetNextItemWidth` and cursor positioning.

**`DrawSourceCombo(const char *label, int *source)`**: `BeginCombo/EndCombo`. Shows all `MOD_SOURCE_COUNT` sources in semantic groups with `TextDisabled` group headers and separators. Group order: Bands (0-2), Spectral (3, 12), Features (13-17), LFOs (4-11), Buses (18-25). Use `ImGui::Selectable` for each source with `ModSourceGetName` for display.

**`ImGuiDrawBusPanel(ModBusConfig*, const ModBusState*, const ModSources*)`**: Main panel function.
1. `ImGui::Begin("Buses")` with early return if collapsed.
2. Update history ring buffers from `states[i].output`.
3. Loop i = 0..7:
   - `accentColor = Theme::GetSectionAccent(i)`
   - `DrawModuleStripBegin("BUS N", accentColor, &configs[i].enabled)`
   - Row 1: Name InputText (hint "Bus N" when empty, hidden when disabled) + operator combo (right-aligned). Name uses `ImGui::InputText` with `ImGuiInputTextFlags_None`, max 31 chars.
   - Row 2: Input combos. If `BusOpIsSingleInput(op)`: one combo labeled "Input". Else: two combos labeled "A" and "B" on same line.
   - Row 3 (if envelope): `SeparatorText("Envelope")` + Attack slider + Release slider. If ENV_TRIGGER: + Hold slider + Threshold slider. All use `ModulatableSlider` with paramId `"busN.field"`.
   - Row 3 (if slew): `SeparatorText("Slew")`. If not asymmetric: Lag Time slider + Asymmetric checkbox. If asymmetric: Rise Time + Fall Time sliders + Asymmetric checkbox.
   - Row 4: `DrawBusHistoryPreview` + `SameLine` + `DrawBusOutputMeter`. Pass `bipolar = !BusOpIsSingleInput(op)`.
   - `DrawModuleStripEnd()`
4. `ImGui::End()`

**Verify**: Compiles with all Wave 2 tasks.

#### Task 2.5: Mod source popup extension

**Files**: `src/ui/modulatable_slider.cpp`
**Depends on**: Wave 1 complete

**Do**: In `DrawModulationPopup` (~line 284), add a "Buses" category after the LFOs section (after line 330, before the spacing/separator at line 332):

```c
static const ModSource busSources1[] = {MOD_SOURCE_BUS1, MOD_SOURCE_BUS2,
                                        MOD_SOURCE_BUS3, MOD_SOURCE_BUS4};
static const ModSource busSources2[] = {MOD_SOURCE_BUS5, MOD_SOURCE_BUS6,
                                        MOD_SOURCE_BUS7, MOD_SOURCE_BUS8};
ImGui::TextDisabled("Buses");
DrawSourceButtonRow(busSources1, 4, selectedSource, route, paramId, hasRoute,
                    sources, buttonWidth);
DrawSourceButtonRow(busSources2, 4, selectedSource, route, paramId, hasRoute,
                    sources, buttonWidth);
```

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Bus panel appears as a docking tab alongside LFOs, Effects, etc.
- [ ] 8 bus strips render with correct accent color cycling
- [ ] Operator combo shows all 11 operators with category separators
- [ ] Input combos show all 26 mod sources in semantic groups
- [ ] Enabling a combinator bus shows A + B inputs and history graph
- [ ] Enabling an envelope bus shows single Input + Envelope params section
- [ ] Enabling a slew bus shows single Input + Slew params section
- [ ] Asymmetric checkbox swaps Lag Time for Rise/Fall Time pair
- [ ] Bus outputs appear in mod source popup under "Buses" category
- [ ] Processor params (attack, release, etc.) show modulation diamonds
- [ ] Preset save/load round-trips bus configurations correctly
- [ ] Disabled buses render dimmed with empty history

---

## Implementation Notes

- Removed `char name[32]` from `ModBusConfig` post-implementation. The name was only shown in the strip InputText but never surfaced in mod source popups or combos, so it added no value. Removing it allowed switching from manual `to_json`/`from_json` to the standard `MOD_BUS_CONFIG_FIELDS` macro with `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT`.
- `mod_bus.cpp` switch on `state->envPhase` needed a `default: break` case to satisfy `bugprone-switch-missing-default-case` (the field is `int`, not the enum type).
- Bus param registration in `main.cpp` uses `snprintf` in a loop, each call suppressed with `NOLINTNEXTLINE(cert-err33-c)` matching the existing pattern in `imgui_analysis.cpp`.
