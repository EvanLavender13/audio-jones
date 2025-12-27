# UI Polish Pass

Remove visual inconsistencies across panels: eliminate separators, standardize section color cycling to Cyan→Magenta→Orange, and document the pattern for future maintainability.

## Current State

The UI uses Dear ImGui with a "Neon Eclipse" synthwave theme. Eight panels exist with inconsistent visual patterns:

- `src/ui/imgui_analysis.cpp:266,275` - Uses `ImGui::Separator()` between sections
- `src/ui/imgui_waveforms.cpp:83` - Uses `ImGui::Separator()` before settings
- `src/ui/imgui_spectrum.cpp:27` - Uses `ImGui::Separator()` after enable toggle
- `src/ui/imgui_presets.cpp:57` - Uses `ImGui::Separator()` between save/load
- `src/ui/imgui_experimental.cpp:23` - Uses `ImGui::Separator()` after checkbox
- `src/ui/imgui_effects.cpp:39,53,66` - Section colors: Cyan→Magenta→Orange (correct base, but first collapsible duplicates top header color)
- `src/ui/imgui_experimental.cpp:27,36` - Section colors: Magenta→Cyan (inverted from standard)

## Phase 1: Remove Separators

**Goal**: Replace all `ImGui::Separator()` calls with spacing for consistent visual weight.

**Modify**:
- `src/ui/imgui_analysis.cpp` - Lines 265-267 and 274-276: Remove separator, keep spacing
- `src/ui/imgui_waveforms.cpp` - Lines 82-84: Remove separator, keep spacing
- `src/ui/imgui_spectrum.cpp` - Lines 26-28: Remove separator, keep spacing
- `src/ui/imgui_presets.cpp` - Lines 56-58: Remove separator, keep spacing
- `src/ui/imgui_experimental.cpp` - Lines 22-24: Remove separator, keep spacing

**Done when**: No `ImGui::Separator()` calls remain in any panel file.

---

## Phase 2: Normalize Section Colors

**Goal**: Apply consistent Cyan→Magenta→Orange cycle to all collapsible sections.

**Modify**:
- `src/ui/imgui_effects.cpp:39` - Change "Domain Warp" from `GLOW_CYAN` to `GLOW_MAGENTA` (Cyan already used by top-level header; first collapsible section starts at Magenta)
- `src/ui/imgui_effects.cpp:53` - Change "Voronoi" from `GLOW_MAGENTA` to `GLOW_ORANGE`
- `src/ui/imgui_effects.cpp:66` - Change "Physarum" from `GLOW_ORANGE` to `GLOW_CYAN` (cycle wraps)
- `src/ui/imgui_experimental.cpp:27` - Change "Feedback" from `GLOW_MAGENTA` to `GLOW_CYAN`
- `src/ui/imgui_experimental.cpp:36` - Change "Flow Field" from `GLOW_CYAN` to `GLOW_MAGENTA`

**Done when**: All collapsible sections follow Cyan→Magenta→Orange→Cyan pattern when read top-to-bottom.

---

## Phase 3: Document Convention

**Goal**: Add documentation to theme.h explaining the section color cycle for future maintainers.

**Modify**:
- `src/ui/theme.h` - Add comment block after line 70 (after `ACCENT_ORANGE_U32` definition):

```cpp
// Section color cycle convention (use in order for visual consistency):
// 1st collapsible section: GLOW_CYAN
// 2nd collapsible section: GLOW_MAGENTA
// 3rd collapsible section: GLOW_ORANGE
// 4th+ sections: repeat cycle (GLOW_CYAN, GLOW_MAGENTA, ...)
//
// Panels with a top-level header (TextColored) should match their first
// section accent to the header color, then continue the cycle.
```

**Done when**: Comment appears in theme.h and explains the convention clearly.

---

## Phase 4: Verification

**Goal**: Confirm all changes work correctly.

**Steps**:
1. Build: `cmake.exe --build build`
2. Launch application
3. Check each panel visually:
   - No horizontal separator lines visible
   - Section header colors cycle consistently
   - All panels function correctly (expand/collapse, controls work)

**Done when**: Build succeeds and visual inspection confirms no separators and consistent colors.
