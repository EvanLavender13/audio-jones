# UI Phase 3 Priority 2: Organize Flat Effects

Add `ImGui::SeparatorText` section headers to 28 effects that have 6+ parameters but no internal sections. Reorder params where needed to follow canonical section ordering.

**Research**: `docs/research/ui_design_language.md` (Phase 3 Priority 2)

## Design

### Rules

- Add `ImGui::SeparatorText("Name")` before each param group
- Follow canonical order: Audio, Geometry, [Domain], Animation, Glow, Color
- Keep conditional param blocks (`if`/`else`) intact; put SeparatorText inside the conditional when the entire section is conditional
- For plasma only: replace `ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();` with SeparatorText
- No config struct changes, no shader changes, no new files
- This is a UI-only refactor of `Draw*Params` functions in effect `.cpp` files

### Section Layouts

Each entry shows the new section order. `[brackets]` = conditional params. `(R)` = reorder needed.

#### Symmetry

**kifs** (R): Geometry(Iterations, Scale, Offset X, Offset Y) | Fold(Octant Fold, Polar Fold, [Segments, Smoothing]) | Animation(Spin, Twist)

**poincare_disk**: Geometry(Tile P, Tile Q, Tile R, Translation X, Translation Y, Disk Scale) | Animation(Motion Radius, Motion Speed, Rotation Speed)

**radial_ifs** (R): Geometry(Segments, Iterations, Scale, Offset, Smoothing) | Animation(Spin, Twist)

**triangle_fold**: Geometry(Iterations, Scale, Offset X, Offset Y) | Animation(Spin, Twist)

#### Warp

**domain_warp**: Geometry(Strength, Scale, Iterations, Falloff) | Animation(Drift Speed, Drift Angle)

**flux_warp**: Geometry(Strength, Cell Scale) | Warp(Coupling, Wave Freq) | Animation(Anim Speed, Divisor Speed, Gate Speed)

**interference_warp** (R): Geometry(Amplitude, Scale, Axes, Harmonics, Decay) | Animation(Axis Rotation, Speed, Drift)

**corridor_warp** (R): Geometry(Tile Density, Horizon, Perspective, Mode) | Animation(View Rotation, Plane Rotation, Scroll Speed, Strafe Speed) | Color(Fog Strength)

**circuit_board** (R): Geometry(Tile Scale, Strength, Base Size) | Pattern(Contour Freq, Dual Layer, [Layer Offset]) | Animation(Breathe, Breathe Speed)

**radial_pulse** (R): Geometry(Radial Freq, Radial Amp, Segments, Swirl, Petal, Spiral Twist, Octaves, Octave Rotation, Depth Blend) | Animation(Phase Speed)

#### Cellular

**voronoi** (R): Geometry(Scale, Smooth) | Cell Mode(Mode, Intensity, [Iso Frequency], [Edge Falloff]) | Animation(Speed)

**lotus_warp** (R): Geometry(Scale) | Cell Mode(Mode, Intensity, [Iso Frequency], [Edge Falloff]) | Animation(Zoom Speed, Spin Speed)

**phyllotaxis** (R): Geometry(Scale, Smooth, Angle, Mode, Intensity, [Iso Frequency], [Cell Radius]) | Animation(Angle Drift, Phase Pulse, Spin Speed)

**multi_scale_grid** (R): Geometry(Coarse Scale, Medium Scale, Fine Scale, Warp, Cell Variation) | Edge(Edge Contrast, Edge Power) | Glow(Glow Threshold, Glow Amount, Glow Mode)

**fracture_grid** (R): Geometry(Subdivision, Stagger, Offset Scale, Rotation, Zoom Scale, Tessellation, Spatial Bias) | Animation(Wave Speed, Wave Shape)

#### Painterly + Optical

**oil_paint** (R): Brush(Layers, Brush Size, Stroke Bend, Brush Detail) | Lighting(Contrast, Brightness, Specular)

**watercolor**: Stroke(Samples, Stroke Step, Wash Strength) | Paper(Paper Scale, Paper Strength) | Flow(Edge Pool, Flow Center, Flow Width)

**impressionist** (R): Brush(Splat Count, Splat Size Min, Splat Size Max, Stroke Freq, Stroke Opacity) | Edge(Edge Strength, Outline Strength, Edge Max Darken) | Texture(Grain Scale, Grain Amount) | Glow(Exposure)

**bokeh** (R): Geometry(Radius, Iterations) | Shape(Shape, [Shape Angle], [Star Points], [Inner Radius]) | Glow(Brightness)

**phi_blur** (R): Kernel(Shape, Radius, Samples, [Shape Angle], [Star Points], [Inner Radius]) | Color(Gamma)

#### Retro

**matrix_rain** (R): Geometry(Cell Size, Faller Count) | Trail(Trail Length) | Animation(Rain Speed, Refresh Rate) | Glow(Lead Brightness) | Color(Overlay Intensity, Sample)

**lattice_crush**: Geometry(Scale, Cell Size, Iterations, Walk Mode) | Animation(Speed, Mix)

**shard_crush** (R): Geometry(Iterations, Zoom, Noise Scale, Rotation Levels, Softness) | Animation(Speed) | Color(Aberration, Mix)

**stripe_shift** (R): Geometry(Density, Width, Displace, Hard Edge) | Animation(Speed) | Color(Spread, Color Displace)

#### Motion + Novelty + Generator

**slit_scan** (R): Geometry(Slit Position, Slit Width, Mode) | Corridor([Perspective, Fog Strength, Glow] -- conditional on mode==0) | Animation(Speed, Push Accel, Rotation, Rotation Speed) | Glow(Brightness)

**wave_drift** (R): Geometry(Octaves, Strength, Octave Rotation) | Wave(Wave Type, Radial Mode, Depth Blend) | Animation(Speed)

**lego_bricks** (R): Geometry(Brick Scale, Stud Height, Max Brick Size) | Lighting(Edge Shadow, Light Angle) | Color(Color Threshold)

**plasma** (R): Geometry(Bolt Count, Layers, Octaves, Falloff, Displacement) | Animation(Drift Speed, Drift Amount, Anim Speed, Flicker, Surge, Sway, Sway Speed, Sway Rotation) | Glow(Glow Radius, Brightness)
*Also replace `Spacing/Separator/Spacing` patterns with SeparatorText.*

---

## Tasks

### Wave 1: All Effects (Parallel)

All 28 effects are independent `.cpp` files with no file overlap. All tasks run in parallel.

#### Task 1.1: Symmetry

**Files**: `src/effects/kifs.cpp`, `src/effects/poincare_disk.cpp`, `src/effects/radial_ifs.cpp`, `src/effects/triangle_fold.cpp`

**Do**: For each effect, edit the `Draw*Params` function to add `ImGui::SeparatorText("SectionName")` calls and reorder params per the Design section.

- **kifs**: Move Fold params (Octant Fold, Polar Fold, conditional Segments/Smoothing) before Animation (Spin, Twist). Add SeparatorText for Geometry, Fold, Animation.
- **poincare_disk**: Add SeparatorText for Geometry, Animation. No reorder.
- **radial_ifs**: Move Smoothing up after Offset, before Spin. Add SeparatorText for Geometry, Animation.
- **triangle_fold**: Add SeparatorText for Geometry, Animation. No reorder.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.2: Warp

**Files**: `src/effects/domain_warp.cpp`, `src/effects/flux_warp.cpp`, `src/effects/interference_warp.cpp`, `src/effects/corridor_warp.cpp`, `src/effects/circuit_board.cpp`, `src/effects/radial_pulse.cpp`

**Do**: For each effect, edit the `Draw*Params` function to add `ImGui::SeparatorText` calls and reorder params per the Design section.

- **domain_warp**: Add SeparatorText for Geometry, Animation. No reorder.
- **flux_warp**: Add SeparatorText for Geometry, Warp, Animation. No reorder.
- **interference_warp**: Move Axis Rotation down after Decay. Add SeparatorText for Geometry, Animation.
- **corridor_warp**: Move Tile Density to position 1 (before Horizon). Add SeparatorText for Geometry, Animation, Color.
- **circuit_board**: Move Pattern block (Contour Freq, Dual Layer, conditional Layer Offset) before Animation (Breathe, Breathe Speed). Add SeparatorText for Geometry, Pattern, Animation.
- **radial_pulse**: Move Phase Speed to after Depth Blend. Add SeparatorText for Geometry, Animation.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.3: Cellular

**Files**: `src/effects/voronoi.cpp`, `src/effects/lotus_warp.cpp`, `src/effects/phyllotaxis.cpp`, `src/effects/multi_scale_grid.cpp`, `src/effects/fracture_grid.cpp`

**Do**: For each effect, edit the `Draw*Params` function to add `ImGui::SeparatorText` calls and reorder params per the Design section.

- **voronoi**: Move Speed after the Cell Mode block (Mode, Intensity, conditional Iso Frequency, conditional Edge Falloff). Move Smooth next to Scale. Add SeparatorText for Geometry, Cell Mode, Animation.
- **lotus_warp**: Move Cell Mode block (Mode, Intensity, conditionals) before Zoom Speed/Spin Speed. Add SeparatorText for Geometry, Cell Mode, Animation.
- **phyllotaxis**: Move Mode, Intensity, and conditional params (Iso Frequency, Cell Radius) up after Angle, before the three speed params. Add SeparatorText for Geometry, Animation.
- **multi_scale_grid**: Move Cell Variation up after Fine Scale. Add SeparatorText for Geometry, Edge, Glow.
- **fracture_grid**: Move Spatial Bias up after Tessellation. Add SeparatorText for Geometry, Animation.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.4: Painterly + Optical

**Files**: `src/effects/oil_paint.cpp`, `src/effects/watercolor.cpp`, `src/effects/impressionist.cpp`, `src/effects/bokeh.cpp`, `src/effects/phi_blur.cpp`

**Do**: For each effect, edit the `Draw*Params` function to add `ImGui::SeparatorText` calls and reorder params per the Design section.

- **oil_paint**: Move Layers to position 1 (before Brush Size). Add SeparatorText for Brush, Lighting.
- **watercolor**: Add SeparatorText for Stroke, Paper, Flow. No reorder.
- **impressionist**: Reorder to: Splat Count, Splat Size Min, Splat Size Max, Stroke Freq, Stroke Opacity, Edge Strength, Outline Strength, Edge Max Darken, Grain Scale, Grain Amount, Exposure. Add SeparatorText for Brush, Edge, Texture, Glow.
- **bokeh**: Move Brightness after the Shape block (Shape, conditional Shape Angle/Star Points/Inner Radius). Add SeparatorText for Geometry, Shape, Glow.
- **phi_blur**: Move Gamma to end (after all Kernel params including conditionals). Add SeparatorText for Kernel, Color.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.5: Retro

**Files**: `src/effects/matrix_rain.cpp`, `src/effects/lattice_crush.cpp`, `src/effects/shard_crush.cpp`, `src/effects/stripe_shift.cpp`

**Do**: For each effect, edit the `Draw*Params` function to add `ImGui::SeparatorText` calls and reorder params per the Design section.

- **matrix_rain**: Reorder to: Cell Size, Faller Count, Trail Length, Rain Speed, Refresh Rate, Lead Brightness, Overlay Intensity, Sample. Add SeparatorText for Geometry, Trail, Animation, Glow, Color.
- **lattice_crush**: Add SeparatorText for Geometry, Animation. No reorder.
- **shard_crush**: Move Noise Scale up after Zoom, move Aberration and Mix to end. Reorder to: Iterations, Zoom, Noise Scale, Rotation Levels, Softness, Speed, Aberration, Mix. Add SeparatorText for Geometry, Animation, Color.
- **stripe_shift**: Move Hard Edge up after Displace. Reorder to: Density, Width, Displace, Hard Edge, Speed, Spread, Color Displace. Add SeparatorText for Geometry, Animation, Color.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.6: Motion + Novelty + Generator

**Files**: `src/effects/slit_scan.cpp`, `src/effects/wave_drift.cpp`, `src/effects/lego_bricks.cpp`, `src/effects/plasma.cpp`

**Do**: For each effect, edit the `Draw*Params` function to add `ImGui::SeparatorText` calls and reorder params per the Design section.

- **slit_scan**: Reorder to: Slit Position, Slit Width, Mode, [if mode==0: Perspective, Fog Strength, Glow], Speed, Push Accel, Rotation, Rotation Speed, Brightness. Add SeparatorText for Geometry. Inside the `if (mode == 0)` block, add SeparatorText for Corridor. After the conditional, add SeparatorText for Animation and Glow.
- **wave_drift**: Reorder to: Octaves, Strength, Octave Rotation, Wave Type, Radial Mode, Depth Blend, Speed. Add SeparatorText for Geometry, Wave, Animation.
- **lego_bricks**: Reorder to: Brick Scale, Stud Height, Max Brick Size, Edge Shadow, Light Angle, Color Threshold. Add SeparatorText for Geometry, Lighting, Color.
- **plasma**: Remove all `ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();` blocks. Move Displacement after Falloff (into Geometry group). Move Flicker after Anim Speed (into Animation group). Move Surge/Sway block after Flicker. Glow Radius and Brightness become the final Glow section. Add SeparatorText for Geometry, Animation, Glow.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no new warnings
- [ ] All 28 effects have SeparatorText section headers
- [ ] No effect has `Spacing/Separator/Spacing` manual breaks remaining
- [ ] No effect has params from different canonical sections interleaved
