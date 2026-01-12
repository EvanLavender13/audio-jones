# Modularize: imgui_effects_transforms.cpp

Extracting 23 effect section helpers from 5 Draw*Category functions to reduce complexity while maintaining single-file cohesion.

## Current State

- `src/ui/imgui_effects_transforms.cpp` - 720 lines, 5 functions
- Primary structs: `EffectConfig`, individual effect configs (KaleidoscopeConfig, GlitchConfig, etc.)
- 23 static `sectionX` bools track UI collapse state (lines 15-37)

### Complexity Before

| Function | Lines | CCN | Effect Sections |
|----------|-------|-----|-----------------|
| DrawStyleCategory | 255 | 50 | 9 |
| DrawCellularCategory | 110 | 32 | 2 |
| DrawWarpCategory | 128 | 28 | 5 |
| DrawSymmetryCategory | 121 | 22 | 4 |
| DrawMotionCategory | 60 | 15 | 3 |

## Extraction Candidates

### 1. Symmetry Helpers

**Functions to extract** (called only by `DrawSymmetryCategory`):

- `DrawSymmetryKaleidoscope` (lines 43-77) - kaleidoscope UI: segments, spin, twist, focal offset, warp
- `DrawSymmetryKifs` (lines 81-103) - KIFS UI: iterations, scale, offset, spin, twist, folds
- `DrawSymmetryPoincare` (lines 107-133) - Poincare disk UI: tile params, translation, rotation
- `DrawSymmetryRadialPulse` (lines 137-158) - Radial pulse UI: freq, amp, segments, swirl, petal, spiral

**Primary structs**: `KaleidoscopeConfig`, `KifsConfig`, `PoincareDiskConfig`, `RadialPulseConfig`

**Signature pattern**:
```cpp
static void DrawSymmetryKaleidoscope(EffectConfig* e, const ModSources* modSources, ImU32 categoryGlow);
```

---

### 2. Warp Helpers

**Functions to extract** (called only by `DrawWarpCategory`):

- `DrawWarpSine` (lines 166-180) - sine warp UI: octaves, strength, speed, rotation, scale
- `DrawWarpTexture` (lines 184-199) - texture warp UI: channel mode, strength, iterations
- `DrawWarpGradientFlow` (lines 203-217) - gradient flow UI: strength, iterations, angle, edge weight
- `DrawWarpWaveRipple` (lines 221-253) - wave ripple UI: octaves, strength, freq, steepness, origin, shading
- `DrawWarpMobius` (lines 257-288) - Mobius UI: spiral, zoom, fixed points, point motion

**Primary structs**: `SineWarpConfig`, `TextureWarpConfig`, `GradientFlowConfig`, `WaveRippleConfig`, `MobiusConfig`

---

### 3. Motion Helpers

**Functions to extract** (called only by `DrawMotionCategory`):

- `DrawMotionInfiniteZoom` (lines 295-309) - infinite zoom UI: speed, depth, layers, spiral angle/twist
- `DrawMotionRadialBlur` (lines 313-322) - radial blur UI: samples, streak length
- `DrawMotionDroste` (lines 326-349) - Droste zoom UI: speed, scale, spiral, shear, masking, branches

**Primary structs**: `InfiniteZoomConfig`, `RadialStreakConfig`, `DrosteZoomConfig`

---

### 4. Style Helpers

**Functions to extract** (called only by `DrawStyleCategory`):

- `DrawStylePixelation` (lines 357-371) - pixelation UI: cell count, posterize, dither
- `DrawStyleGlitch` (lines 375-429) - glitch UI: CRT, analog, digital, VHS modes, overlay
- `DrawStyleToon` (lines 433-451) - toon UI: levels, edge threshold/softness, brush stroke
- `DrawStyleOilPaint` (lines 455-465) - oil paint UI: radius
- `DrawStyleWatercolor` (lines 469-487) - watercolor UI: edge darkening, granulation, paper, softness, bleed, color levels
- `DrawStyleNeonGlow` (lines 491-518) - neon glow UI: color, intensity, edge threshold, visibility, advanced
- `DrawStyleHeightfieldRelief` (lines 522-538) - heightfield UI: intensity, scale, light angle/height, shininess
- `DrawStyleColorGrade` (lines 542-571) - color grade UI: hue, saturation, brightness, contrast, temperature, lift/gamma/gain
- `DrawStyleAsciiArt` (lines 575-606) - ASCII art UI: cell size, color mode, foreground/background, invert

**Primary structs**: `PixelationConfig`, `GlitchConfig`, `ToonConfig`, `OilPaintConfig`, `WatercolorConfig`, `NeonGlowConfig`, `HeightfieldReliefConfig`, `ColorGradeConfig`, `AsciiArtConfig`

---

### 5. Cellular Helpers

**Functions to extract** (called only by `DrawCellularCategory`):

- `DrawCellularVoronoi` (lines 614-695) - voronoi UI: scale, speed, 9 intensity toggles, blend mix, iso settings
- `DrawCellularLatticeFold` (lines 700-718) - lattice fold UI: cell type, cell scale, spin

**Primary structs**: `VoronoiConfig`, `LatticeFoldConfig`

---

## Extraction Order

1. **Style helpers** (9 functions) - highest CCN reduction (50 → ~10)
2. **Cellular helpers** (2 functions) - second highest CCN (32 → ~8)
3. **Warp helpers** (5 functions) - CCN 28 → ~8
4. **Symmetry helpers** (4 functions) - CCN 22 → ~8
5. **Motion helpers** (3 functions) - CCN 15 → ~6, lowest priority

## Blockers to Resolve

None. All helpers remain static within `imgui_effects_transforms.cpp`. Section bools stay at file scope.

## Implementation Notes

- Each helper is `static` (internal linkage), declared before its caller
- Helpers take `(EffectConfig* e, const ModSources* modSources, ImU32 categoryGlow)` or subset as needed
- Section bools (`sectionKaleidoscope`, etc.) remain at file top; each helper accesses the relevant one directly
- The Draw*Category functions become orchestrators:
  ```cpp
  void DrawStyleCategory(EffectConfig* e, const ModSources* modSources)
  {
      const ImU32 categoryGlow = Theme::GetSectionGlow(4);
      DrawCategoryHeader("Style", categoryGlow);
      DrawStylePixelation(e, modSources, categoryGlow);
      ImGui::Spacing();
      DrawStyleGlitch(e, modSources, categoryGlow);
      // ... etc
  }
  ```
- No new files created; no header changes
- Build verification: `cmake.exe --build build` after each category's helpers
- Run `/sync-architecture` when complete

## Phase 6: Update add-effect Skill

Modify `.claude/skills/add-effect/SKILL.md` Phase 6 (UI Panel) to reflect the new helper pattern:

**Replace** the current "Add section" instruction (lines 178-187) with:

```markdown
2. **Add helper function** before the appropriate `Draw*Category()`:
   ```cpp
   static void Draw{Category}{EffectName}(EffectConfig* e, const ModSources* modSources, ImU32 categoryGlow)
   {
       if (DrawSectionBegin("Effect Name", categoryGlow, &section{EffectName})) {
           const bool wasEnabled = e->{effectName}.enabled;
           ImGui::Checkbox("Enabled##{id}", &e->{effectName}.enabled);
           if (!wasEnabled && e->{effectName}.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_{EFFECT_NAME}); }
           if (e->{effectName}.enabled) {
               ModulatableSlider("Param", &e->{effectName}.param, "effectName.param", "%.2f", modSources);
           }
           DrawSectionEnd();
       }
   }
   ```

3. **Add helper call** in the orchestrator with spacing:
   ```cpp
   ImGui::Spacing();
   Draw{Category}{EffectName}(e, modSources, categoryGlow);
   ```
```

This ensures future effect additions follow the helper function pattern.
