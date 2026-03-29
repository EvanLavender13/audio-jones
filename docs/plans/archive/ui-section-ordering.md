# UI Section Ordering Normalization (Phase 3 Priority 3)

Reorder `SeparatorText` sections in 26 effect `Draw*Params()` functions to follow the canonical order. UI-only changes - no config structs, shaders, or serialization affected.

**Research**: `docs/research/ui_design_language.md` (Phase 3 Priority 3)

## Design

### Canonical Order

```
1. Audio       -- FFT: baseFreq, maxFreq, gain, contrast, baseBright
2. Geometry    -- Spatial: scale, size, layers, radius, count, offset
3. [Domain]    -- Effect-specific (0-4 sections, unique names)
4. Animation   -- Time: speed, phase, frequency, spin, drift
5. Glow        -- Brightness: threshold, intensity, radius, softness
6. Color       -- Color: tint, saturation, hue shift
```

Skip sections that don't apply. When present, they appear in this order. Sliders within each section stay in their current order - only the section blocks move.

### Reordering Table

Each row: cut the SeparatorText block (header + all sliders/widgets until the next SeparatorText) and paste it in the target position.

| File | Current Order | Target Order | Change |
|------|--------------|--------------|--------|
| `digital_shard.cpp` | Geometry, **Audio** | **Audio**, Geometry | Audio to 1st |
| `shell.cpp` | Geometry, **Audio**, Color | **Audio**, Geometry, Color | Audio to 1st |
| `chladni.cpp` | Wave, **Audio**, Trail | **Audio**, Wave, Trail | Audio to 1st |
| `spark_flash.cpp` | Geometry, **Audio** | **Audio**, Geometry | Audio to 1st |
| `vortex.cpp` | Raymarching, **Audio**, Color, Tonemap | **Audio**, Raymarching, Color, Tonemap | Audio to 1st |
| `triskelion.cpp` | Geometry, Animation, **Audio** | **Audio**, Geometry, Animation | Audio to 1st |
| `muons.cpp` | Raymarching, Trails, **Audio**, Color, Tonemap | **Audio**, Raymarching, Trails, Color, Tonemap | Audio to 1st |
| `fireworks.cpp` | Burst, Timing, Physics, Visual, **Audio** | **Audio**, Burst, Timing, Physics, Visual | Audio to 1st |
| `laser_dance.cpp` | (flat), Warp, Camera, **Audio** | **Audio**, (flat), Warp, Camera | Audio to 1st |
| `star_trail.cpp` | Stars, Orbit, Appearance, **Audio** | **Audio**, Stars, Orbit, Appearance | Audio to 1st |
| `spin_cage.cpp` | Shape, Rotation, Projection, Glow, **Audio** | **Audio**, Shape, Rotation, Projection, Glow | Audio to 1st |
| `faraday.cpp` | Wave, Geometry, **Audio**, Trail | **Audio**, Geometry, Wave, Trail | Audio to 1st, Geometry before Wave |
| `polymorph.cpp` | Shape, Morph, Camera, Geometry, **Audio** | **Audio**, Geometry, Shape, Morph, Camera | Audio to 1st, Geometry to 2nd |
| `polyhedral_mirror.cpp` | Shape, Camera, Reflection, **Audio**, Geometry | **Audio**, Geometry, Shape, Camera, Reflection | Audio to 1st, Geometry to 2nd |
| `viscera.cpp` | Geometry, **Animation**, Pulsation, Lighting, Height, **Audio** | **Audio**, Geometry, Pulsation, Lighting, Height, **Animation** | Audio to 1st, Animation after domain |
| `filaments.cpp` | Audio, Geometry, **Glow**, **Animation** | Audio, Geometry, **Animation**, **Glow** | Swap Animation before Glow |
| `spiral_walk.cpp` | Audio, Geometry, **Glow**, **Animation** | Audio, Geometry, **Animation**, **Glow** | Swap Animation before Glow |
| `signal_frames.cpp` | Audio, Geometry, **Glow**, Sweep, Animation | Audio, Geometry, Sweep, Animation, **Glow** | Glow to after Animation |
| `dream_fractal.cpp` | Audio, Geometry, Fold, Animation, **Julia**, Color | Audio, Geometry, Fold, **Julia**, Animation, Color | Julia before Animation |
| `galaxy.cpp` | Audio, Geometry, View, Animation, **Dust & Stars**, **Bulge** | Audio, Geometry, View, **Dust & Stars**, **Bulge**, Animation | Domain before Animation |
| `protean_clouds.cpp` | Audio, Volume, **Color**, Motion, Camera | Audio, Volume, Camera, Motion, **Color** | Camera before Motion, Color to last |
| `data_traffic.cpp` | Audio, Geometry, Animation, **Behaviors** | Audio, Geometry, **Behaviors**, Animation | Domain before Animation |
| `slashes.cpp` | Audio, Timing, **Geometry**, Glow | Audio, **Geometry**, Timing, Glow | Geometry to 2nd |
| `arc_strobe.cpp` | Audio, Shape, Lissajous, **Glow**, Strobe | Audio, Shape, Lissajous, Strobe, **Glow** | Glow after domain |
| `twist_cage.cpp` | Audio, Geometry, Twist, Projection, **Glow**, Camera | Audio, Geometry, Twist, Projection, Camera, **Glow** | Camera before Glow |

### Constellation Special Case

`constellation.cpp` has orphan params with no `SeparatorText` header. Add headers and reorder:

| Current | Target |
|---------|--------|
| (no header) Grid Scale, Anim Speed, Wander | Split into Geometry and Animation |
| Wave | Wave (unchanged) |
| (no header) Depth Layers | Merge into Geometry |
| Points | Points (unchanged) |
| Lines | Lines (unchanged) |
| Triangles | Triangles (unchanged) |
| Audio | Audio (move to 1st) |

Target order:
```
Audio: Base Freq, Max Freq, Gain, Contrast, Base Bright, Star Bins
Geometry: Grid Scale, Depth Layers
Wave: Wave Freq, Wave Amp, Wave Speed, Wave Center X, Wave Center Y, Wave Influence
Points: Point Shape, Point Size, Point Bright, Point Opacity
Lines: Line Width, Max Line Len, Line Opacity, Interpolate Line Color, lineGradient
Triangles: Fill Triangles, Fill Opacity, Fill Threshold
Animation: Anim Speed, Wander
```

### Excluded

`ripple_tank.cpp` - Audio section is conditional (only shown when `waveSource == 2`). The Audio block is inherently tied to the Wave Source combo and cannot be unconditionally first.

---

## Tasks

### Wave 1: All Parallel (no file overlap)

#### Task 1.1: Audio-to-first - simple batch A

**Files**: `src/effects/digital_shard.cpp`, `src/effects/shell.cpp`, `src/effects/chladni.cpp`, `src/effects/spark_flash.cpp`, `src/effects/vortex.cpp`, `src/effects/triskelion.cpp`

**Do**: For each file, cut the entire Audio `SeparatorText` block (from `SeparatorText("Audio")` through the last slider before the next `SeparatorText` or end of function) and paste it immediately after the local variable declarations at the top of the `Draw*Params` function. Preserve the blank line and comment style between sections. Refer to the Reordering Table for each file's target order.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.2: Audio-to-first - simple batch B

**Files**: `src/effects/muons.cpp`, `src/effects/fireworks.cpp`, `src/effects/laser_dance.cpp`, `src/effects/star_trail.cpp`, `src/effects/spin_cage.cpp`

**Do**: Same as Task 1.1. For `laser_dance.cpp`, the flat params (Speed, Freq Ratio, Brightness, Fold/Distance/Combine combos) stay in their current position after Audio - only the Audio block moves above them. Refer to the Reordering Table.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.3: Audio-to-first + Geometry reorder

**Files**: `src/effects/faraday.cpp`, `src/effects/polymorph.cpp`, `src/effects/polyhedral_mirror.cpp`, `src/effects/viscera.cpp`

**Do**: Move the Audio block to first position AND reorder other sections per the Reordering Table:
- `faraday`: Audio to 1st, Geometry block before Wave
- `polymorph`: Audio to 1st, Geometry block to 2nd (before Shape)
- `polyhedral_mirror`: Audio to 1st, Geometry block to 2nd (before Shape)
- `viscera`: Audio to 1st, Animation block after Height (move from position 2 to after all domain sections)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.4: Constellation restructure

**Files**: `src/effects/constellation.cpp`

**Do**: Restructure the `DrawConstellationParams` function per the Constellation Special Case section:
1. Move the Audio block (lines 284-295) to the top of the function, right after the local variable declarations
2. Add `ImGui::SeparatorText("Geometry");` before Grid Scale, then move Depth Layers (currently orphaned at line 248) to immediately after Grid Scale
3. Remove the orphaned `// Grid and animation` and `// Depth` comments
4. Wave, Points, Lines, Triangles sections stay in their current order but shift down
5. Add `ImGui::SeparatorText("Animation");` before Anim Speed and Wander at the bottom of the function (after Triangles)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.5: Swap Glow/Animation order

**Files**: `src/effects/filaments.cpp`, `src/effects/spiral_walk.cpp`

**Do**: In both files, swap the Glow and Animation `SeparatorText` blocks so Animation comes before Glow. Cut the Animation block and paste it before Glow.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.6: Section reordering batch A

**Files**: `src/effects/signal_frames.cpp`, `src/effects/dream_fractal.cpp`, `src/effects/galaxy.cpp`, `src/effects/protean_clouds.cpp`

**Do**: Reorder sections per the Reordering Table:
- `signal_frames`: Move Glow block from between Geometry and Sweep to after Animation
- `dream_fractal`: Move Julia block from after Animation to before Animation (between Fold and Animation)
- `galaxy`: Move Dust & Stars and Bulge blocks from after Animation to before Animation (between View and Animation)
- `protean_clouds`: Move Camera block from after Motion to before Motion (between Volume and Motion). Move Color block from between Volume and Motion to end of function (after Camera)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.7: Section reordering batch B

**Files**: `src/effects/data_traffic.cpp`, `src/effects/slashes.cpp`, `src/effects/arc_strobe.cpp`, `src/effects/twist_cage.cpp`

**Do**: Reorder sections per the Reordering Table:
- `data_traffic`: Move Behaviors block from after Animation to before Animation
- `slashes`: Move Geometry block from after Timing to before Timing (between Audio and Timing)
- `arc_strobe`: Move Glow block from between Lissajous and Strobe to after Strobe
- `twist_cage`: Move Camera block from after Glow to before Glow (between Projection and Glow)

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe --build build`
- [ ] All 26 effects' `SeparatorText` calls match the Target Order column in the Reordering Table
- [ ] Constellation has 7 `SeparatorText` sections (Audio, Geometry, Wave, Points, Lines, Triangles, Animation)
- [ ] No orphan params remain in constellation (all params are under a `SeparatorText` header)
