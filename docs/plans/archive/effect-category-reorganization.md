# Effect Category Reorganization

Reorganize transform effects in UI by visual outcome, not implementation history. Users think in terms of what they see, not mathematical taxonomy.

## Current State

Effects listed flat in TRANSFORMS group, ordered by implementation date:
- `src/ui/imgui_effects.cpp:200-540` - TRANSFORMS group with reorder list and 6 effect sections

Current order in UI:
1. Infinite Zoom
2. Kaleidoscope
3. Texture Warp
4. Radial Blur
5. Sine Warp
6. Voronoi

## Categories

| Category | Effects | Visual Result |
|----------|---------|---------------|
| **Symmetry** | Kaleidoscope | Mirrored/folded patterns |
| **Warp** | Sine Warp, Texture Warp, Voronoi | Bend/distort space |
| **Motion** | Infinite Zoom, Radial Blur | Depth/movement/streaks |
| **Experimental** | (empty) | Future effects |

## Design Decisions

- **Pipeline groups unchanged**: FEEDBACK, SIMULATIONS, TRANSFORMS, OUTPUT stay as-is
- **Reorder list stays at top**: Global Up/Down list above sub-categories
- **Collapsible sub-sections**: Each category uses `DrawSectionBegin()` for collapse/expand
- **Experimental bucket**: Empty now, catches future effects that don't fit existing categories

---

## Phase 1: Add Sub-category Structure

**Goal**: Reorganize TRANSFORMS into collapsible sub-categories.

**Build**:

1. Add section state variables for sub-categories in `imgui_effects.cpp`:
   - `sectionSymmetry`, `sectionWarp`, `sectionMotion`, `sectionExperimental`

2. After the reorder list (line ~258), add sub-category sections:
   - **Symmetry** section containing Kaleidoscope
   - **Warp** section containing Sine Warp, Texture Warp, Voronoi
   - **Motion** section containing Infinite Zoom, Radial Blur
   - **Experimental** section (empty, shows "No effects" text)

3. Move existing effect sections into their sub-categories:
   - Preserve all existing `DrawSectionBegin()` calls for individual effects
   - Effects become nested inside sub-category sections
   - Keep individual effect section state variables (`sectionKaleidoscope`, etc.)

4. Apply consistent styling:
   - Sub-category headers use `DrawGroupHeader()` with distinct colors
   - Individual effects use `DrawSectionBegin()` as before

**Done when**: TRANSFORMS group shows 4 collapsible sub-categories, each containing its effects. Reorder list at top still functions. All effect parameters accessible.

---

## Phase 2: Documentation Update

**Goal**: Update docs to reflect new organization.

**Build**:

1. Update `docs/effects.md`:
   - Revise "Reorderable transforms" list to show category grouping
   - Add note about sub-category organization

**Done when**: `docs/effects.md` accurately describes the new UI organization.
