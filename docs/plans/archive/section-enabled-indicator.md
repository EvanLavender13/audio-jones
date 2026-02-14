# Section Enabled Indicator

Add visual indication to collapsible effect section headers showing whether the effect is enabled. When enabled, the header looks as it does today (full accent bar, white text). When disabled, the accent bar dims to ~20% opacity and the label/arrow text fades to muted blue-gray — like a neon sign switched off.

## Design

### Approach

Modify `DrawSectionHeader` to accept an optional `isEnabled` parameter. When false:
- Accent bar color: `SetColorAlpha(accentColor, 50)` (~20% of full)
- Label text: `IM_COL32(107, 107, 123, 255)` (#6B6B7B — muted blue-gray)
- Collapse arrow: same muted color as label
- Background gradient and border: unchanged

When true (or omitted): current behavior, no visual change.

### API Change

```
// imgui_panels.h — add isEnabled with default true
bool DrawSectionHeader(const char *label, ImU32 accentColor, bool *isOpen, bool isEnabled = true);
bool DrawSectionBegin(const char *label, ImU32 accentColor, bool *isOpen, bool isEnabled = true);
```

### Theme Constant

Add to `theme.h`:
```
constexpr ImU32 TEXT_DISABLED_U32 = IM_COL32(107, 107, 123, 255);
```

### Call Site Pattern

Every effect section currently does:
```cpp
if (DrawSectionBegin("Name", categoryGlow, &sectionName)) {
    ImGui::Checkbox("Enabled##name", &e->name.enabled);
```

Changes to:
```cpp
if (DrawSectionBegin("Name", categoryGlow, &sectionName, e->name.enabled)) {
    ImGui::Checkbox("Enabled##name", &e->name.enabled);
```

Non-effect sections (drawable controls, analysis, LFO) omit the parameter and get default `true`.

---

## Tasks

### Wave 1: Core widget + theme

#### Task 1.1: Add theme constant and modify DrawSectionHeader

**Files**: `src/ui/theme.h`, `src/ui/imgui_panels.h`, `src/ui/imgui_widgets.cpp`
**Creates**: `TEXT_DISABLED_U32` constant, updated `DrawSectionHeader`/`DrawSectionBegin` signatures

**Do**:
1. In `theme.h`: add `constexpr ImU32 TEXT_DISABLED_U32 = IM_COL32(107, 107, 123, 255);` in the Text colors section (after `TEXT_SECONDARY_U32`)
2. In `imgui_panels.h`: add `bool isEnabled = true` as 4th param to both `DrawSectionHeader` and `DrawSectionBegin`
3. In `imgui_widgets.cpp` `DrawSectionHeader`: add `bool isEnabled` parameter. Use it to pick colors:
   - `const ImU32 barColor = isEnabled ? accentColor : SetColorAlpha(accentColor, 50);`
   - `const ImU32 textColor = isEnabled ? Theme::TEXT_PRIMARY_U32 : Theme::TEXT_DISABLED_U32;`
   - Apply `barColor` to the accent bar `AddRectFilled`
   - Apply `textColor` to both the arrow and label `AddText` calls
4. In `imgui_widgets.cpp` `DrawSectionBegin`: add `bool isEnabled` parameter, pass through to `DrawSectionHeader`

**Verify**: `cmake.exe --build build` compiles. All existing call sites still work (default parameter = true).

---

### Wave 2: Update effect call sites

All tasks in this wave are independent (no file overlap).

#### Task 2.1: Transform category — Symmetry

**Files**: `src/ui/imgui_effects_symmetry.cpp`
**Depends on**: Wave 1

**Do**: For each `DrawSectionBegin` call, append the effect's `.enabled` field as 4th arg:
- `"Kaleidoscope"` → `e->kaleidoscope.enabled`
- `"KIFS"` → `e->kifs.enabled`
- `"Poincare Disk"` → `e->poincareDisk.enabled`
- `"Mandelbox"` → `e->mandelbox.enabled`
- `"Triangle Fold"` → `e->triangleFold.enabled`
- `"Moire Interference"` → `e->moireInterference.enabled`
- `"Radial IFS"` → `e->radialIfs.enabled`

**Verify**: Compiles.

#### Task 2.2: Transform category — Warp

**Files**: `src/ui/imgui_effects_warp.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled` to each `DrawSectionBegin`:
- `"Sine Warp"` → `e->sineWarp.enabled`
- `"Texture Warp"` → `e->textureWarp.enabled`
- `"Gradient Flow"` → `e->gradientFlow.enabled`
- `"Wave Ripple"` → `e->waveRipple.enabled`
- `"Mobius"` → `e->mobius.enabled`
- `"Chladni Warp"` → `e->chladniWarp.enabled`
- `"Domain Warp"` → `e->domainWarp.enabled`
- `"Surface Warp"` → `e->surfaceWarp.enabled`
- `"Interference Warp"` → `e->interferenceWarp.enabled`
- `"Circuit Board"` → `e->circuitBoard.enabled`
- `"Corridor Warp"` → `e->corridorWarp.enabled`
- `"Radial Pulse"` → `e->radialPulse.enabled`
- `"Tone Warp"` → `e->toneWarp.enabled`

**Verify**: Compiles.

#### Task 2.3: Transform category — Cellular

**Files**: `src/ui/imgui_effects_cellular.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Voronoi"` → `e->voronoi.enabled`
- `"Lattice Fold"` → `e->latticeFold.enabled`
- `"Phyllotaxis"` → `e->phyllotaxis.enabled`
- `"Multi-Scale Grid"` → `e->multiScaleGrid.enabled`
- `"Dot Matrix"` → `e->dotMatrix.enabled`

**Verify**: Compiles.

#### Task 2.4: Transform category — Motion

**Files**: `src/ui/imgui_effects_motion.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Infinite Zoom"` → `e->infiniteZoom.enabled`
- `"Radial Blur"` → `e->radialStreak.enabled`
- `"Droste Zoom"` → `e->drosteZoom.enabled`
- `"Density Wave Spiral"` → `e->densityWaveSpiral.enabled`
- `"Shake"` → `e->shake.enabled`
- `"Relativistic Doppler"` → `e->relativisticDoppler.enabled`

**Verify**: Compiles.

#### Task 2.5: Transform category — Artistic

**Files**: `src/ui/imgui_effects_artistic.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Oil Paint"` → `e->oilPaint.enabled`
- `"Watercolor"` → `e->watercolor.enabled`
- `"Impressionist"` → `e->impressionist.enabled`
- `"Ink Wash"` → `e->inkWash.enabled`
- `"Pencil Sketch"` → `e->pencilSketch.enabled`
- `"Cross-Hatching"` → `e->crossHatching.enabled`

**Verify**: Compiles.

#### Task 2.6: Transform category — Graphic

**Files**: `src/ui/imgui_effects_graphic.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Toon"` → `e->toon.enabled`
- `"Neon Glow"` → `e->neonGlow.enabled`
- `"Kuwahara"` → `e->kuwahara.enabled`
- `"Halftone"` → `e->halftone.enabled`
- `"Disco Ball"` → `e->discoBall.enabled`
- `"LEGO Bricks"` → `e->legoBricks.enabled`

**Verify**: Compiles.

#### Task 2.7: Transform category — Retro

**Files**: `src/ui/imgui_effects_retro.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Pixelation"` → `e->pixelation.enabled`
- `"Glitch"` → `e->glitch.enabled`
- `"ASCII Art"` → `e->asciiArt.enabled`
- `"Matrix Rain"` → `e->matrixRain.enabled`
- `"Synthwave"` → `e->synthwave.enabled`
- `"CRT"` → `e->crt.enabled`

**Verify**: Compiles.

#### Task 2.8: Transform category — Optical

**Files**: `src/ui/imgui_effects_optical.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Bloom"` → `e->bloom.enabled`
- `"Bokeh"` → `e->bokeh.enabled`
- `"Anamorphic Streak"` → `e->anamorphicStreak.enabled`
- `"Heightfield Relief"` → `e->heightfieldRelief.enabled`
- `"Phi Blur"` → `e->phiBlur.enabled`

**Verify**: Compiles.

#### Task 2.9: Transform category — Color

**Files**: `src/ui/imgui_effects_color.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Color Grade"` → `e->colorGrade.enabled`
- `"False Color"` → `e->falseColor.enabled`
- `"Palette Quantization"` → `e->paletteQuantization.enabled`

**Verify**: Compiles.

#### Task 2.10: Generators — Geometric

**Files**: `src/ui/imgui_effects_gen_geometric.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Signal Frames"` → `e->signalFrames.enabled`
- `"Arc Strobe"` → `e->arcStrobe.enabled`
- `"Pitch Spiral"` → `e->pitchSpiral.enabled`
- `"Spectral Arcs"` → `e->spectralArcs.enabled`

**Verify**: Compiles.

#### Task 2.11: Generators — Filament

**Files**: `src/ui/imgui_effects_gen_filament.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Constellation"` → `e->constellation.enabled`
- `"Filaments"` → `e->filaments.enabled`
- `"Slashes"` → `e->slashes.enabled`
- `"Muons"` → `e->muons.enabled`
- `"Attractor Lines"` → `e->attractorLines.enabled`

**Verify**: Compiles.

#### Task 2.12: Generators — Texture

**Files**: `src/ui/imgui_effects_gen_texture.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Plasma"` → `e->plasma.enabled`
- `"Interference"` → `e->interference.enabled`
- `"Moire Generator"` → `e->moireGenerator.enabled`
- `"Scan Bars"` → `e->scanBars.enabled`
- `"Glyph Field"` → `e->glyphField.enabled`
- `"Motherboard"` → `e->motherboard.enabled`

**Verify**: Compiles.

#### Task 2.13: Generators — Atmosphere

**Files**: `src/ui/imgui_effects_gen_atmosphere.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled`:
- `"Nebula"` → `e->nebula.enabled`
- `"Solid Color"` → `e->solidColor.enabled`

**Verify**: Compiles.

#### Task 2.14: Simulations + Flow Field

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 1

**Do**: Append `.enabled` to simulation and flow field sections:
- `"Flow Field"` → `e->flowField.enabled`
- `"Physarum"` → `e->physarum.enabled`
- `"Curl Flow"` → `e->curlFlow.enabled`
- `"Attractor Flow"` → `e->attractorFlow.enabled`
- `"Boids"` → `e->boids.enabled`
- `"Curl Advection"` → `e->curlAdvection.enabled`
- `"Cymatics"` → `e->cymatics.enabled`
- `"Particle Life"` → `e->particleLife.enabled`

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Sections for disabled effects show dimmed accent bar and muted text
- [ ] Sections for enabled effects look unchanged (full accent bar, white text)
- [ ] Non-effect sections (drawable controls, analysis, LFO) are unaffected (default true)
- [ ] Toggling the Enabled checkbox inside a section immediately updates the header appearance
