# Generator Category Reorganization

Reorganize generator subcategories: dissolve Atmosphere, create Scatter and Field, move Plasma to Filament and Solid Color to Texture. UI order becomes alphabetical: Field, Filament, Geometric, Scatter, Texture.

## Design

### Section Index Mapping

Repurpose section 13 (was Atmosphere) as Field. Add section 15 as Scatter.

| Index | Old Name    | New Name   | Glow |
|-------|-------------|------------|------|
| 10    | Geometric   | Geometric  | 0    |
| 11    | Filament    | Filament   | 1    |
| 12    | Texture     | Texture    | 2    |
| 13    | Atmosphere  | **Field**  | 3    |
| 15    | *(new)*     | **Scatter**| 5    |

### Effect Moves

| Effect        | File                  | Old Section      | New Section     |
|---------------|-----------------------|------------------|-----------------|
| Slashes       | `slashes.cpp`         | 11 (Filament)    | 15 (Scatter)    |
| Fireworks     | `fireworks.cpp`       | 13 (Atmosphere)  | 15 (Scatter)    |
| Spark Flash   | `spark_flash.cpp`     | 13 (Atmosphere)  | 15 (Scatter)    |
| Constellation | `constellation.cpp`   | 11 (Filament)    | 13 (Field)      |
| Interference  | `interference.cpp`    | 12 (Texture)     | 13 (Field)      |
| Plasma        | `plasma.cpp`          | 12 (Texture)     | 11 (Filament)   |
| Solid Color   | `solid_color.cpp`     | 13 (Atmosphere)  | 12 (Texture)    |

Nebula stays at section 13 — no change needed (13 is just renamed from Atmosphere to Field).

### UI Order (alphabetical)

```
DrawEffectCategory(e, modSources, 13); // Field
DrawEffectCategory(e, modSources, 11); // Filament
DrawEffectCategory(e, modSources, 10); // Geometric
DrawEffectCategory(e, modSources, 15); // Scatter
DrawEffectCategory(e, modSources, 12); // Texture
```

---

## Tasks

### Wave 1: All changes (no file overlap)

#### Task 1.1: Update UI dispatch infrastructure

**Files**: `src/ui/imgui_effects_dispatch.cpp`, `src/ui/imgui_effects.cpp`

**Do**:

1. In `imgui_effects_dispatch.cpp`, update `CATEGORY_INFO[]`:
   - Change index 13 from `{"Atmosphere", 3}` to `{"Field", 3}`
   - Add index 15: `{"Scatter", 5}` at the end of the array

2. In `imgui_effects.cpp`, replace the GENERATORS section calls:
   ```cpp
   // Old:
   DrawEffectCategory(e, modSources, 10);
   ImGui::Spacing();
   DrawEffectCategory(e, modSources, 11);
   ImGui::Spacing();
   DrawEffectCategory(e, modSources, 12);
   ImGui::Spacing();
   DrawEffectCategory(e, modSources, 13);

   // New (alphabetical):
   DrawEffectCategory(e, modSources, 13); // Field
   ImGui::Spacing();
   DrawEffectCategory(e, modSources, 11); // Filament
   ImGui::Spacing();
   DrawEffectCategory(e, modSources, 10); // Geometric
   ImGui::Spacing();
   DrawEffectCategory(e, modSources, 15); // Scatter
   ImGui::Spacing();
   DrawEffectCategory(e, modSources, 12); // Texture
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.2: Update effect registration macros

**Files**: `src/effects/slashes.cpp`, `src/effects/fireworks.cpp`, `src/effects/spark_flash.cpp`, `src/effects/constellation.cpp`, `src/effects/interference.cpp`, `src/effects/plasma.cpp`, `src/effects/solid_color.cpp`

**Do**: Change the section index parameter in each file's `REGISTER_GENERATOR` or `REGISTER_GENERATOR_FULL` macro:

- `slashes.cpp` line 189: change `11` → `15`
- `fireworks.cpp` line 274: change `13` → `15`
- `spark_flash.cpp` line 172: change `13` → `15`
- `constellation.cpp` line 255: change `11` → `13`
- `interference.cpp` line 224: change `12` → `13`
- `plasma.cpp` line 166: change `12` → `11`
- `solid_color.cpp` line 65: change `13` → `12`

Each is a single integer change in the registration macro. No other changes needed.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.3: Update effects inventory

**Files**: `docs/effects.md`

**Do**: Reorganize the GENERATORS section to reflect the new subcategories in alphabetical order:

- **Field**: Constellation (from Filament), Interference (from Texture), Nebula (from Atmosphere)
- **Filament**: Filaments, Muons, Attractor Lines, Plasma (from Texture) — remove Constellation and Slashes
- **Geometric**: unchanged (Signal Frames, Arc Strobe, Hex Rush, Pitch Spiral, Spectral Arcs, Iris Rings, Prism Shatter)
- **Scatter**: Slashes (from Filament), Fireworks (from Atmosphere), Spark Flash (from Atmosphere)
- **Texture**: Moire Generator, Scan Bars, Glyph Field, Motherboard, Bit Crush, Data Traffic, Plaid, Scrawl, Byzantine, Solid Color (from Atmosphere) — remove Plasma and Interference

Remove the Atmosphere subsection entirely.

**Verify**: Subsection headings are alphabetical. Every generator appears exactly once.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All 5 generator subcategories render in alphabetical order in the UI
- [ ] Each generator appears in exactly one subcategory
- [ ] Atmosphere section no longer exists
- [ ] `docs/effects.md` matches the new categorization
