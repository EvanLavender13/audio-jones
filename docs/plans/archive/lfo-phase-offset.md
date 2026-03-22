# LFO Phase Offset

Per-LFO phase offset that shifts the waveform's starting position in its cycle. Enables quadrature relationships, staggered animation timing, and wider modulation patterns.

**Research**: `docs/research/lfo_phase_offset.md`

## Design

### Types

**LFOConfig** (modified, `src/config/lfo_config.h`):
```
struct LFOConfig {
  bool enabled = false;
  float rate = 0.1f;              // Oscillation frequency (Hz)
  int waveform = LFO_WAVE_SINE;
  float phaseOffset = 0.0f;       // Phase offset in radians (0.0-TWO_PI_F)
};
```

Add `LFO_CONFIG_FIELDS` macro:
```
#define LFO_CONFIG_FIELDS enabled, rate, waveform, phaseOffset
```

**Constants** (modified, `src/config/constants.h`):
```
#define LFO_PHASE_OFFSET_MAX TWO_PI_F // 360 deg in radians
```

### Algorithm

**LFOProcess** (`src/automation/lfo.cpp`): Apply offset at evaluation time only. The internal `state->phase` accumulator is unchanged -- offset shifts only the waveform read position:

```c
float evalPhase = state->phase + config->phaseOffset / TAU;
evalPhase -= floorf(evalPhase);  // wrap to [0, 1]
state->currentOutput = GenerateWaveform(
    config->waveform, evalPhase, &state->heldValue, &state->prevHeldValue);
```

`TAU` is the file-local `6.283185307f` constant already defined at the top of `lfo.cpp`.

**LFOEvaluateWaveform** (`src/automation/lfo.cpp`, `src/automation/lfo.h`): Add `float phaseOffset` parameter. Apply the same offset-and-wrap before evaluating:

```c
float LFOEvaluateWaveform(int waveform, float phase, float phaseOffset);
```

Inside the function, before the switch:
```c
phase += phaseOffset / TAU;
phase -= floorf(phase);  // wrap to [0, 1]
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| phaseOffset | float | 0.0-TWO_PI_F | 0.0 | Yes | Phase |

### Param Registration

Register alongside existing `"lfo<N>.rate"` params in `src/main.cpp`:
```
ModEngineRegisterParam("lfo1.phaseOffset", &ctx->modLFOConfigs[0].phaseOffset, 0.0f, LFO_PHASE_OFFSET_MAX);
// ... through lfo8
```

### UI

In `src/ui/imgui_lfo.cpp`, add a `ModulatableSliderAngleDeg` for phase offset after the rate slider. Label: `"Phase"`. Use `SliderAngleDeg` with range 0-360 degrees (0 to TWO_PI_F radians). Pass `configs[i].phaseOffset` to `LFOEvaluateWaveform` calls.

Format the paramId as `"lfo%d.phaseOffset"` matching the registration pattern.

---

## Tasks

### Wave 1: Config and Constants

#### Task 1.1: Add phaseOffset to LFOConfig and constants

**Files**: `src/config/lfo_config.h`, `src/config/constants.h`
**Creates**: `phaseOffset` field and `LFO_CONFIG_FIELDS` macro that all other tasks depend on; `LFO_PHASE_OFFSET_MAX` constant

**Do**:
- In `lfo_config.h`: add `float phaseOffset = 0.0f;` field with range comment `// Phase offset in radians (0.0-TWO_PI_F)`. Add `#define LFO_CONFIG_FIELDS enabled, rate, waveform, phaseOffset` after the struct.
- In `constants.h`: add `#define LFO_PHASE_OFFSET_MAX TWO_PI_F // 360 deg in radians` after the LFO rate bounds.
- `lfo_config.h` needs `#include "config/constants.h"` for the `LFO_CONFIG_FIELDS` macro to reference `TWO_PI_F` indirectly (it doesn't actually use it, but the field default is `0.0f` which needs no include). Actually, no new include needed -- the macro just lists field names.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Core Logic, UI, Serialization, Param Registration

#### Task 2.1: Apply phase offset in LFO processing and preview

**Files**: `src/automation/lfo.cpp`, `src/automation/lfo.h`
**Depends on**: Wave 1 complete

**Do**:
- In `lfo.h`: change `LFOEvaluateWaveform` signature to `float LFOEvaluateWaveform(int waveform, float phase, float phaseOffset)`.
- In `lfo.cpp` `LFOProcess`: replace the direct `GenerateWaveform` call with the offset-and-wrap pattern from the Algorithm section. Use `config->phaseOffset / TAU` to convert radians to normalized phase.
- In `lfo.cpp` `LFOEvaluateWaveform`: add the `float phaseOffset` parameter. Before the switch statement, apply `phase += phaseOffset / TAU; phase -= floorf(phase);`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Add phase offset UI slider

**Files**: `src/ui/imgui_lfo.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "ui/ui_units.h"` for `ModulatableSliderAngleDeg`.
- After the rate slider (line ~202), add a phase offset slider:
  - Format labels: `"Phase##lfo%d"` and paramId `"lfo%d.phaseOffset"` (using `i + 1`).
  - Use `ModulatableSliderAngleDeg(phaseLabel, &configs[i].phaseOffset, paramId, sources, "%.1f deg")` with 0-360 degree display.
- Update `LFOEvaluateWaveform` call in `DrawWaveformIcon` (line ~63) to pass `0.0f` as phaseOffset (icons show canonical waveform shape, not offset). <!-- Intentional deviation: research says preview should apply offset, but 24x24 icons identify waveform type, not phase. Live history preview already reflects offset via recorded LFO output. -->

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Update LFO serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Replace the inline field list in the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LFOConfig, enabled, rate, waveform)` call with `LFO_CONFIG_FIELDS` macro.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Register phaseOffset params

**Files**: `src/main.cpp`
**Depends on**: Wave 1 complete

**Do**:
- After the existing `ModEngineRegisterParam("lfo8.rate", ...)` block, add 8 registration calls for phaseOffset:
  ```
  ModEngineRegisterParam("lfo1.phaseOffset", &ctx->modLFOConfigs[0].phaseOffset, 0.0f, LFO_PHASE_OFFSET_MAX);
  ```
  Through `lfo8`. Follow the same pattern as the rate registrations.
- Add `#include "config/constants.h"` if not already present (for `LFO_PHASE_OFFSET_MAX`).

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Phase offset slider appears in LFO panel between rate and waveform selector
- [ ] Changing phase offset shifts waveform output without resetting the cycle
- [ ] Two LFOs at same rate with 90-degree offset produce quadrature outputs
- [ ] Preset save/load round-trips the phaseOffset field
- [ ] LFO preview waveform reflects the offset
- [ ] Waveform selector icons show canonical shapes (no offset applied)
