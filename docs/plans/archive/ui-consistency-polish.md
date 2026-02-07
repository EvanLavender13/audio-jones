# UI Consistency & Polish

Fix inconsistencies across the effects UI: mismatched category badges, non-uniform separator styles, missing modulation on spectrum drawables, stale header declarations, and speed suffix display. Add small polish: drawable color swatches in the list.

## Design

### Category Badge Fixes

Update `GetTransformCategory()` in `imgui_effects.cpp` to match where each effect's UI actually lives:

| Effect | Current Badge | Current UI Category | New Badge/Section |
|--------|---------------|--------------------|--------------------|
| Disco Ball | CELL (2) | Graphic | GFX (5) |
| Radial Pulse | SYM (0) | Warp | WARP (1) |
| Synthwave | GFX (5) | Retro | RET (6) |
| Lego Bricks | RET (6) | Graphic | GFX (5) |
| Circuit Board | ??? (0) | Warp | WARP (1) |

### Separator Style Standardization

Three styles currently coexist:
- **Simulations** (`imgui_effects.cpp`): `ImGui::SeparatorText("Label")`
- **Generators** (`imgui_effects_generators.cpp`): `Spacing()+Separator()+Spacing()+TextColored(disabled, "Label")+Spacing()`
- **Transforms**: `TreeNodeAccented()` or flat (no sub-separators)

**Decision**: Adopt `SeparatorText()` as the standard for parameter sub-groups within flat (non-collapsible) sections. Generators currently use the 5-line `Separator+TextColored` pattern in 15+ places — replace each with a single `ImGui::SeparatorText()` call. `TreeNodeAccented()` remains appropriate for optional/advanced sub-groups that benefit from collapse.

### Stale Header Cleanup

`imgui_effects_transforms.h` declares 6 functions but 4 categories now have dedicated headers (`imgui_effects_artistic.h`, `imgui_effects_graphic.h`, `imgui_effects_optical.h`, `imgui_effects_retro.h`). The stale header still declares `DrawStyleCategory()` which doesn't exist. Clean up:
- Remove `DrawStyleCategory` (doesn't exist)
- Remove declarations already in dedicated headers: `DrawSymmetryCategory`, `DrawWarpCategory`, `DrawCellularCategory`, `DrawMotionCategory`, `DrawColorCategory`
- Either delete the file entirely or keep it with only the declarations not covered by other headers

Since `imgui_effects.cpp:11` includes `imgui_effects_transforms.h` and also includes the 4 dedicated headers, the transforms header only needs to declare functions NOT in other headers. Check which categories lack their own header:
- Symmetry: no dedicated header (uses transforms.h)
- Warp: no dedicated header (uses transforms.h)
- Cellular: no dedicated header (uses transforms.h)
- Motion: no dedicated header (uses transforms.h)
- Color: no dedicated header (uses transforms.h)
- Artistic: has `imgui_effects_artistic.h`
- Graphic: has `imgui_effects_graphic.h`
- Optical: has `imgui_effects_optical.h`
- Retro: has `imgui_effects_retro.h`

**Decision**: Keep `imgui_effects_transforms.h` with only the 5 declarations that lack dedicated headers. Remove `DrawStyleCategory`.

### Spectrum Modulation Parity

Spectrum geometry params use plain `ImGui::SliderFloat` while waveform equivalents are modulatable. Fix requires:

1. **Register params** in `drawable_params.cpp` under `DRAWABLE_SPECTRUM`:
   - `innerRadius`: bounds 0.05–0.4
   - `barHeight`: bounds 0.1–0.5
   - `barWidth`: bounds 0.3–1.0
   - `smoothing`: bounds 0.0–0.95

2. **Update UI** in `drawable_type_controls.cpp` `DrawSpectrumControls()`:
   - Replace `ImGui::SliderFloat("Radius", ...)` with `ModulatableDrawableSlider("Radius", ...)`
   - Replace `ImGui::SliderFloat("Height", ...)` with `ModulatableDrawableSlider("Height", ...)`
   - Replace `ImGui::SliderFloat("Width", ...)` with `ModulatableDrawableSlider("Width", ...)`
   - Replace `ImGui::SliderFloat("Smooth", ...)` with `ModulatableDrawableSlider("Smooth", ...)`

3. **Register in param table** in `param_registry.cpp` under `DRAWABLE_FIELD_TABLE`.

### Speed Suffix Convention

`docs/conventions.md` states: Speed sliders show "/s" suffix. The default format for `ModulatableSliderAngleDeg` is `"%.1f °"`. Speed parameters need `"%.1f °/s"`.

Affected speed parameters using the default `"%.1f °"` format (no explicit "/s"):
- All `ModulatableSliderAngleDeg` calls where the underlying field is a `*Speed`, `*speed`, `Spin*`, or `Twist*` (rotation rate) — these store radians/second but display without the "/s" suffix.

**Decision**: The `ModulatableSliderAngleDeg` function defaults to `"%.1f °"`. Rather than changing every call site, add a new convenience wrapper `ModulatableSliderSpeedDeg()` in `ui_units.h` that defaults to `"%.1f °/s"`. Then update call sites that represent rotation rates to use the new wrapper. Call sites that pass an explicit `"%.1f °/s"` format already (e.g., Disco Ball, Corridor Warp) just switch to the new wrapper and drop the format arg.

New wrapper in `ui_units.h`:
```
ModulatableSliderSpeedDeg(label, radians, paramId, sources)
```
Calls `ModulatableSlider(label, radians, paramId, "%.1f °/s", sources, RAD_TO_DEG)`.

Similarly, add `SliderSpeedDeg()` for non-modulatable speed sliders (currently only Sine Warp speed at `imgui_effects_warp.cpp:36`).

### Drawable Color Swatches

The drawable list (`imgui_drawables.cpp:158-187`) shows labels like `[W] Waveform 1` with no color indicator. Add a small colored square (drawn via `ImDrawList::AddRectFilled`) before each label using the drawable's solid color. For non-solid color modes (rainbow/gradient), use the first gradient stop or a default.

---

## Tasks

### Wave 1: Header and wrapper changes (no file overlap)

#### Task 1.1: Clean up imgui_effects_transforms.h

**Files**: `src/ui/imgui_effects_transforms.h`
**Creates**: Correct header declarations

**Do**: Remove `DrawStyleCategory` declaration. Keep only: `DrawSymmetryCategory`, `DrawWarpCategory`, `DrawCellularCategory`, `DrawMotionCategory`, `DrawColorCategory`. These 5 categories lack dedicated headers. Leave the include guard and forward declarations.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.2: Add speed angle wrappers to ui_units.h

**Files**: `src/ui/ui_units.h`
**Creates**: `ModulatableSliderSpeedDeg()` and `SliderSpeedDeg()` convenience wrappers

**Do**: Add two inline functions after `ModulatableSliderAngleDeg`:

`SliderSpeedDeg(label, radians, minDeg, maxDeg)` — same as `SliderAngleDeg` but with default format `"%.1f °/s"`.

`ModulatableSliderSpeedDeg(label, radians, paramId, sources)` — same as `ModulatableSliderAngleDeg` but with default format `"%.1f °/s"`.

Follow the exact pattern of the existing wrappers. Keep it inline in the header.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Fix mismatches and standardize (parallel — no file overlap between tasks)

#### Task 2.1: Fix category badge mismatches

**Files**: `src/ui/imgui_effects.cpp`

**Do**: In `GetTransformCategory()`:
- Move `TRANSFORM_DISCO_BALL` from Cellular (section 2) to Graphic (section 5)
- Move `TRANSFORM_RADIAL_PULSE` from Symmetry (section 0) to Warp (section 1)
- Move `TRANSFORM_SYNTHWAVE` from Graphic (section 5) to Retro (section 6)
- Move `TRANSFORM_LEGO_BRICKS` from Retro (section 6) to Graphic (section 5)
- Add `TRANSFORM_CIRCUIT_BOARD` to Warp (section 1) — currently falls through to default `{"???", 0}`

Each move: cut the `case TRANSFORM_X:` line from its current section and paste it into the target section's case list.

**Verify**: `cmake.exe --build build` compiles. Visually confirm: pipeline list badges match the category header where each effect's controls live.

---

#### Task 2.2: Standardize generator separator style

**Files**: `src/ui/imgui_effects_generators.cpp`

**Do**: Replace every instance of the 5-line separator pattern:
```cpp
ImGui::Spacing();
ImGui::Separator();
ImGui::Spacing();
ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Label");
ImGui::Spacing();
```
with:
```cpp
ImGui::SeparatorText("Label");
```

This pattern appears approximately 15 times throughout the file. Search for `ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)` to find each occurrence. Also remove the standalone `ImGui::Spacing()` + `ImGui::Separator()` + `ImGui::Spacing()` that precede these `TextColored` calls. The leading comment lines (e.g., `// Radial Wave`) can stay as-is.

Keep the single `Separator()+Spacing()` between "Params" and "Output" sub-functions if they exist (e.g., between `DrawFilamentsParams` and `DrawFilamentsOutput`) — these are section boundaries, not labeled sub-groups, and `SeparatorText()` doesn't apply there. Actually, those already have a label ("Output" etc.), so convert them too.

**Verify**: `cmake.exe --build build` compiles. Visual check: sub-group headers in Generators panel now match Simulations style.

---

#### Task 2.3: Register spectrum params and update UI

**Files**: `src/automation/drawable_params.cpp`, `src/ui/drawable_type_controls.cpp`

**Depends on**: Wave 1 complete (needs no new wrappers, just existing `ModulatableDrawableSlider`)

**Do**:

In `drawable_params.cpp`, inside the `if (d->type == DRAWABLE_SPECTRUM)` block (after the existing colorShift/colorShiftSpeed registrations), add:
- `drawable.<id>.innerRadius` → `&d->spectrum.innerRadius`, bounds 0.05f–0.4f
- `drawable.<id>.barHeight` → `&d->spectrum.barHeight`, bounds 0.1f–0.5f
- `drawable.<id>.barWidth` → `&d->spectrum.barWidth`, bounds 0.3f–1.0f
- `drawable.<id>.smoothing` → `&d->spectrum.smoothing`, bounds 0.0f–0.95f

Follow the exact snprintf+ModEngineRegisterParam pattern from the waveform section.

In `drawable_type_controls.cpp` `DrawSpectrumControls()`:
- Replace `ImGui::SliderFloat("Radius", &d->spectrum.innerRadius, 0.05f, 0.4f)` with `ModulatableDrawableSlider("Radius", &d->spectrum.innerRadius, d->id, "innerRadius", "%.2f", sources)`
- Replace `ImGui::SliderFloat("Height", &d->spectrum.barHeight, 0.1f, 0.5f)` with `ModulatableDrawableSlider("Height", &d->spectrum.barHeight, d->id, "barHeight", "%.2f", sources)`
- Replace `ImGui::SliderFloat("Width", &d->spectrum.barWidth, 0.3f, 1.0f)` with `ModulatableDrawableSlider("Width", &d->spectrum.barWidth, d->id, "barWidth", "%.2f", sources)`
- In the Dynamics section, replace `ImGui::SliderFloat("Smooth", &d->spectrum.smoothing, 0.0f, 0.95f)` with `ModulatableDrawableSlider("Smooth", &d->spectrum.smoothing, d->id, "smoothing", "%.2f", sources)`

**Verify**: `cmake.exe --build build` compiles. Open a Spectrum drawable and verify diamond modulation indicators appear on Radius, Height, Width, Smooth.

---

#### Task 2.4: Update speed sliders to use °/s suffix

**Files**: `src/ui/imgui_effects_symmetry.cpp`, `src/ui/imgui_effects_cellular.cpp`, `src/ui/imgui_effects_graphic.cpp`, `src/ui/imgui_effects_motion.cpp`, `src/ui/imgui_effects_warp.cpp`, `src/ui/imgui_effects_generators.cpp`, `src/ui/imgui_effects.cpp`

**Depends on**: Task 1.2 (needs `ModulatableSliderSpeedDeg` wrapper)

**Do**: For every `ModulatableSliderAngleDeg` call where the underlying value is a rotation speed (field name contains `Speed`, `speed`, or label contains "Spin", "Twist"), replace with `ModulatableSliderSpeedDeg`. Specifically:

**`imgui_effects_symmetry.cpp`**: "Spin" and "Twist" sliders for kaleidoscope, KIFS, mandelbox, triangle fold, moire, radial IFS (12 calls). "Motion Speed##poincare", "Rotation Speed##poincare" (2 calls).

**`imgui_effects_cellular.cpp`**: "Spin##lattice", "Angle Drift##phyllo", "Phase Pulse##phyllo", "Spin Speed##phyllo" (4 calls).

**`imgui_effects_graphic.cpp`**: "Spin##halftone", "Spin##disco" (2 calls). Note: Disco Ball already passes `"%.1f °/s"` explicitly — remove the format arg when switching to `ModulatableSliderSpeedDeg`.

**`imgui_effects_motion.cpp`**: "Rotation Speed##dws" and any other speed fields (check each — "Spiral Angle" and "Twist" are static angles, not speeds).

**`imgui_effects_warp.cpp`**: "Speed##mobius", "Drift Speed##domainwarp", "Phase Speed##fftradialwarp", "View Rotation##corridorwarp" (already has "°/s"), "Plane Rotation##corridorwarp" (already has "°/s"). For those already passing explicit `"%.1f °/s"`, switch to `ModulatableSliderSpeedDeg` and drop the format arg.

**`imgui_effects_generators.cpp`**: "Rotation Speed##pitchspiral", "Rotation Speed##spectralarcs", "Rotation Speed##filaments", and any layer rotation speed calls.

**`imgui_effects.cpp`**: Flow field "Spin##base", "Spin##radial", "Spin##angular" — these are rotation speeds.

Also: Replace the single `SliderAngleDeg("Speed##sineWarp", ...)` in `imgui_effects_warp.cpp:36` with `SliderSpeedDeg("Speed##sineWarp", ...)`.

**Important**: Do NOT change static angle/offset parameters (e.g., "Angle", "Offset", "Octave Rotation", "Sensor Angle", "Turn Angle", "Light Angle", "Divergence Angle", "Scroll Angle", "Ridge Angle", "Flow Angle"). Only speed/rate fields that accumulate over time.

**Verify**: `cmake.exe --build build` compiles. Spot-check several effects: speed sliders display "°/s", angle sliders still display "°".

---

#### Task 2.5: Add drawable color swatches

**Files**: `src/ui/imgui_drawables.cpp`

**Do**: In the drawable list loop (`imgui_drawables.cpp:158-187`), after the `ImGui::Selectable` call and before the label text, draw a small color indicator square. Implementation:

1. After line ~172 (where label is populated), extract the drawable's display color:
   - If `colorMode == COLOR_MODE_SOLID`: use `d->base.color.solid`
   - If `colorMode == COLOR_MODE_GRADIENT`: use the first gradient stop's color
   - If `colorMode == COLOR_MODE_RAINBOW`: use a default cyan

2. Use `ImGui::SameLine()` and `ImDrawList::AddRectFilled()` to draw an 8x8 color square inline before the type badge. Follow the same pattern used in the pipeline list for drawing category badges — get cursor position, draw the rect, then render the label text.

Actually, simpler approach: insert a colored bullet or square right before the label text inside the Selectable. Since `Selectable` uses the full label for display, modify the approach: render the Selectable with an empty display string + ID, then overlay the colored square + label text using `SameLine()` and `ImDrawList`.

Follow the pipeline list pattern at lines 836-854 of `imgui_effects.cpp` — use `ImGui::SameLine()` after a blank `Selectable`, then draw content.

**Verify**: `cmake.exe --build build` compiles. Visual check: each drawable in the list shows a small color swatch matching its configured color.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Pipeline badges match the category header each effect lives under
- [ ] Generator sub-groups use `SeparatorText()` matching simulation style
- [ ] `imgui_effects_transforms.h` has no stale declarations
- [ ] Spectrum geometry sliders show modulation diamonds
- [ ] Speed sliders display "°/s", static angle sliders display "°"
- [ ] Drawable list shows color swatches
