# Draw Frequency

Configurable interval controlling how often each drawable renders. Between draws, visuals persist via feedback buffer decay, creating strobing or pulsing effects.

## Current State

- `src/config/drawable_config.h:10-18` - `DrawableBase` struct with shared properties
- `src/render/drawable.h:14-19` - `DrawableState` with `globalTick` (20 Hz)
- `src/render/drawable.cpp:103-156` - `DrawableRenderCore` loop iterates and renders
- `src/ui/drawable_type_controls.cpp:16-21` - `DrawBaseAnimationControls` for shared UI
- `src/ui/ui_units.h` - Unit conversion slider helpers
- `src/config/preset.cpp:98` - `DrawableBase` serialization macro

## Phase 1: Data Structures

**Goal**: Add draw interval config field and timing state storage.

**Modify**:
- `src/config/drawable_config.h` - Add `uint8_t drawInterval = 0;` to `DrawableBase` after `feedbackPhase` (line 16). Range 0-100 ticks (0 = every frame, 100 = 5 seconds at 20 Hz).
- `src/render/drawable.h` - Add `uint64_t lastDrawTick[MAX_DRAWABLES];` to `DrawableState` after `globalTick` (line 17).
- `src/render/drawable.cpp` - `DrawableStateInit` already zeroes via `memset`, no change needed.

**Done when**: Project compiles with new fields.

---

## Phase 2: Render Loop Integration

**Goal**: Skip drawing when interval not elapsed.

**Modify**:
- `src/render/drawable.cpp` - In `DrawableRenderCore` loop (line 114), after enabled check and before opacity calculation:
  1. If `drawInterval > 0` and `(tick - state->lastDrawTick[i]) < drawInterval`, skip rendering (but still increment `waveformIndex` for waveforms)
  2. After rendering, update `state->lastDrawTick[i] = tick`
- Remove `const` from `DrawableState*` parameter in `DrawableRenderCore`, `DrawableRenderAll`, `DrawableRenderFull` signatures (both `.h` and `.cpp`)
- Update callers in `main.cpp` if needed (likely no change required)

**Done when**: Setting `drawInterval = 20` on a drawable causes it to render once per second.

---

## Phase 3: UI Slider

**Goal**: Add time-based slider for draw frequency.

**Modify**:
- `src/ui/ui_units.h` - Add `SliderDrawInterval` helper that displays seconds (0-5.0) but stores ticks (0-100). Show "Every frame" or "0 ms" when value is 0.
- `src/ui/drawable_type_controls.cpp` - Add slider in `DrawBaseAnimationControls` after the Feedback slider. Label: "Draw Freq" or "Redraw".

**Done when**: Slider appears in UI, adjusting it changes visual update rate.

---

## Phase 4: Serialization

**Goal**: Persist draw interval in presets.

**Modify**:
- `src/config/preset.cpp` - Add `drawInterval` to `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `DrawableBase` (line 98).

**Done when**: Presets save and load draw interval correctly. Old presets load with default value 0.

---

## Implementation Notes

**Timing math**:
- `globalTick` increments at 20 Hz (50ms per tick)
- `drawInterval = 0` → every frame (no skipping)
- `drawInterval = 1` → every tick (20 Hz, skip 2 of 3 frames at 60 FPS)
- `drawInterval = 20` → once per second
- `drawInterval = 100` → once per 5 seconds

**First-frame behavior**: `lastDrawTick` initializes to 0. On first frame with `tick = 0`, condition `(0 - 0) >= interval` is true for any non-negative interval, so first draw always happens.

**Waveform index tracking**: The existing pattern increments `waveformIndex` for all waveforms regardless of enabled state. Apply same pattern for interval-skipped waveforms to maintain buffer alignment.
