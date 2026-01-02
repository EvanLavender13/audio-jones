# Modulation Easing Functions

Replace the current misnamed curves (EXP was quadratic, SQUARED was cubic) with a clean set of 7 easing functions. Includes dropdown UI with inline curve preview.

**New curve set:**
| Index | Name | Formula |
|-------|------|---------|
| 0 | Linear | `t` |
| 1 | Ease In | `t³` |
| 2 | Ease Out | `1-(1-t)³` |
| 3 | Ease In-Out | cubic blend |
| 4 | Spring | damped oscillation |
| 5 | Elastic | decaying sine |
| 6 | Bounce | parabolic bounces |

## Current State

- `src/automation/modulation_engine.h:7-11` - `ModCurve` enum with LINEAR, EXP, SQUARED (EXP/SQUARED misnamed and unused)
- `src/automation/modulation_engine.cpp:19-27` - `ApplyCurve()` switch statement
- `src/ui/modulatable_slider.cpp:246-255` - Radio buttons for curve selection
- `src/config/modulation_config.cpp:13` - Curves serialized as int in JSON

## Phase 1: Easing Math Header

**Goal**: Create header-only easing functions module.

**Build**:
- Create `src/automation/easing.h` with 6 inline easing functions:
  - `EaseInCubic(t)` - `t³`
  - `EaseOutCubic(t)` - `1 - (1-t)³`
  - `EaseInOutCubic(t)` - piecewise cubic blend
  - `EaseSpring(t)` - damped oscillation: `1 - cos(t*π*2.5) * e^(-6t)`
  - `EaseElastic(t)` - sine with exponential decay
  - `EaseBounce(t)` - piecewise parabolic bounces
- All functions take `float t` (0-1 range), return eased value
- Spring/elastic may return values > 1.0 (overshoot)

**Done when**: Header compiles, functions return expected values at t=0, t=0.5, t=1.

---

## Phase 2: Engine Integration

**Goal**: Replace ModCurve enum and wire easing functions into ApplyCurve().

**Build**:
- Replace `ModCurve` enum in `modulation_engine.h` (remove EXP/SQUARED):
  ```cpp
  typedef enum {
      MOD_CURVE_LINEAR = 0,
      MOD_CURVE_EASE_IN,
      MOD_CURVE_EASE_OUT,
      MOD_CURVE_EASE_IN_OUT,
      MOD_CURVE_SPRING,
      MOD_CURVE_ELASTIC,
      MOD_CURVE_BOUNCE,
      MOD_CURVE_COUNT
  } ModCurve;
  ```
- Add `#include "easing.h"` to `modulation_engine.cpp`
- Add `BipolarEase()` helper for LFO sources (-1 to +1):
  ```cpp
  static float BipolarEase(float x, float (*ease)(float)) {
      const float sign = (x >= 0.0f) ? 1.0f : -1.0f;
      return ease(fabsf(x)) * sign;
  }
  ```
- Replace `ApplyCurve()` switch: remove EXP/SQUARED cases, add new easing cases

**Done when**: Route with EASE_IN produces visibly different modulation than LINEAR.

---

## Phase 3: Curve Preview Widget

**Goal**: Add inline curve visualization for the modulation popup (Read temp.txt file).

**Build**:
- Add `DrawCurvePreview()` static function in `modulatable_slider.cpp`:
  - Size: 60x28 pixels
  - Background: `IM_COL32(15, 13, 23, 200)` with `Theme::WIDGET_BORDER` border
  - Sample curve at 24 points, draw as polyline
  - Curve color: source color from `ModSourceGetColor()`
  - For spring/elastic/bounce: expand Y range (yMin=-0.1, yMax=1.3) to show overshoot
  - Baseline markers at y=0 and y=1 for overshoot curves

**Done when**: Preview renders correctly for all 7 curve types, overshoot visible on spring/elastic.

---

## Phase 4: Dropdown UI

**Goal**: Replace radio buttons with dropdown + preview.

**Build**:
- Add curve name array in `modulatable_slider.cpp`:
  ```cpp
  static const char* curveNames[] = {
      "Linear", "Ease In", "Ease Out", "Ease In-Out",
      "Spring", "Elastic", "Bounce"
  };
  ```
- Replace radio button loop (lines 246-255) with `ImGui::Combo()`
- Add `ImGui::SameLine()` + `DrawCurvePreview()` after dropdown
- Wire selection to `route->curve` and call `ModEngineSetRoute()`

**Done when**: Dropdown shows all 7 curves, selection persists, preview updates on selection.

---

## Phase 5: Verification

**Goal**: Ensure visual correctness across all curve types.

**Build**:
- Test preset save/load with all curve values (0-6)
- Test each curve with LFO source (bipolar) and audio source (unipolar)
- Verify spring/elastic/bounce produce visible oscillation on modulated parameters
- Confirm LINEAR (index 0) still works for existing presets

**Done when**: All presets load correctly, all 7 curves produce distinct visual behavior.
