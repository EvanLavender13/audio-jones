# Normalize Section Names

Rename all non-standard `SeparatorText` section labels to the canonical vocabulary defined in the UI Design Language. This is Phase 3 Priority 4 of the Signal Stack rollout.

**Research**: `docs/research/ui_design_language.md`

## Design

### Canonical Vocabulary

| Standard Name | Replaces |
|---|---|
| `"Geometry"` | `"Shape"`, `"Grid"`, `"Ring Layout"`, `"Structure"` |
| `"Animation"` | `"Motion"`, `"Timing"`, `"Dynamics"`, `"Movement"`, `"Speed"`, `"Rotation"` |
| `"Glow"` | `"Visual"`, `"Rendering"`, `"Appearance"` |
| `"Color"` | `"Tonemap"`, `"Palette"` |
| `"Trail"` | `"Trails"` (plural to singular) |
| `"Audio"` | `"Dynamics"` (in drawable_type_controls.cpp only -- smoothing/dB params are audio, not animation) |

### Rename Rules

**Simple rename**: File has no existing section with the standard name. Change the `SeparatorText` string literal. No code movement.

**Merge**: File already has a section with the standard name AND a non-standard section covering the same domain. Remove the non-standard `SeparatorText` call so the params fall under the existing standard section. If the sections are not adjacent, move the non-standard params directly after the standard section's params.

**Structure -> Animation**: `prism_shatter.cpp` has `"Structure"` with a single speed param and no Animation section. Rename `"Structure"` to `"Animation"`.

### Change Manifest

#### Simple Geometry Renames (no existing Geometry section)

| File | Line | Old | New |
|---|---|---|---|
| `src/effects/arc_strobe.cpp` | 197 | `"Shape"` | `"Geometry"` |
| `src/effects/scan_bars.cpp` | 187 | `"Shape"` | `"Geometry"` |
| `src/effects/spin_cage.cpp` | 239 | `"Shape"` | `"Geometry"` |
| `src/effects/synapse_tree.cpp` | 150 | `"Shape"` | `"Geometry"` |
| `src/effects/spectral_rings.cpp` | 178 | `"Shape"` | `"Geometry"` |
| `src/effects/glyph_field.cpp` | 225 | `"Grid"` | `"Geometry"` |
| `src/effects/spectral_arcs.cpp` | 161 | `"Ring Layout"` | `"Geometry"` |
| `src/effects/synthwave.cpp` | 140 | `"Grid"` | `"Geometry"` |

#### Geometry Merges (existing Geometry section absorbs non-standard section)

| File | Standard Section | Non-standard Section | Action |
|---|---|---|---|
| `src/effects/bokeh.cpp` | `"Geometry"` (line 73) | `"Shape"` (line 76) | Remove `"Shape"` SeparatorText |
| `src/effects/polyhedral_mirror.cpp` | `"Geometry"` (line 261) | `"Shape"` (line 270) | Remove `"Shape"` SeparatorText |
| `src/effects/polymorph.cpp` | `"Geometry"` (line 343) | `"Shape"` (line 352) | Remove `"Shape"` SeparatorText |
| `src/effects/neon_lattice.cpp` | none | `"Grid"` (line 189) + `"Shape"` (line 212) | Rename `"Grid"` to `"Geometry"`, move Shape params after Grid params, remove `"Shape"` SeparatorText |

#### Simple Animation Renames

| File | Line | Old | New |
|---|---|---|---|
| `src/effects/voxel_march.cpp` | 233 | `"Motion"` | `"Animation"` |
| `src/effects/isoflow.cpp` | 204 | `"Motion"` | `"Animation"` |
| `src/effects/rainbow_road.cpp` | 171 | `"Motion"` | `"Animation"` |
| `src/effects/protean_clouds.cpp` | 216 | `"Motion"` | `"Animation"` |
| `src/effects/scan_bars.cpp` | 208 | `"Motion"` | `"Animation"` |
| `src/effects/fireworks.cpp` | 216 | `"Timing"` | `"Animation"` |
| `src/effects/slashes.cpp` | 173 | `"Timing"` | `"Animation"` |
| `src/effects/hex_rush.cpp` | 246 | `"Dynamics"` | `"Animation"` |
| `src/effects/neon_lattice.cpp` | 204 | `"Speed"` | `"Animation"` |
| `src/effects/spin_cage.cpp` | 245 | `"Rotation"` | `"Animation"` |
| `src/effects/prism_shatter.cpp` | 147 | `"Structure"` | `"Animation"` |
| `src/simulation/physarum.cpp` | 456 | `"Movement"` | `"Animation"` |
| `src/simulation/curl_flow.cpp` | 443 | `"Movement"` | `"Animation"` |
| `src/simulation/boids.cpp` | 449 | `"Movement"` | `"Animation"` |

#### Simple Glow Renames

| File | Line | Old | New |
|---|---|---|---|
| `src/effects/fireworks.cpp` | 234 | `"Visual"` | `"Glow"` |
| `src/effects/hex_rush.cpp` | 262 | `"Visual"` | `"Glow"` |
| `src/effects/subdivide.cpp` | 161 | `"Visual"` | `"Glow"` |
| `src/effects/polygon_subdivide.cpp` | 165 | `"Visual"` | `"Glow"` |
| `src/effects/motherboard.cpp` | 184 | `"Rendering"` | `"Glow"` |
| `src/effects/scrawl.cpp` | 152 | `"Rendering"` | `"Glow"` |
| `src/effects/prism_shatter.cpp` | 152 | `"Rendering"` | `"Glow"` |
| `src/effects/attractor_lines.cpp` | 337 | `"Appearance"` | `"Glow"` |
| `src/effects/star_trail.cpp` | 233 | `"Appearance"` | `"Glow"` |

#### Color Changes

| File | Line | Old | New | Action |
|---|---|---|---|---|
| `src/effects/synthwave.cpp` | 132 | `"Palette"` | `"Color"` | Simple rename |
| `src/effects/vortex.cpp` | 209 | `"Tonemap"` | -- | Merge: remove SeparatorText, keep Brightness slider under existing `"Color"` (line 202) |
| `src/effects/muons.cpp` | 291 | `"Tonemap"` | -- | Merge: remove SeparatorText, keep Brightness slider under existing `"Color"` (line 281) |

#### Trail Change

| File | Line | Old | New |
|---|---|---|---|
| `src/effects/muons.cpp` | 274 | `"Trails"` | `"Trail"` |

#### Audio Change (Drawable UI)

| File | Line | Old | New |
|---|---|---|---|
| `src/ui/drawable_type_controls.cpp` | 71 | `"Dynamics"` | `"Audio"` |

#### Drawable Geometry Change

| File | Line | Old | New |
|---|---|---|---|
| `src/ui/drawable_type_controls.cpp` | 191 | `"Shape"` | `"Geometry"` |

---

## Tasks

### Wave 1: All Tasks (No File Overlap)

All files are independent `.cpp` files with no cross-file dependencies. Every task runs in parallel.

#### Task 1.1: Simple Geometry Renames

**Files**: `src/effects/arc_strobe.cpp`, `src/effects/synapse_tree.cpp`, `src/effects/spectral_rings.cpp`, `src/effects/glyph_field.cpp`, `src/effects/spectral_arcs.cpp`

**Do**: In each file, find the `SeparatorText` call and change the string literal to `"Geometry"`. No code movement needed.

- `arc_strobe.cpp`: `"Shape"` -> `"Geometry"`
- `synapse_tree.cpp`: `"Shape"` -> `"Geometry"`
- `spectral_rings.cpp`: `"Shape"` -> `"Geometry"`
- `glyph_field.cpp`: `"Grid"` -> `"Geometry"`
- `spectral_arcs.cpp`: `"Ring Layout"` -> `"Geometry"`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.2: Geometry Merges + neon_lattice Animation

**Files**: `src/effects/bokeh.cpp`, `src/effects/polyhedral_mirror.cpp`, `src/effects/polymorph.cpp`, `src/effects/neon_lattice.cpp`

**Do**: For each file, read the Draw*Params function and apply the merge:

- **bokeh.cpp**: Remove the `SeparatorText("Shape")` line. The Shape combo, Shape Angle, Star Points, and Inner Radius sliders stay where they are -- they now fall under the preceding `"Geometry"` header.
- **polyhedral_mirror.cpp**: Remove the `SeparatorText("Shape")` line. The Shape combo stays -- it now falls under the preceding `"Geometry"` header.
- **polymorph.cpp**: Remove the `SeparatorText("Shape")` line. The Shape combo, Randomness, and Freeform sliders stay -- they now fall under the preceding `"Geometry"` header.
- **neon_lattice.cpp**: Three changes: (1) Rename `SeparatorText("Grid")` to `SeparatorText("Geometry")`. (2) Move the Ring Radius and Tube Width sliders (currently under `"Shape"`) to directly after the Grid params (Axes, Spacing, Light Spacing, Thickness), then remove `SeparatorText("Shape")`. (3) Rename `SeparatorText("Speed")` to `SeparatorText("Animation")`.

**Verify**: `cmake.exe --build build` compiles. Grep `SeparatorText.*Shape` and `SeparatorText.*Grid` and `SeparatorText.*Speed` in these four files to confirm none remain.

---

#### Task 1.3: Simple Animation Renames

**Files**: `src/effects/voxel_march.cpp`, `src/effects/isoflow.cpp`, `src/effects/rainbow_road.cpp`, `src/effects/protean_clouds.cpp`, `src/effects/slashes.cpp`, `src/simulation/physarum.cpp`, `src/simulation/curl_flow.cpp`, `src/simulation/boids.cpp`

**Do**: In each file, find the `SeparatorText` call and change the string literal to `"Animation"`:

- `voxel_march.cpp`: `"Motion"` -> `"Animation"`
- `isoflow.cpp`: `"Motion"` -> `"Animation"`
- `rainbow_road.cpp`: `"Motion"` -> `"Animation"`
- `protean_clouds.cpp`: `"Motion"` -> `"Animation"`
- `slashes.cpp`: `"Timing"` -> `"Animation"`
- `physarum.cpp`: `"Movement"` -> `"Animation"`
- `curl_flow.cpp`: `"Movement"` -> `"Animation"`
- `boids.cpp`: `"Movement"` -> `"Animation"`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.4: Multi-Change Effects (Geometry + Animation)

**Files**: `src/effects/scan_bars.cpp`, `src/effects/spin_cage.cpp`

**Do**: Each file has both a Geometry rename and an Animation rename:

- **scan_bars.cpp**: `SeparatorText("Shape")` -> `"Geometry"`, `SeparatorText("Motion")` -> `"Animation"`
- **spin_cage.cpp**: `SeparatorText("Shape")` -> `"Geometry"`, `SeparatorText("Rotation")` -> `"Animation"`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.5: Multi-Change Effects (Animation + Glow)

**Files**: `src/effects/fireworks.cpp`, `src/effects/hex_rush.cpp`, `src/effects/prism_shatter.cpp`

**Do**: Each file has both an Animation rename and a Glow rename:

- **fireworks.cpp**: `SeparatorText("Timing")` -> `"Animation"`, `SeparatorText("Visual")` -> `"Glow"`
- **hex_rush.cpp**: `SeparatorText("Dynamics")` -> `"Animation"`, `SeparatorText("Visual")` -> `"Glow"`
- **prism_shatter.cpp**: `SeparatorText("Structure")` -> `"Animation"`, `SeparatorText("Rendering")` -> `"Glow"`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.6: Simple Glow Renames

**Files**: `src/effects/subdivide.cpp`, `src/effects/polygon_subdivide.cpp`, `src/effects/motherboard.cpp`, `src/effects/scrawl.cpp`, `src/effects/attractor_lines.cpp`, `src/effects/star_trail.cpp`

**Do**: In each file, find the `SeparatorText` call and change to `"Glow"`:

- `subdivide.cpp`: `"Visual"` -> `"Glow"`
- `polygon_subdivide.cpp`: `"Visual"` -> `"Glow"`
- `motherboard.cpp`: `"Rendering"` -> `"Glow"`
- `scrawl.cpp`: `"Rendering"` -> `"Glow"`
- `attractor_lines.cpp`: `"Appearance"` -> `"Glow"`
- `star_trail.cpp`: `"Appearance"` -> `"Glow"`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.7: Color, Trail, and Geometry Changes (synthwave, vortex, muons)

**Files**: `src/effects/synthwave.cpp`, `src/effects/vortex.cpp`, `src/effects/muons.cpp`

**Do**:

- **synthwave.cpp**: `SeparatorText("Palette")` -> `"Color"`, `SeparatorText("Grid")` -> `"Geometry"`
- **vortex.cpp**: Remove the `SeparatorText("Tonemap")` line. The Brightness slider stays where it is -- it now falls under the preceding `"Color"` section.
- **muons.cpp**: (1) `SeparatorText("Trails")` -> `"Trail"`, (2) Remove `SeparatorText("Tonemap")` line so Brightness falls under preceding `"Color"` section.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.8: Drawable UI Changes

**Files**: `src/ui/drawable_type_controls.cpp`

**Do**: Two changes:

- `SeparatorText("Dynamics")` (line ~71) -> `"Audio"`
- `SeparatorText("Shape")` (line ~191) -> `"Geometry"`

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe --build build`
- [ ] No banned labels remain: grep for `SeparatorText` with any of: `"Shape"`, `"Grid"`, `"Ring Layout"`, `"Structure"`, `"Motion"`, `"Timing"`, `"Dynamics"`, `"Movement"`, `"Speed"`, `"Rotation"`, `"Visual"`, `"Rendering"`, `"Appearance"`, `"Tonemap"`, `"Palette"`, `"Trails"` across `src/effects/`, `src/simulation/`, and `src/ui/drawable_type_controls.cpp`
- [ ] Standard labels gained: grep confirms new `"Geometry"`, `"Animation"`, `"Glow"`, `"Color"`, `"Trail"`, `"Audio"` labels in the expected files
