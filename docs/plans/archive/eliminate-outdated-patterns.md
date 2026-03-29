# Eliminate Outdated UI Patterns

Replace all `TreeNodeAccented`/`TreeNodeAccentedPop` usage with `ImGui::SeparatorText`, then remove the deprecated widget. This is Phase 3 Priority 1 from `docs/research/ui_design_language.md`. Section name normalization (Priority 4) and section ordering (Priority 3) are separate future work.

**Research**: `docs/research/ui_design_language.md`

## Design

### Conversion Pattern

Every call site follows the same structure:

```
BEFORE:
  if (TreeNodeAccented("Label##id", glow)) {
      // params
      TreeNodeAccentedPop();
  }

AFTER:
  ImGui::SeparatorText("Label");
  // params
```

- Remove the `if (...) {` and closing `}`
- Remove `TreeNodeAccentedPop();`
- Replace `TreeNodeAccented("Label##id", glow)` with `ImGui::SeparatorText("Label")`
- Strip `##id` suffixes (SeparatorText is non-interactive, no ImGui ID needed)
- Content becomes unconditional (always visible)

### Include Cleanup

After conversion, each effect file no longer needs `#include "ui/imgui_panels.h"` (it was only included for TreeNodeAccented). Remove it unless the file uses another symbol from that header. None of the 17 affected files are generators, so none use `ImGuiDrawColorMode` or other imgui_panels.h symbols.

### Unused `glow` Parameter

The `DrawParamsFn` signature is `void (*)(EffectConfig*, const ModSources*, ImU32)`. The `glow` (ImU32) parameter was used only for TreeNodeAccented. After conversion, add `(void)glow;` at the top of each Draw function.

**glitch.cpp special case**: The 9 sub-functions (`DrawGlitchAnalog`, `DrawGlitchDigital`, etc.) each accept `ImU32 glow` solely for TreeNodeAccented. After conversion, remove the `glow` parameter from each sub-function signature and from the call sites in `DrawGlitchParams`. Then add `(void)glow;` in `DrawGlitchParams` only.

### Widget Removal

Remove from `src/ui/imgui_panels.h` (lines 58-61):
- Comment, `TreeNodeAccented` declaration, `TreeNodeAccentedPop` declaration

Remove from `src/ui/imgui_widgets.cpp` (lines 259-285):
- Three statics (`sTreeNodeAccentStartY`, `sTreeNodeAccentX`, `sTreeNodeAccentColor`)
- `TreeNodeAccented` function body
- `TreeNodeAccentedPop` function body

### Affected Files (17 effects)

| File | Sections | Section Names |
|------|----------|---------------|
| `glitch.cpp` | 9 | Analog, Digital, VHS, Datamosh, Slice, Diagonal Bands, Block Mask, Temporal, Block Multiply |
| `crt.cpp` | 5 | Phosphor Mask, Scanlines, Curvature, Vignette, Pulse |
| `synthwave.cpp` | 5 | Palette, Grid, Sun Stripes, Horizon Glow, Animation |
| `mobius.cpp` | 3 | Fixed Points, Point 1 Motion, Point 2 Motion |
| `density_wave_spiral.cpp` | 2 | Center, Aspect |
| `droste_zoom.cpp` | 2 | Masking, Spiral |
| `lens_space.cpp` | 2 | Center, Sphere Position |
| `texture_warp.cpp` | 2 | Directional, Noise |
| `mandelbox.cpp` | 2 | Box Fold, Sphere Fold |
| `toon.cpp` | 1 | Brush Stroke |
| `wave_ripple.cpp` | 1 | Origin |
| `color_grade.cpp` | 1 | Lift/Gamma/Gain |
| `chladni_warp.cpp` | 1 | Animation |
| `disco_ball.cpp` | 1 | Light Spots |
| `moire_interference.cpp` | 1 | Center |
| `relativistic_doppler.cpp` | 1 | Center |
| `pencil_sketch.cpp` | 1 | Animation |

---

## Tasks

### Wave 1: Convert All Effect Files

All 17 effect files have no file overlap. They run in parallel.

#### Task 1.1: Convert glitch.cpp

**Files**: `src/effects/glitch.cpp`

**Do**:
1. In each of the 9 `DrawGlitch*` sub-functions, apply the conversion pattern from the Design section
2. Remove the `ImU32 glow` parameter from each sub-function signature (it was only used for TreeNodeAccented)
3. Update all 9 call sites in `DrawGlitchParams` to remove the `glow` argument
4. Add `(void)glow;` at the top of `DrawGlitchParams`
5. Remove `#include "ui/imgui_panels.h"`

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.2: Convert crt.cpp and synthwave.cpp

**Files**: `src/effects/crt.cpp`, `src/effects/synthwave.cpp`

**Do**: For each file:
1. Apply the conversion pattern from the Design section to every `TreeNodeAccented`/`TreeNodeAccentedPop` pair
2. Add `(void)glow;` at the top of the Draw function
3. Remove `#include "ui/imgui_panels.h"`

crt.cpp has 5 sections, synthwave.cpp has 5 sections.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.3: Convert medium-complexity effects

**Files**: `src/effects/mobius.cpp`, `src/effects/texture_warp.cpp`, `src/effects/density_wave_spiral.cpp`, `src/effects/droste_zoom.cpp`, `src/effects/lens_space.cpp`, `src/effects/mandelbox.cpp`

**Do**: For each file:
1. Apply the conversion pattern from the Design section to every `TreeNodeAccented`/`TreeNodeAccentedPop` pair
2. Add `(void)glow;` at the top of the Draw function
3. Remove `#include "ui/imgui_panels.h"`

Each file has 2-3 TreeNodeAccented sections.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.4: Convert single-section effects

**Files**: `src/effects/toon.cpp`, `src/effects/wave_ripple.cpp`, `src/effects/color_grade.cpp`, `src/effects/chladni_warp.cpp`, `src/effects/disco_ball.cpp`, `src/effects/moire_interference.cpp`, `src/effects/relativistic_doppler.cpp`, `src/effects/pencil_sketch.cpp`

**Do**: For each file:
1. Apply the conversion pattern from the Design section to the single `TreeNodeAccented`/`TreeNodeAccentedPop` pair
2. Add `(void)glow;` at the top of the Draw function (check if glow is already suppressed or used elsewhere first)
3. Remove `#include "ui/imgui_panels.h"`

Each file has 1 TreeNodeAccented section.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 2: Remove Deprecated Widget

#### Task 2.1: Remove TreeNodeAccented widget definition

**Files**: `src/ui/imgui_panels.h`, `src/ui/imgui_widgets.cpp`

**Depends on**: Wave 1 complete (no call sites remain)

**Do**:
1. In `imgui_panels.h`: Remove lines 58-61 (the comment, `TreeNodeAccented` declaration, `TreeNodeAccentedPop` declaration)
2. In `imgui_widgets.cpp`: Remove lines 259-285 (the three statics, `TreeNodeAccented` function, `TreeNodeAccentedPop` function)

**Verify**: `cmake.exe --build build` compiles with no errors. `grep -r TreeNodeAccented src/` returns no matches.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `grep -r TreeNodeAccented src/` returns zero matches
- [ ] `grep -r TreeNodeAccentedPop src/` returns zero matches
- [ ] No `#include "ui/imgui_panels.h"` remains in the 17 converted files
- [ ] All converted sections render as non-collapsible `SeparatorText` dividers
