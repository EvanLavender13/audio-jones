# Category Reorganization: Artistic + Graphic → Painterly / Print / Novelty

Replace two loosely-defined transform categories with three tighter ones. Artistic (traditional media) becomes Painterly. Graphic splits into Print (mechanical reproduction) and Novelty (fun transformations). All other categories unchanged.

## Design

### Category Mapping

| Old Category | Old Section | Old Badge | New Category | New Section | New Badge |
|-------------|-------------|-----------|-------------|-------------|-----------|
| Artistic | 4 | ART | Painterly | 4 | ART |
| Graphic | 5 | GFX | Print | 5 | PRT |
| — | — | — | Novelty | 14 | NOV |

### Effect Assignments

**Painterly (section 4, badge "ART")** — no change needed for these 6:
- Oil Paint, Watercolor, Impressionist, Ink Wash, Pencil Sketch, Cross-Hatching

**Painterly (section 4, badge "ART")** — moved from Graphic:
- Kuwahara: section 5→4, badge "GFX"→"ART"
- Woodblock: section 5→4, badge "GFX"→"ART"

**Print (section 5, badge "PRT")** — badge change only:
- Toon: badge "GFX"→"PRT"
- Halftone: badge "GFX"→"PRT"
- Risograph: badge "GFX"→"PRT"

**Novelty (section 14, badge "NOV")** — moved from Graphic:
- Disco Ball: section 5→14, badge "GFX"→"NOV"
- LEGO Bricks: section 5→14, badge "GFX"→"NOV"

### CATEGORY_INFO Changes

```cpp
// Before:
{/* 4  */ "Artistic", 4},  {/* 5  */ "Graphic", 5},

// After:
{/* 4  */ "Painterly", 4},  {/* 5  */ "Print", 5},
// ...
{/* 14 */ "Novelty", 5},
```

Novelty gets glow index 5 (ORANGE) — distinct from neighbors: Painterly (4/MAGENTA) before, Retro (6/CYAN) after.

### UI Ordering

Novelty (section 14) renders between Print (section 5) and Retro (section 6) in the transforms panel.

---

## Tasks

### Wave 1: All changes (no file overlaps)

#### Task 1.1: Move Kuwahara to Painterly

**Files**: `src/effects/kuwahara.cpp`

**Do**: In the `REGISTER_EFFECT` macro call, change `"GFX", 5` to `"ART", 4`.

**Verify**: Compiles.

#### Task 1.2: Move Woodblock to Painterly

**Files**: `src/effects/woodblock.cpp`

**Do**: In the `REGISTER_EFFECT` macro call, change `"GFX", 5` to `"ART", 4`.

**Verify**: Compiles.

#### Task 1.3: Rebadge Toon to Print

**Files**: `src/effects/toon.cpp`

**Do**: In the `REGISTER_EFFECT` macro call, change `"GFX"` to `"PRT"`. Section stays 5.

**Verify**: Compiles.

#### Task 1.4: Rebadge Halftone to Print

**Files**: `src/effects/halftone.cpp`

**Do**: In the `REGISTER_EFFECT` macro call, change `"GFX"` to `"PRT"`. Section stays 5.

**Verify**: Compiles.

#### Task 1.5: Rebadge Risograph to Print

**Files**: `src/effects/risograph.cpp`

**Do**: In the `REGISTER_EFFECT` macro call, change `"GFX"` to `"PRT"`. Section stays 5.

**Verify**: Compiles.

#### Task 1.6: Move Disco Ball to Novelty

**Files**: `src/effects/disco_ball.cpp`

**Do**: In the `REGISTER_EFFECT` macro call, change `"GFX", 5` to `"NOV", 14`.

**Verify**: Compiles.

#### Task 1.7: Move LEGO Bricks to Novelty

**Files**: `src/effects/lego_bricks.cpp`

**Do**: In the `REGISTER_EFFECT` macro call, change `"GFX", 5` to `"NOV", 14`.

**Verify**: Compiles.

#### Task 1.8: Update CATEGORY_INFO and add Novelty draw call

**Files**: `src/ui/imgui_effects_dispatch.cpp`, `src/ui/imgui_effects.cpp`

**Do**:
- In `imgui_effects_dispatch.cpp`: rename `CATEGORY_INFO[4]` from `"Artistic"` to `"Painterly"`, rename `CATEGORY_INFO[5]` from `"Graphic"` to `"Print"`. Extend the array to index 14 by adding a slot for `{/* 14 */ "Novelty", 5}` (pad indices 14 with null entries for any gaps if needed — but current max is 13, so just append one entry).
- In `imgui_effects.cpp`: add `DrawEffectCategory(e, modSources, 14);` with `ImGui::Spacing();` between the existing section 5 and section 6 calls (lines 266-267).

**Verify**: Compiles.

#### Task 1.9: Update docs/effects.md

**Files**: `docs/effects.md`

**Do**: Under TRANSFORMS, rename "Artistic" to "Painterly" and add Kuwahara and Woodblock to it. Rename "Graphic" to "Print" and keep only Toon, Halftone, Risograph. Add new "Novelty" subsection between Print and Retro with Disco Ball and LEGO Bricks.

**Verify**: Section names and effect lists match the design above.

#### Task 1.10: Update docs/conventions.md

**Files**: `docs/conventions.md`

**Do**: Update the "Category badges" line to replace `"ART"` description with `(Painterly)`, replace `"GFX" (Graphic)` with `"PRT" (Print)`, add `"NOV" (Novelty)`. Update the "Category section indices" line to show `4=Painterly, 5=Print` (was Artistic, Graphic) and add `14=Novelty`.

**Verify**: Badge list and section index list match the new categories.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Painterly section shows: Oil Paint, Watercolor, Impressionist, Ink Wash, Pencil Sketch, Cross-Hatching, Kuwahara, Woodblock
- [ ] Print section shows: Toon, Halftone, Risograph
- [ ] Novelty section shows: Disco Ball, LEGO Bricks
- [ ] Transform pipeline list badges show ART/PRT/NOV correctly
- [ ] Novelty appears between Print and Retro in UI
- [ ] docs/effects.md and docs/conventions.md updated
