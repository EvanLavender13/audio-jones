# UI Design Language Phase 1: Panel Refactors

Refactor LFO, Analysis, and Drawables panels to use the Signal Stack design language. Create the Module Strip widget for LFO panel. Replace mixed `TextColored`/`DrawSectionBegin` with consistent `SeparatorText`. Convert drawables list to rich custom-drawn list with drag-to-reorder.

**Research**: `docs/research/ui_design_language.md`

## Design

### Module Strip Widget

New begin/end pair in `imgui_widgets.cpp`, declared in `imgui_panels.h`. Replaces collapsible `DrawSectionBegin` for the LFO panel with always-visible compact containers.

```c
void DrawModuleStripBegin(const char *label, ImU32 accentColor, bool *enabled);
void DrawModuleStripEnd(void);
```

**Visual anatomy (top to bottom):**

- **Top rim-light**: 1px horizontal line at top edge, `SetColorAlpha(accentColor, 60)`
- **Background tint**: full-width rect, `SetColorAlpha(accentColor, 20)` (matches `DrawCategoryHeader` convention)
- **Enable indicator**: filled circle (4px radius) in accent color when `*enabled == true`, outline-only circle (4px radius, `Theme::TEXT_DISABLED_U32`) when false. Uses a small `InvisibleButton` over the dot area for click-to-toggle.
- **Label text**: `Theme::TEXT_PRIMARY_U32` when enabled, `Theme::TEXT_DISABLED_U32` when disabled
- **Left accent bar**: 3px wide, drawn retroactively in `DrawModuleStripEnd` once height is known (deferred-height technique, same pattern as `TreeNodeAccentedPop`)
- **No bottom border**: the next module's rim-light or panel edge provides closure

**DrawModuleStripBegin flow:**

1. `GetCursorScreenPos()`, `GetContentRegionAvail().x`
2. Compute `headerHeight = lineHeight + framePadding.y * 2 + 4.0f`
3. Draw background tint rect: `AddRectFilled(pos, pos + (width, headerHeight), SetColorAlpha(accentColor, 20))`
4. Draw top rim-light: `AddLine(pos, (pos.x + width, pos.y), SetColorAlpha(accentColor, 60), 1.0f)`
5. Draw enable indicator dot:
   - Position: `dotCenter = (pos.x + 14, pos.y + headerHeight/2)`
   - If `*enabled`: `AddCircleFilled(dotCenter, 4.0f, accentColor)`
   - If `!*enabled`: `AddCircle(dotCenter, 4.0f, Theme::TEXT_DISABLED_U32, 0, 1.0f)`
   - Use `SetCursorScreenPos` to place an `InvisibleButton("##enable", (16, 16))` centered on the dot. Check `IsItemClicked()` to toggle `*enabled`.
6. Draw label text at `(pos.x + 28, pos.y + framePadding.y + 2)`, color based on enabled state
7. `Dummy(width, headerHeight)` to advance cursor
8. Store statics: `sModuleStripStartY = GetCursorScreenPos().y`, `sModuleStripX = pos.x`, `sModuleStripAccentColor = accentColor`, `sModuleStripHeaderH = headerHeight`
9. If `!*enabled`: `PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED)` (will be popped in End)
10. `Indent(12.0f)`, `Spacing()`

**DrawModuleStripEnd flow:**

1. `Spacing()`, `Unindent(12.0f)`
2. Pop the disabled text color if it was pushed (track via a static bool)
3. Compute height: `endY = GetCursorScreenPos().y`
4. Draw left accent bar: `AddRectFilled((sModuleStripX, sModuleStripStartY - sModuleStripHeaderH), (sModuleStripX + 3, endY), sModuleStripAccentColor)`
   - The accent bar spans from the top of the header to the bottom of the content
5. `Spacing()` for gap between modules

**State statics:**

```c
static float sModuleStripStartY = 0.0f;
static float sModuleStripHeaderH = 0.0f;
static float sModuleStripX = 0.0f;
static ImU32 sModuleStripAccentColor = 0;
static bool sModuleStripDisabledPushed = false;
```

### LFO Panel Changes

In `imgui_lfo.cpp`:

- Remove `DrawGroupHeader("LFOS", Theme::ACCENT_ORANGE_U32)` at line 182 (window title is sufficient)
- Remove `static bool sectionLFO[NUM_LFOS]` at line 11 (no longer collapsible)
- Replace `DrawSectionBegin(sectionLabel, ...)` / `DrawSectionEnd()` with `DrawModuleStripBegin(sectionLabel, accentColor, &configs[i].enabled)` / `DrawModuleStripEnd()`. The accent color cycles Cyan -> Magenta -> Orange per LFO index using `Theme::GetSectionAccent(i)` (existing helper).
- Remove the `ImGui::Checkbox(enabledLabel, ...)` call (the Module Strip's enable dot replaces it)
- Remove the `if (DrawSectionBegin(...)) {` guard - content is always visible
- Keep all existing content (rate slider, phase slider, waveform icons, history preview, output meter) - these are already well-designed

### Analysis Panel Changes

In `imgui_analysis.cpp`:

| Line | Current | New |
|------|---------|-----|
| 615 | `ImGui::TextColored(Theme::ACCENT_CYAN, "Beat Detection")` | `ImGui::SeparatorText("Beat Detection")` |
| 622 | `ImGui::TextColored(Theme::ACCENT_MAGENTA, "Band Energy")` | `ImGui::SeparatorText("Band Energy")` |
| 634 | `ImGui::TextColored(Theme::ACCENT_ORANGE, "Profiler")` | `ImGui::SeparatorText("Profiler")` |
| 555 | `DrawSectionBegin("Audio Features", ...)` | `ImGui::SeparatorText("Audio Features")` |
| 325 | `DrawSectionBegin("Zone Timing", ...)` | `ImGui::SeparatorText("Zone Timing")` |

Also:
- Remove `static bool featuresOpen` in `DrawAudioFeaturesSection` (line 552)
- Remove `static bool sparklinesOpen` in `DrawProfilerSparklines` (line 324)
- Remove all `DrawSectionEnd()` calls (lines 557, 563, 603, 391)
- Remove the `if (!DrawSectionBegin(...))` early-return guards - content always visible
- Remove `ImGui::Spacing()` calls adjacent to replaced headers (SeparatorText provides its own spacing)
- Remove `#include "ui/imgui_panels.h"` if no other symbols from it remain used (check: `DrawGradientBox` is still used, so the include stays)

### Drawables Panel Changes

**In `imgui_drawables.cpp`:**

*Remove redundant headers:*
- Remove `ImGui::TextColored(Theme::ACCENT_CYAN, "Drawable List")` + `Spacing()` (lines 30-31)
- Remove the `TextColored` type settings header block (lines 226-236)

*Remove Up/Down/Delete buttons:*
- Remove the Delete button block (lines 106-120)
- Remove the Up button block (lines 124-134)
- Remove the Down button block (lines 139-148)
- Keep all four `+ Waveform`, `+ Spectrum`, `+ Shape`, `+ Trail` buttons unchanged

*Convert `BeginListBox` to rich custom-drawn list:*

Follow the playlist pattern from `imgui_playlist.cpp:151-285`:

1. Replace `BeginListBox("##DrawableList", ImVec2(-FLT_MIN, 100))` with `BeginChild("##DrawableList", ImVec2(-1, 8 * ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders)`
2. Suppress Selectable backgrounds with `PushStyleColor` for Header/HeaderHovered/HeaderActive to zero alpha
3. For each drawable row:
   - `Selectable("##row", *selected == i, ImGuiSelectableFlags_AllowOverlap, ImVec2(contentWidth, 0))` for interaction
   - Use `GetItemRectMin()`/`GetItemRectMax()` for row bounds
   - Selected row: `AddRectFilled(rowMin, rowMax, SetColorAlpha(Theme::ACCENT_CYAN_U32, 32))`
   - Hovered row: `AddRectFilled(rowMin, rowMax, SetColorAlpha(Theme::ACCENT_CYAN_U32, 14))`
   - Color swatch: 8x8 filled rect (existing logic, repositioned)
   - Index number: dim text (`Theme::TEXT_DISABLED_U32`), 2-digit
   - Type badge: `"W"`, `"S"`, `"P"`, `"T"` in accent color (Cyan/Magenta/Orange/Cyan per type)
   - Name: `"Waveform 1"`, `"Spectrum 1"`, etc. in primary text (dimmed when disabled)
   - Drag-drop source: payload `"DRAWABLE_ITEM"`, display name as text
   - Drag-drop target: accept `"DRAWABLE_ITEM"`, swap with source index, call `DrawableParamsSyncAll`. Draw insertion line on hover.
   - Hover-only remove button: `SmallButton("x")` at right edge, calls the existing delete logic (unregister, shift array, sync)
4. `PopStyleColor(3)`, `EndChild()`

*Simplify selected-item section:*
- Keep `Separator()` + `Spacing()` between list and controls
- Keep `Checkbox("Enabled", ...)` and `Combo("Path", ...)` directly below
- Keep dispatch to `Draw<Type>Controls()` unchanged

**In `drawable_type_controls.cpp`:**

*Replace all `DrawSectionBegin`/`DrawSectionEnd` with `SeparatorText`:*
- Remove all 9 `static bool section*` variables (lines 11-19)
- For each `DrawSectionBegin`/`DrawSectionEnd` pair: replace with `ImGui::SeparatorText("Label")`, remove the `if` guard, remove `DrawSectionEnd()`, remove inter-section `ImGui::Spacing()` calls
- 17 `DrawSectionBegin` calls and 17 `DrawSectionEnd` calls across 4 functions, all become `SeparatorText`
- Include `"ui/imgui_panels.h"` stays - still needed for `ImGuiDrawColorMode`

---

## Tasks

### Wave 1: Foundation + Independent Refactors

#### Task 1.1: Module Strip Widget

**Files**: `src/ui/imgui_panels.h` (modify), `src/ui/imgui_widgets.cpp` (modify)
**Creates**: `DrawModuleStripBegin`/`DrawModuleStripEnd` widget pair

**Do**:
- In `imgui_panels.h`: add declarations for `DrawModuleStripBegin(const char *label, ImU32 accentColor, bool *enabled)` and `DrawModuleStripEnd(void)` in the "Reusable drawing helpers" section (after `DrawSectionEnd` declaration, before `TreeNodeAccented`)
- In `imgui_widgets.cpp`: implement both functions as specified in the Design section, between `DrawSectionEnd` and the `TreeNodeAccented` block. Use the deferred-height technique from `TreeNodeAccentedPop` (statics for startY/X/color). Follow `DrawCategoryHeader` for the background tint alpha (20) convention. The enable dot uses `InvisibleButton` for click detection.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.2: Analysis Panel Refactor

**Files**: `src/ui/imgui_analysis.cpp` (modify)

**Do**:
- In `ImGuiDrawAnalysisPanel` (line 606): replace the three `ImGui::TextColored(Theme::ACCENT_*, "...")` calls (lines 615, 622, 634) with `ImGui::SeparatorText("...")`. Remove the `ImGui::Spacing()` calls that immediately follow each replaced header.
- In `DrawAudioFeaturesSection` (line 550): remove `static bool featuresOpen`. Replace `DrawSectionBegin("Audio Features", ...)` with `ImGui::SeparatorText("Audio Features")`. Remove the early-return `if (!DrawSectionBegin(...))` guard and its `DrawSectionEnd()`. Remove the `DrawSectionEnd()` at the end of the function (line 603). The null-check for `bands`/`features` (line 561) stays as a direct early return.
- In `DrawProfilerSparklines` (line 319): remove `static bool sparklinesOpen`. Replace `DrawSectionBegin("Zone Timing", ...)` with `ImGui::SeparatorText("Zone Timing")`. Remove the early-return guard and its `DrawSectionEnd()`. Remove the `DrawSectionEnd()` at the end (line 391).
- After all replacements, check whether `#include "ui/imgui_panels.h"` is still needed. It is - `DrawGradientBox` is used throughout the file.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.3: Drawable Type Controls Refactor

**Files**: `src/ui/drawable_type_controls.cpp` (modify)

**Do**:
- Remove all 9 `static bool section*` variables (lines 11-19)
- In each of the 4 functions (`DrawWaveformControls`, `DrawSpectrumControls`, `DrawShapeControls`, `DrawParametricTrailControls`): replace every `if (DrawSectionBegin("Label", ..., &sectionVar)) { ... DrawSectionEnd(); }` pattern with just `ImGui::SeparatorText("Label");` followed by the content (no `if` guard, no `DrawSectionEnd`). Remove inter-section `ImGui::Spacing()` calls between sections.
- Do NOT remove `#include "ui/imgui_panels.h"` - the file uses `ImGuiDrawColorMode` from it.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.4: Drawables Panel Refactor

**Files**: `src/ui/imgui_drawables.cpp` (modify)

**Do**:
- Remove `ImGui::TextColored(Theme::ACCENT_CYAN, "Drawable List")` and its `Spacing()` (lines 30-31)
- Remove the Delete button block (lines 106-120), the Up button block (lines 124-134), and the Down button block (lines 139-148). Keep all four add buttons.
- Convert `BeginListBox("##DrawableList", ...)` to a rich custom-drawn list following the playlist pattern from `imgui_playlist.cpp:151-285`:
  - `BeginChild("##DrawableList", ImVec2(-1, 8 * ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders)`
  - Push 3 style colors to suppress Selectable backgrounds (Header/HeaderHovered/HeaderActive to zero alpha)
  - For each drawable: invisible `Selectable("##row", ...)` with `AllowOverlap`, then custom draw via `GetItemRectMin/Max`: selected row with `SetColorAlpha(Theme::ACCENT_CYAN_U32, 32)` background, hovered with alpha 14. Draw color swatch (8x8), index number (`Theme::TEXT_DISABLED_U32`), type badge letter (`"W"`/`"S"`/`"P"`/`"T"` in accent color matching type), and name text. Dim text when `!d->base.enabled`.
  - Add drag-drop: source payload `"DRAWABLE_ITEM"` (int index), target accepts and swaps entries + calls `DrawableParamsSyncAll`. Draw insertion line `Theme::ACCENT_CYAN_U32` at drop target edge.
  - Hover-only remove: `SmallButton("x")` at right edge when hovered, performs the same delete logic as the old Delete button (unregister, shift, decrement count, sync).
  - Pop 3 style colors, `EndChild()`
- Remove the `TextColored` type settings header block (lines 226-236). The selected item's controls follow directly after `Separator()` + `Spacing()`.
- Badge accent colors per type: `DRAWABLE_WAVEFORM` = `ACCENT_CYAN_U32`, `DRAWABLE_SPECTRUM` = `ACCENT_MAGENTA_U32`, `DRAWABLE_SHAPE` = `ACCENT_ORANGE_U32`, `DRAWABLE_PARAMETRIC_TRAIL` = `ACCENT_CYAN_U32`

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 2: LFO Panel (depends on Module Strip)

#### Task 2.1: LFO Panel Refactor

**Files**: `src/ui/imgui_lfo.cpp` (modify)
**Depends on**: Wave 1 (Task 1.1 - Module Strip widget)

**Do**:
- Remove `static bool sectionLFO[NUM_LFOS]` (line 11)
- Remove `DrawGroupHeader("LFOS", Theme::ACCENT_ORANGE_U32)` (line 182)
- In the LFO loop (line 184): replace `if (DrawSectionBegin(sectionLabel, Theme::GetSectionGlow(i), &sectionLFO[i])) { ... DrawSectionEnd(); }` with `DrawModuleStripBegin(sectionLabel, Theme::GetSectionAccent(i), &configs[i].enabled)` at the top and `DrawModuleStripEnd()` at the bottom. The accent color cycles Cyan -> Magenta -> Orange via the existing `GetSectionAccent(i)` helper. Remove the `if` guard - content is always visible.
- Remove the `ImGui::Checkbox(enabledLabel, &configs[i].enabled)` call and its `SameLine()` (lines 200-201) - the Module Strip's enable dot replaces it. Also remove the `enabledLabel` snprintf.
- Remove the `if (i < NUM_LFOS - 1) { ImGui::Spacing(); }` block (lines 247-249) - the Module Strip End provides inter-module spacing.
- Keep all other content unchanged: rate slider, phase slider, waveform icons, history preview, output meter.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

## Final Verification

- [ ] Build succeeds with `cmake.exe --build build` and no warnings
- [ ] LFO panel: 8 LFOs visible as Module Strips, enable dot toggles, no group header, no collapse arrows
- [ ] Analysis panel: all 5 sections use `SeparatorText`, none collapsible
- [ ] Drawables panel: rich list with swatches/badges/drag-reorder/hover-x, no Up/Down/Delete buttons, no `TextColored` headers
- [ ] Drawable type controls: all sections use `SeparatorText`, none collapsible
- [ ] No regressions in Effects window (DrawGroupHeader, DrawCategoryHeader, DrawSectionBegin still work there)
