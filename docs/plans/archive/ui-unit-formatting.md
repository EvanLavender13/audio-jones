# UI Unit Formatting

Replace radian values with degrees throughout the UI and add descriptive unit suffixes (°, px, dB) for human-friendly parameter display. Internal storage remains in radians.

## Current State

All angle sliders display radians, requiring mental conversion:

- `src/ui/imgui_effects.cpp:29` - `feedbackRotation` shows "0.0087 rad"
- `src/ui/imgui_effects.cpp:68-69` - Physarum angles show "0.50 rad"
- `src/ui/imgui_waveforms.cpp:98-99` - Rotation speed/offset show radians
- `src/ui/imgui_spectrum.cpp:51-52` - Same pattern
- Pixel parameters (thickness, chroma) lack unit suffixes
- dB parameters have unit in label but not format string

---

## Phase 1: Create Constants Header

**Goal**: Define conversion constants in a reusable header.

**Create**:
- `src/ui/ui_units.h` - Header with `RAD_TO_DEG` (57.2957795131f) and `DEG_TO_RAD` (0.01745329251f) constants

**Done when**: Header exists and compiles cleanly.

---

## Phase 2: Update Effects Panel

**Goal**: Convert all radian parameters and add unit suffixes in Effects panel.

**Modify** `src/ui/imgui_effects.cpp`:
- Add include for `ui/ui_units.h`
- Line 27: Add `"%d px"` format to Chroma slider
- Line 29: Convert `feedbackRotation` to degrees with `"%.2f °/f"` format (range ±1.15°)
- Line 67: Add `"%.1f px"` format to Sensor Dist slider
- Line 68: Convert `sensorAngle` to degrees with `"%.1f °"` format (range 0-360°)
- Line 69: Convert `turningAngle` to degrees with `"%.1f °"` format (range 0-360°)
- Line 70: Add `"%.1f px"` format to Step Size slider

**Conversion pattern**:
```cpp
float sensorAngleDeg = e->physarum.sensorAngle * RAD_TO_DEG;
if (ImGui::SliderFloat("Sensor Angle", &sensorAngleDeg, 0.0f, 360.0f, "%.1f °")) {
    e->physarum.sensorAngle = sensorAngleDeg * DEG_TO_RAD;
}
```

**Done when**: All Effects panel angles show degrees, pixel params show "px".

---

## Phase 3: Update Waveforms Panel

**Goal**: Convert rotation parameters and add pixel suffixes in Waveforms panel.

**Modify** `src/ui/imgui_waveforms.cpp`:
- Add include for `ui/ui_units.h`
- Line 89: Add `"%d px"` format to Thickness slider
- Line 90: Add `"%.1f px"` format to Smooth slider
- Line 98: Convert `rotationSpeed` to degrees with `"%.2f °/f"` format (range ±2.87°)
- Line 99: Convert `rotationOffset` to degrees with `"%.1f °"` format (range 0-360°)

**Done when**: Waveform rotation shows degrees, thickness/smooth show "px".

---

## Phase 4: Update Spectrum Panel

**Goal**: Convert rotation parameters and add dB suffix in Spectrum panel.

**Modify** `src/ui/imgui_spectrum.cpp`:
- Add include for `ui/ui_units.h`
- Line 42: Add `"%.1f dB"` format to Min dB slider
- Line 43: Add `"%.1f dB"` format to Max dB slider
- Line 51: Convert `rotationSpeed` to degrees with `"%.2f °/f"` format (range ±2.87°)
- Line 52: Convert `rotationOffset` to degrees with `"%.1f °"` format (range 0-360°)

**Done when**: Spectrum rotation shows degrees, dB params show "dB" suffix.

---

## Phase 5: Verification

**Goal**: Ensure all conversions work correctly and presets remain compatible.

**Test**:
- Build and run AudioJones
- Verify all 7 angle parameters display degrees
- Verify all 6 pixel parameters show "px" suffix
- Verify 2 dB parameters show "dB" suffix
- Load existing preset - values should display correctly (radians auto-convert)
- Save new preset - verify JSON still contains radians
- Adjust sliders and confirm visual effects match expected behavior

**Done when**: All unit displays correct, presets load/save correctly.
