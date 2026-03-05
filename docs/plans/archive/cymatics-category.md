# Cymatics Category

Create a "Cymatics" generator category (section index 16) and move Ripple Tank and Chladni from section 13 (Field) into it.

**Research**: `docs/research/cymatics-category.md` (Sections 1, 5, 6)
**Depends on**: `docs/plans/ripple-tank-merge.md` (must be implemented first)

## Design

### Constants

- Section index: 16
- Section badge: `"CYM"`
- Section display name: `"Cymatics"`
- Members: Ripple Tank (`TRANSFORM_RIPPLE_TANK`), Chladni (`TRANSFORM_CHLADNI_BLEND`)

### Changes

Both Ripple Tank and Chladni currently register with section index 13 (Field). Their `REGISTER_GENERATOR_FULL` macros change the section argument from `13` to `16`. No other code changes to those effects.

The UI layout in `imgui_effects.cpp` adds a new `DrawEffectCategory(e, modSources, 16)` call in the GENERATORS group.

---

## Tasks

### Wave 1 (parallel — no file overlap)

#### Task 1.1: Update Ripple Tank section index

**Files**: `src/effects/ripple_tank.cpp` (modify)

**Do**: In the `REGISTER_GENERATOR_FULL` macro at the bottom, change section index from `13` to `16`.

**Verify**: Compiles.

---

#### Task 1.2: Update Chladni section index

**Files**: `src/effects/chladni.cpp` (modify)

**Do**: In the `REGISTER_GENERATOR_FULL` macro at the bottom, change section index from `13` to `16`.

**Verify**: Compiles.

---

#### Task 1.3: Add Cymatics category to UI

**Files**: `src/ui/imgui_effects.cpp` (modify)

**Do**: In the GENERATORS group section, add a new category draw call for section 16. Insert it before the existing `DrawEffectCategory(e, modSources, 13)` (Field) call with spacing:
```cpp
DrawEffectCategory(e, modSources, 16); // Cymatics
ImGui::Spacing();
```

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Ripple Tank and Chladni appear under "CYM" (Cymatics) category in UI
- [ ] Section 13 (Field) still shows Constellation and Nebula
- [ ] Category badges in pipeline list show "CYM" for Ripple Tank and Chladni
