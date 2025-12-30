# Modularize: imgui_drawables.cpp

Extract query helpers and type-specific controls from `src/ui/imgui_drawables.cpp` (404 lines) to reduce complexity. The main function `ImGuiDrawDrawablesPanel` has CCN 44 (threshold: 20).

## Current State

- `src/ui/imgui_drawables.cpp:36-67` - Three query helpers (`CountWaveforms`, `HasSpectrum`, `CountShapes`)
- `src/ui/imgui_drawables.cpp:70-195` - Five control functions (base + type-specific)
- `src/ui/imgui_drawables.cpp:27-31,154` - Section toggle state (5 static bools)
- `src/render/drawable.h` - Drawable API with lifecycle, process, render functions
- `src/render/drawable.cpp` - 209 lines, includes `DrawableValidate` which counts types internally

## Phase 1: Add Query Functions to Render Module

**Goal**: Create generalized drawable type queries in the render layer.

**Build**:
- Add `DrawableCountByType(const Drawable*, int, DrawableType) -> int` to `render/drawable.h:57`
- Add `DrawableHasType(const Drawable*, int, DrawableType) -> bool` to `render/drawable.h:58`
- Implement both in `render/drawable.cpp` after line 208 - simple iteration loops

**Done when**: Build succeeds with new API available.

---

## Phase 2: Create Type Controls Module

**Goal**: Extract type-specific control rendering to dedicated file.

**Build**:
- Create `src/ui/drawable_type_controls.h`:
  - Forward declare `Drawable`, `ModSources`
  - Declare `DrawWaveformControls`, `DrawSpectrumControls`, `DrawShapeControls`
- Create `src/ui/drawable_type_controls.cpp`:
  - Move section toggle state from `imgui_drawables.cpp:27-31,154`
  - Move `DrawBaseAnimationControls` and `DrawBaseColorControls` as static helpers
  - Move `DrawWaveformControls`, `DrawSpectrumControls`, `DrawShapeControls` as public functions
  - Includes: `imgui.h`, `ui/imgui_panels.h`, `ui/theme.h`, `ui/ui_units.h`, `ui/modulatable_drawable_slider.h`, `config/drawable_config.h`, `automation/mod_sources.h`
- Add `src/ui/drawable_type_controls.cpp` to CMakeLists.txt

**Done when**: Build succeeds with new module compiled (no callers yet).

---

## Phase 3: Refactor Drawables Panel

**Goal**: Update `imgui_drawables.cpp` to use extracted modules.

**Build**:
- Add `#include "ui/drawable_type_controls.h"` to `imgui_drawables.cpp`
- Replace query calls in `ImGuiDrawDrawablesPanel`:
  - `CountWaveforms(drawables, *count)` → `DrawableCountByType(drawables, *count, DRAWABLE_WAVEFORM)`
  - `HasSpectrum(drawables, *count)` → `DrawableHasType(drawables, *count, DRAWABLE_SPECTRUM)`
  - `CountShapes(drawables, *count)` → `DrawableCountByType(drawables, *count, DRAWABLE_SHAPE)`
- Replace control calls:
  - `DrawWaveformControls(sel, sources)` → unchanged signature, now external
  - `DrawSpectrumControls(sel, sources)` → unchanged signature, now external
  - `DrawShapeControls(sel, sources)` → unchanged signature, now external
- Remove dead code from `imgui_drawables.cpp`:
  - Lines 27-31: section state statics (moved)
  - Line 154: `sectionTexture` (moved)
  - Lines 36-67: query helpers (replaced)
  - Lines 70-195: control functions (moved)

**Done when**: Build succeeds, UI renders correctly, all drawable types show proper controls.

---

## Phase 4: Verification and Cleanup

**Goal**: Verify functionality and sync documentation.

**Build**:
- Run application and verify:
  - Each drawable type displays correct sections (Geometry, Animation, Color, etc.)
  - Section collapse/expand state persists across selections
  - Add/delete/reorder operations work
  - Modulation sliders function correctly
- Run `/sync-architecture` to update docs

**Done when**: All manual verification passes, architecture docs updated.

---

## Data Flow After Refactor

```
imgui_drawables.cpp (ImGuiDrawDrawablesPanel)
    |
    +-- DrawableCountByType() --> render/drawable.cpp
    +-- DrawableHasType()     --> render/drawable.cpp
    |
    +-- DrawWaveformControls() --> ui/drawable_type_controls.cpp
    +-- DrawSpectrumControls() --> ui/drawable_type_controls.cpp
    +-- DrawShapeControls()    --> ui/drawable_type_controls.cpp
                                      |
                                      +-- DrawBaseAnimationControls() (static)
                                      +-- DrawBaseColorControls() (static)
                                      +-- section toggle state (static)
```

## Expected Impact

- `imgui_drawables.cpp` drops from ~404 lines to ~200 lines
- `ImGuiDrawDrawablesPanel` CCN drops from 44 to ~25
- Query logic consolidated with `DrawableValidate` in render module
- Type controls isolated for easier maintenance when adding drawable types
