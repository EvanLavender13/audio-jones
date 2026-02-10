# Generator Sub-Categories

Split 15 generators from one flat `imgui_effects_generators.cpp` into 4 sub-category files with `DrawCategoryHeader()` visual grouping. Also consolidate the 4 stray transform sub-category headers into `imgui_effects_transforms.h`.

**Research**: `docs/research/generator_categories.md`

## Design

### File Structure

Mirrors how transforms organize — each transform category has its own `.cpp` file (`imgui_effects_symmetry.cpp`, `imgui_effects_warp.cpp`, etc.) with declarations in the shared `imgui_effects_transforms.h`. Generators follow the same pattern with `imgui_effects_generators.h` as the shared header.

| New File | Sub-Category | Effects |
|----------|-------------|---------|
| `imgui_effects_gen_geometric.cpp` | Geometric | Signal Frames, Arc Strobe, Pitch Spiral, Spectral Arcs |
| `imgui_effects_gen_filament.cpp` | Filament | Constellation, Filaments, Muons, Slashes |
| `imgui_effects_gen_texture.cpp` | Texture | Plasma, Interference, Moire Generator, Scan Bars, Glyph Field |
| `imgui_effects_gen_atmosphere.cpp` | Atmosphere | Nebula, Solid Color |

### Function Signatures

**Public sub-category functions** (declared in `imgui_effects_generators.h`):

```
DrawGeneratorsGeometric(EffectConfig *e, const ModSources *modSources)
DrawGeneratorsFilament(EffectConfig *e, const ModSources *modSources)
DrawGeneratorsTexture(EffectConfig *e, const ModSources *modSources)
DrawGeneratorsAtmosphere(EffectConfig *e, const ModSources *modSources)
```

**Dispatcher** (remains in `imgui_effects_generators.cpp`):

```
DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources)
```

Signature change: drops the `int &sectionIndex` parameter. Each sub-category picks a fixed glow color internally instead of cycling per-effect.

**Static draw functions** per effect stay named `DrawGenerators<Name>()` — they move from `imgui_effects_generators.cpp` to their sub-category file unchanged, along with their `static bool section*` state.

### Glow Colors

Each sub-category uses a fixed color for its `DrawCategoryHeader()` and all child effect sections. Follows the transform pattern (e.g., Symmetry = `GetSectionGlow(0)` for all 7 effects).

| Sub-Category | Index | Color |
|-------------|-------|-------|
| Geometric | 0 | Cyan |
| Filament | 1 | Magenta |
| Texture | 2 | Orange |
| Atmosphere | 3 | Cyan (wraps) |

Pipeline badge stays `"GEN"` for all generators. No descriptor table changes.

### Transform Header Consolidation

Four stray per-category headers (`imgui_effects_artistic.h`, `imgui_effects_graphic.h`, `imgui_effects_optical.h`, `imgui_effects_retro.h`) each declare one function. These declarations move into `imgui_effects_transforms.h` and the stray headers are deleted.

---

## Tasks

### Wave 1: Header Updates

#### Task 1.1: Update generator header

**Files**: `src/ui/imgui_effects_generators.h` (modify)
**Creates**: Declarations that Wave 2 sub-category `.cpp` files include

**Do**: Replace the single `DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources, int &sectionIndex)` declaration with 5 declarations:
- `DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources)` (no `int &sectionIndex`)
- `DrawGeneratorsGeometric(EffectConfig *e, const ModSources *modSources)`
- `DrawGeneratorsFilament(EffectConfig *e, const ModSources *modSources)`
- `DrawGeneratorsTexture(EffectConfig *e, const ModSources *modSources)`
- `DrawGeneratorsAtmosphere(EffectConfig *e, const ModSources *modSources)`

Follow same file structure as `imgui_effects_transforms.h` — forward declarations of `EffectConfig` and `ModSources`, then function declarations.

**Verify**: `cmake.exe --build build` compiles (will have linker errors until Wave 2 — that's expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create geometric generators UI

**Files**: `src/ui/imgui_effects_gen_geometric.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Extract from `imgui_effects_generators.cpp`: the static draw functions and `static bool` section states for Signal Frames, Arc Strobe, Pitch Spiral, Spectral Arcs. Create `DrawGeneratorsGeometric()` following the transform category pattern — call `DrawCategoryHeader("Geometric", categoryGlow)` then each effect draw function with the same `categoryGlow`. Use `Theme::GetSectionGlow(0)` as the fixed color. Follow `imgui_effects_symmetry.cpp` as the reference pattern.

Include headers: `imgui_effects_generators.h`, same set as current `imgui_effects_generators.cpp` but only the effect headers needed (arc_strobe.h, pitch_spiral.h, signal_frames.h, spectral_arcs.h), plus imgui.h, config/effect_config.h, automation/mod_sources.h, ui/imgui_panels.h, ui/modulatable_slider.h, ui/theme.h, ui/ui_units.h, render/blend_mode.h.

**Verify**: Compiles.

---

#### Task 2.2: Create filament generators UI

**Files**: `src/ui/imgui_effects_gen_filament.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Extract from `imgui_effects_generators.cpp`: static draw functions and `static bool` section states for Constellation, Filaments, Muons, Slashes. Create `DrawGeneratorsFilament()` — call `DrawCategoryHeader("Filament", categoryGlow)` then each effect. Use `Theme::GetSectionGlow(1)` as the fixed color. Follow same structure as Task 2.1.

Include only needed effect headers: constellation.h, filaments.h, muons.h, slashes.h.

**Verify**: Compiles.

---

#### Task 2.3: Create texture generators UI

**Files**: `src/ui/imgui_effects_gen_texture.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Extract from `imgui_effects_generators.cpp`: static draw functions and `static bool` section states for Plasma, Interference, Moire Generator, Scan Bars, Glyph Field. Create `DrawGeneratorsTexture()` — call `DrawCategoryHeader("Texture", categoryGlow)` then each effect. Use `Theme::GetSectionGlow(2)` as the fixed color.

Include only needed effect headers: plasma.h, interference.h, moire_generator.h, scan_bars.h, glyph_field.h.

**Verify**: Compiles.

---

#### Task 2.4: Create atmosphere generators UI

**Files**: `src/ui/imgui_effects_gen_atmosphere.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Extract from `imgui_effects_generators.cpp`: static draw functions and `static bool` section states for Nebula, Solid Color. Create `DrawGeneratorsAtmosphere()` — call `DrawCategoryHeader("Atmosphere", categoryGlow)` then each effect. Use `Theme::GetSectionGlow(3)` as the fixed color.

Include only needed effect headers: nebula.h, solid_color.h.

**Verify**: Compiles.

---

#### Task 2.5: Rewrite generators dispatcher

**Files**: `src/ui/imgui_effects_generators.cpp` (rewrite)
**Depends on**: Wave 1 complete

**Do**: Replace the entire file contents. Remove all 15 static draw functions and their `static bool` states (moved to sub-category files). The file becomes a slim dispatcher:

- Include `imgui_effects_generators.h`, `imgui.h`, `ui/imgui_panels.h`, `ui/theme.h`
- `DrawGeneratorsCategory()` calls the 4 sub-category functions with `ImGui::Spacing()` between them
- No `int &sectionIndex` parameter — sub-categories handle their own colors

Follow `imgui_effects.cpp:764-780` as the reference pattern for how it dispatches to transform categories.

**Verify**: Compiles.

---

#### Task 2.6: Update integration files

**Files**: `src/ui/imgui_effects.cpp` (modify), `src/ui/imgui_effects_transforms.h` (modify), `src/ui/imgui_effects_retro.cpp` (modify)
**Delete**: `src/ui/imgui_effects_artistic.h`, `src/ui/imgui_effects_graphic.h`, `src/ui/imgui_effects_optical.h`, `src/ui/imgui_effects_retro.h`
**Depends on**: Wave 1 complete

**Do**:

1. **`imgui_effects_transforms.h`**: Add declarations for `DrawArtisticCategory`, `DrawGraphicCategory`, `DrawOpticalCategory`, `DrawRetroCategory` (same signature as the existing 5 declarations).

2. **`imgui_effects.cpp`**: Remove the 4 stray includes (`imgui_effects_artistic.h`, `imgui_effects_graphic.h`, `imgui_effects_optical.h`, `imgui_effects_retro.h`). Change the generators call from `int genIdx = 0; DrawGeneratorsCategory(e, modSources, genIdx);` to just `DrawGeneratorsCategory(e, modSources);`.

3. **`imgui_effects_retro.cpp`**: Change `#include "ui/imgui_effects_retro.h"` to `#include "ui/imgui_effects_transforms.h"` (the other 3 category .cpp files already include transforms.h).

4. **Delete** the 4 stray header files.

**Verify**: Compiles with no stray header references.

---

#### Task 2.7: Update CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add the 4 new `.cpp` files to the `IMGUI_UI_SOURCES` list, after the existing `imgui_effects_generators.cpp` entry:

```
src/ui/imgui_effects_gen_geometric.cpp
src/ui/imgui_effects_gen_filament.cpp
src/ui/imgui_effects_gen_texture.cpp
src/ui/imgui_effects_gen_atmosphere.cpp
```

**Verify**: CMake configure succeeds.

---

#### Task 2.8: Update effects documentation

**Files**: `docs/effects.md` (modify)
**Depends on**: Wave 1 complete

**Do**: Replace the flat GENERATORS table with 4 sub-headed tables matching the TRANSFORMS format. Add `###` headings: Geometric, Filament, Texture, Atmosphere. Each sub-heading has its own table. Keep effect descriptions unchanged. Use the groupings from `docs/research/generator_categories.md`.

**Verify**: Markdown renders correctly.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build`
- [ ] No stray includes of deleted headers (`imgui_effects_artistic.h`, etc.)
- [ ] All 15 generators still appear in the UI under their sub-category headers
- [ ] Each sub-category uses `DrawCategoryHeader()` with a fixed glow color
- [ ] Pipeline list badges still show `GEN` for all generators
- [ ] `docs/effects.md` GENERATORS section has 4 sub-headings with tables
