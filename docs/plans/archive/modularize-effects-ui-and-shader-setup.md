# Modularize: imgui_effects_transforms.cpp

Split single 1283-line file into 6 category-based UI modules. All candidates share **zero inter-module dependencies**.

## Current State

| File | Lines | Functions | Issues |
|------|-------|-----------|--------|
| `src/ui/imgui_effects_transforms.cpp` | 1283 | 42 | CCN 25 in Voronoi/Phyllotaxis |

---

## Phase 1: Style UI Module

**Goal**: Extract style/NPR effects to `src/ui/imgui_effects_style.cpp` (~400 lines).

**Build**:
- Create `src/ui/imgui_effects_style.cpp`
- Move functions: `DrawStylePixelation`, `DrawStyleGlitch`, `DrawStyleToon`, `DrawStyleOilPaint`, `DrawStyleWatercolor`, `DrawStyleNeonGlow`, `DrawStyleHeightfieldRelief`, `DrawStyleAsciiArt`, `DrawStyleCrossHatching`, `DrawStyleBokeh`, `DrawStyleBloom`, `DrawStyleCategory`
- Move static section variables: `sectionPixelation`, `sectionGlitch`, `sectionToon`, `sectionOilPaint`, `sectionWatercolor`, `sectionNeonGlow`, `sectionHeightfieldRelief`, `sectionAsciiArt`, `sectionCrossHatching`, `sectionBokeh`, `sectionBloom`
- Add includes: `imgui.h`, `ui/imgui_panels.h`, `ui/imgui_effects_transforms.h`, `ui/theme.h`, `ui/ui_units.h`, `ui/modulatable_slider.h`, `config/effect_config.h`, `config/bokeh_config.h`, `config/bloom_config.h`, `automation/mod_sources.h`

**Done when**: Build succeeds, `DrawStyleCategory` renders all 11 style effects.

---

## Phase 2: Warp UI Module

**Goal**: Extract warp/distortion effects to `src/ui/imgui_effects_warp.cpp` (~250 lines).

**Build**:
- Create `src/ui/imgui_effects_warp.cpp`
- Move functions: `DrawWarpSine`, `DrawWarpTexture`, `DrawWarpGradientFlow`, `DrawWarpWaveRipple`, `DrawWarpMobius`, `DrawWarpChladniWarp`, `DrawWarpDomainWarp`, `DrawWarpPhyllotaxisWarp`, `DrawWarpCategory`
- Move static section variables: `sectionSineWarp`, `sectionTextureWarp`, `sectionGradientFlow`, `sectionWaveRipple`, `sectionMobius`, `sectionChladniWarp`, `sectionDomainWarp`, `sectionPhyllotaxisWarp`
- Add includes: same base set plus `config/domain_warp_config.h`, `config/phyllotaxis_warp_config.h`

**Done when**: Build succeeds, `DrawWarpCategory` renders all 8 warp effects.

---

## Phase 3: Symmetry UI Module

**Goal**: Extract symmetry/folding effects to `src/ui/imgui_effects_symmetry.cpp` (~280 lines).

**Build**:
- Create `src/ui/imgui_effects_symmetry.cpp`
- Move functions: `DrawSymmetryKaleidoscope`, `DrawSymmetryKifs`, `DrawSymmetryPoincare`, `DrawSymmetryRadialPulse`, `DrawSymmetryMandelbox`, `DrawSymmetryTriangleFold`, `DrawSymmetryMoireInterference`, `DrawSymmetryCategory`
- Move static section variables: `sectionKaleidoscope`, `sectionKifs`, `sectionPoincareDisk`, `sectionRadialPulse`, `sectionMandelbox`, `sectionTriangleFold`, `sectionMoireInterference`
- Add includes: same base set plus `config/kaleidoscope_config.h`, `config/kifs_config.h`, `config/mandelbox_config.h`, `config/triangle_fold_config.h`, `config/moire_interference_config.h`

**Done when**: Build succeeds, `DrawSymmetryCategory` renders all 7 symmetry effects.

---

## Phase 4: Cellular UI Module

**Goal**: Extract cellular pattern effects to `src/ui/imgui_effects_cellular.cpp` (~215 lines).

**Build**:
- Create `src/ui/imgui_effects_cellular.cpp`
- Move functions: `DrawCellularVoronoi`, `DrawCellularLatticeFold`, `DrawCellularPhyllotaxis`, `DrawCellularCategory`
- Move static section variables: `sectionVoronoi`, `sectionLatticeFold`, `sectionPhyllotaxis`
- Add includes: same base set plus `config/voronoi_config.h`, `config/lattice_fold_config.h`, `config/phyllotaxis_config.h`

**Done when**: Build succeeds, `DrawCellularCategory` renders all 3 cellular effects.

---

## Phase 5: Color UI Module

**Goal**: Extract color processing effects to `src/ui/imgui_effects_color.cpp` (~115 lines).

**Build**:
- Create `src/ui/imgui_effects_color.cpp`
- Move functions: `DrawColorColorGrade`, `DrawColorFalseColor`, `DrawColorHalftone`, `DrawColorPaletteQuantization`, `DrawColorCategory`
- Move static section variables: `sectionColorGrade`, `sectionFalseColor`, `sectionHalftone`, `sectionPaletteQuantization`
- Add includes: same base set plus `config/false_color_config.h`, `config/halftone_config.h`, `config/cross_hatching_config.h`, `config/palette_quantization_config.h`

**Done when**: Build succeeds, `DrawColorCategory` renders all 4 color effects.

---

## Phase 6: Motion UI Module

**Goal**: Extract motion effects to `src/ui/imgui_effects_motion.cpp` (~110 lines).

**Build**:
- Create `src/ui/imgui_effects_motion.cpp`
- Move functions: `DrawMotionInfiniteZoom`, `DrawMotionRadialBlur`, `DrawMotionDroste`, `DrawMotionDensityWaveSpiral`, `DrawMotionCategory`
- Move static section variables: `sectionInfiniteZoom`, `sectionRadialStreak`, `sectionDrosteZoom`, `sectionDensityWaveSpiral`
- Add includes: same base set

**Done when**: Build succeeds, `DrawMotionCategory` renders all 4 motion effects.

---

## Phase 7: Cleanup Original File

**Goal**: Remove extracted code from `imgui_effects_transforms.cpp`.

**Build**:
- Delete all moved functions and static variables from original file
- Keep only includes needed by remaining code (if any)
- Original file should be empty or contain only shared utilities

**Done when**: Build succeeds, original file reduced to ~0 lines, all effects still render correctly.

---

## Phase 8: Update add-effect Skill

**Goal**: Update `.claude/skills/add-effect/SKILL.md` to reference category-based files.

**Build**:
- In Phase 6 (UI Panel), change `src/ui/imgui_effects_transforms.cpp` to `src/ui/imgui_effects_{category}.cpp`
- Update File Summary table: change `imgui_effects_transforms.cpp` to `imgui_effects_{category}.cpp`

**Done when**: Skill references the new category-based file structure.

---

## Implementation Notes

### Header Changes

**`src/ui/imgui_effects_transforms.h`** - Already exports all 6 `Draw*Category` functions. No changes needed.

### Build Verification

After each phase:
1. Build: `cmake --build build`
2. Verify no duplicate symbol errors
3. Run application to confirm UI unchanged

---

## Blockers

None identified. All extraction candidates are fully independent with no circular dependencies or shared mutable state.
