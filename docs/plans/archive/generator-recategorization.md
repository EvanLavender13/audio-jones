# Generator Recategorization

Add a new "Sculpture" generator subcategory for closed/bounded raymarched objects, move Marble and Geode into it, move Neon Lattice from Geometric to Field, and sync `docs/effects.md` to reflect Dream Zoom's existing placement under Texture.

**Research**: none (architectural reorg, no algorithm work)

## Design

### New Section

- Section index: `17`
- Display name: `"Sculpture"`
- Glow color index: `6` (shared with Novelty — color reuse is already the convention; see existing `CATEGORY_INFO` table)
- Group: GENERATORS
- Panel placement: alphabetical sort within the Generators group puts Sculpture between Scatter and Texture

### Section Index Map (post-change)

| Index | Name | Glow |
|-------|------|------|
| 0 | Symmetry | 0 |
| 1 | Warp | 1 |
| 2 | Cellular | 2 |
| 3 | Motion | 3 |
| 4 | Painterly | 4 |
| 5 | Print | 5 |
| 6 | Retro | 7 |
| 7 | Optical | 8 |
| 8 | Color | 9 |
| 9 | Simulation | 4 |
| 10 | Geometric | 3 |
| 11 | Filament | 2 |
| 12 | Texture | 5 |
| 13 | Field | 1 |
| 14 | Novelty | 6 |
| 15 | Scatter | 4 |
| 16 | Cymatics | 0 |
| **17** | **Sculpture** | **6** |

### Effect Moves (post-change)

| Effect | Old section | New section |
|--------|-------------|-------------|
| Neon Lattice | 10 (Geometric) | 13 (Field) |
| Marble | 13 (Field) | 17 (Sculpture) |
| Geode | 13 (Field) | 17 (Sculpture) |
| Dream Zoom | 12 (Texture) — already correct | (no code change; doc only) |

### Constants

- No new enums; existing `TRANSFORM_NEON_LATTICE_BLEND`, `TRANSFORM_MARBLE_BLEND`, `TRANSFORM_GEODE_BLEND` are unchanged.
- Generator badge stays `"GEN"` for all moved effects.

---

## Tasks

### Wave 1: Parallel edits (no file overlap between any two tasks)

#### Task 1.1: Add Sculpture row to category dispatch table

**Files**: `src/ui/imgui_effects_dispatch.cpp`

**Do**: Extend the `CATEGORY_INFO[]` array (currently lines 33-43) with one new entry:

```cpp
{/* 17 */ "Sculpture", 6},
```

The array is initialized in index order; append after the `{/* 16 */ "Cymatics", 0}` row. `CATEGORY_INFO_COUNT` is computed via `sizeof` and updates automatically.

**Verify**: Build compiles. `CATEGORY_INFO_COUNT` evaluates to 18 at runtime.

---

#### Task 1.2: Render Sculpture in the Generators panel

**Files**: `src/ui/imgui_effects.cpp`

**Do**: In the GENERATORS group block (around lines 135-146), insert a new `DrawEffectCategory` call for index 17 between the Scatter and Texture lines so the alphabetical order is Cymatics, Field, Filament, Geometric, Scatter, Sculpture, Texture:

```cpp
DrawEffectCategory(e, modSources, 15); // Scatter
ImGui::Spacing();
DrawEffectCategory(e, modSources, 17); // Sculpture
ImGui::Spacing();
DrawEffectCategory(e, modSources, 12); // Texture
```

Do not change any other section index in this file.

**Verify**: Build compiles. Generators panel shows a new "Sculpture" header between Scatter and Texture at runtime.

---

#### Task 1.3: Move Marble to Sculpture

**Files**: `src/effects/marble.cpp`

**Do**: In the `REGISTER_GENERATOR` macro at the bottom of the file (line 197), change the section index argument from `13` to `17`. Only that integer changes; do not touch the `// clang-format off / on` markers, function names, or any other arguments.

**Verify**: Build compiles. Marble appears under the Sculpture header in the Generators panel; no longer under Field.

---

#### Task 1.4: Move Geode to Sculpture

**Files**: `src/effects/geode.cpp`

**Do**: In the `REGISTER_GENERATOR` macro at the bottom of the file (line 261), change the section index argument from `13` to `17`. Only that integer changes.

**Verify**: Build compiles. Geode appears under the Sculpture header; no longer under Field.

---

#### Task 1.5: Move Neon Lattice to Field

**Files**: `src/effects/neon_lattice.cpp`

**Do**: In the `REGISTER_GENERATOR` macro at the bottom of the file (line 228), change the section index argument from `10` to `13`. Only that integer changes.

**Verify**: Build compiles. Neon Lattice appears under the Field header; no longer under Geometric.

---

#### Task 1.6: Update effects inventory

**Files**: `docs/effects.md`

**Do**: Apply four changes to the GENERATORS section:

1. **Remove** the `Dream Zoom` row from the `### Field` table (it lives under Texture in code already).
2. **Remove** the `Marble` row from the `### Field` table.
3. **Remove** the `Geode` row from the `### Field` table.
4. **Remove** the `Neon Lattice` row from the `### Geometric` table.
5. **Add** the `Neon Lattice` row to the `### Field` table (alphabetical position; descriptions unchanged).
6. **Add** the `Dream Zoom` row to the `### Texture` table (alphabetical position; description unchanged).
7. **Insert** a new `### Sculpture` subsection between `### Scatter` and `### Texture`, containing the moved Marble and Geode rows with their existing descriptions:

```markdown
### Sculpture

| Effect | Description |
|--------|-------------|
| Geode | Spherical cluster of tumbled crystal cubes hollowed by winding caverns like a cracked-open mineral geode |
| Marble | Glowing glass marble floating in darkness with luminous fractal filaments swirling inside like a living cat's-eye |
```

Do not modify any descriptions; preserve them verbatim from the existing rows.

**Verify**: Visual diff shows: Field loses Dream Zoom, Marble, Geode; Field gains Neon Lattice; Geometric loses Neon Lattice; Texture gains Dream Zoom; new Sculpture section contains Geode + Marble.

---

#### Task 1.7: Update conventions section index list

**Files**: `docs/conventions.md`

**Do**: Find the `Category section indices` line under "Effect Descriptor Pattern" (currently `0=Symmetry, ..., 16=Cymatics`). Append `, 17=Sculpture` to the list. Do not touch any other text.

**Verify**: The line now ends with `..., 16=Cymatics, 17=Sculpture`.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Launching the app shows a "Sculpture" header inside the GENERATORS group
- [ ] Marble and Geode appear under Sculpture (and only Sculpture)
- [ ] Neon Lattice appears under Field (and only Field)
- [ ] Dream Zoom appears under Texture (already true; smoke check)
- [ ] No effect appears under more than one section, and no section is empty
- [ ] `docs/effects.md` matches the runtime panel
- [ ] `docs/conventions.md` index list ends with `17=Sculpture`
