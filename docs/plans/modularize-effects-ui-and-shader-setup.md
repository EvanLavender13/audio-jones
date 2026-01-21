# Modularize: imgui_effects_transforms.cpp + shader_setup.cpp

Extracting cohesive modules from two large files to reduce file size and improve maintainability.

## Current State

| File | Lines | Functions | Issues |
|------|-------|-----------|--------|
| `src/ui/imgui_effects_transforms.cpp` | 1215 | 41 | CCN 25 in Voronoi/Phyllotaxis |
| `src/render/shader_setup.cpp` | 960 | 50 | 6 identical trail boost functions |

---

## Part A: imgui_effects_transforms.cpp Extraction

Split single 1215-line file into 6 category-based modules. All candidates share **zero inter-module dependencies**.

### A1. Style UI Module

**New file**: `src/ui/imgui_effects_style.cpp`

**Functions to extract** (12 total, ~425 lines):
- `DrawStylePixelation` (line 575) - pixelation/dithering controls
- `DrawStyleGlitch` (line 594) - CRT/VHS/analog glitch
- `DrawStyleToon` (line 648) - cartoon posterization
- `DrawStyleOilPaint` (line 672) - Kuwahara filter
- `DrawStyleWatercolor` (line 687) - paper/bleed effects
- `DrawStyleNeonGlow` (line 710) - edge neon glow
- `DrawStyleHeightfieldRelief` (line 744) - emboss/relief
- `DrawStyleAsciiArt` (line 878) - character rendering
- `DrawStyleCrossHatching` (line 914) - NPR strokes
- `DrawStyleBokeh` (line 936) - golden-angle blur
- `DrawStyleBloom` (line 955) - Kawase bloom
- `DrawStyleCategory` (line 975) - **public entry point**

**Static variables to move**:
- `sectionPixelation`, `sectionGlitch`, `sectionToon`, `sectionOilPaint`
- `sectionWatercolor`, `sectionNeonGlow`, `sectionHeightfieldRelief`
- `sectionAsciiArt`, `sectionCrossHatching`, `sectionBokeh`, `sectionBloom`

**Config headers**: `pixelation_config.h`, `glitch_config.h`, `toon_config.h`, `oil_paint_config.h`, `watercolor_config.h`, `neon_glow_config.h`, `heightfield_relief_config.h`, `ascii_art_config.h`, `cross_hatching_config.h`, `bokeh_config.h`, `bloom_config.h`

---

### A2. Warp UI Module

**New file**: `src/ui/imgui_effects_warp.cpp`

**Functions to extract** (10 total, ~250 lines):
- `DrawWarpSine` (line 250) - sine UV distortion
- `DrawWarpTexture` (line 268) - self-referential warp
- `DrawWarpGradientFlow` (line 303) - edge displacement
- `DrawWarpWaveRipple` (line 322) - pseudo-3D waves
- `DrawWarpMobius` (line 361) - conformal transform
- `DrawWarpChladniWarp` (line 398) - nodal warping
- `DrawWarpDomainWarp` (line 431) - FBM displacement
- `DrawWarpPhyllotaxisWarp` (line 455) - spiral displacement
- `DrawWarpCategory` (line 481) - **public entry point**

**Static variables to move**:
- `sectionSineWarp`, `sectionTextureWarp`, `sectionGradientFlow`
- `sectionWaveRipple`, `sectionMobius`, `sectionChladniWarp`
- `sectionDomainWarp`, `sectionPhyllotaxisWarp`

**Config headers**: `sine_warp_config.h`, `texture_warp_config.h`, `gradient_flow_config.h`, `wave_ripple_config.h`, `mobius_config.h`, `chladni_warp_config.h`, `domain_warp_config.h`, `phyllotaxis_warp_config.h`

---

### A3. Symmetry UI Module

**New file**: `src/ui/imgui_effects_symmetry.cpp`

**Functions to extract** (7 total, ~185 lines):
- `DrawSymmetryKaleidoscope` (line 62) - polar mirroring
- `DrawSymmetryKifs` (line 83) - KIFS fractal
- `DrawSymmetryPoincare` (line 112) - hyperbolic tiling
- `DrawSymmetryRadialPulse` (line 143) - polar distortion
- `DrawSymmetryMandelbox` (line 169) - box/sphere fold
- `DrawSymmetryTriangleFold` (line 211) - Sierpinski fold
- `DrawSymmetryCategory` (line 233) - **public entry point**

**Static variables to move**:
- `sectionKaleidoscope`, `sectionKifs`, `sectionPoincareDisk`
- `sectionRadialPulse`, `sectionMandelbox`, `sectionTriangleFold`

**Config headers**: `kaleidoscope_config.h`, `kifs_config.h`, `poincare_disk_config.h`, `radial_pulse_config.h`, `mandelbox_config.h`, `triangle_fold_config.h`

---

### A4. Cellular UI Module

**New file**: `src/ui/imgui_effects_cellular.cpp`

**Functions to extract** (4 total, ~210 lines):
- `DrawCellularVoronoi` (line 1002) - Voronoi cells (87 lines, CCN 25)
- `DrawCellularLatticeFold` (line 1090) - grid tiling
- `DrawCellularPhyllotaxis` (line 1115) - spiral cells (89 lines, CCN 25)
- `DrawCellularCategory` (line 1206) - **public entry point**

**Static variables to move**:
- `sectionVoronoi`, `sectionLatticeFold`, `sectionPhyllotaxis`

**Config headers**: `voronoi_config.h`, `lattice_fold_config.h`, `phyllotaxis_config.h`

---

### A5. Color UI Module

**New file**: `src/ui/imgui_effects_color.cpp`

**Functions to extract** (5 total, ~110 lines):
- `DrawColorColorGrade` (line 765) - color grading
- `DrawColorFalseColor` (line 799) - luminance mapping
- `DrawColorHalftone` (line 817) - CMYK dots
- `DrawColorPaletteQuantization` (line 841) - dithered reduction
- `DrawColorCategory` (line 865) - **public entry point**

**Static variables to move**:
- `sectionColorGrade`, `sectionFalseColor`, `sectionHalftone`, `sectionPaletteQuantization`

**Config headers**: `color_grade_config.h`, `false_color_config.h`, `halftone_config.h`, `palette_quantization_config.h`

---

### A6. Motion UI Module

**New file**: `src/ui/imgui_effects_motion.cpp`

**Functions to extract** (4 total, ~75 lines):
- `DrawMotionInfiniteZoom` (line 502) - layered zoom
- `DrawMotionRadialBlur` (line 521) - radial streak
- `DrawMotionDroste` (line 536) - recursive zoom
- `DrawMotionCategory` (line 564) - **public entry point**

**Static variables to move**:
- `sectionInfiniteZoom`, `sectionRadialStreak`, `sectionDrosteZoom`

**Config headers**: `infinite_zoom_config.h`, `radial_streak_config.h`, `droste_zoom_config.h`

---

## Part B: shader_setup.cpp Extraction

Split 960-line file into category-based modules with deduplication.

### B1. Trail Boost Setup Module

**New file**: `src/render/shader_setup_trail_boost.cpp`

**Functions to extract** (6 total, all identical pattern):
- `SetupTrailBoost` (line 209)
- `SetupCurlFlowTrailBoost` (line 217)
- `SetupCurlAdvectionTrailBoost` (line 225)
- `SetupAttractorFlowTrailBoost` (line 233)
- `SetupBoidsTrailBoost` (line 241)
- `SetupCymaticsTrailBoost` (line 249)

**Deduplication opportunity**: All 6 functions have identical bodies:
```cpp
void SetupXxxTrailBoost(PostEffect* pe) {
    BlendCompositorApply(&pe->blendCompositor, pe->xxxTrailBoostShader,
                         TrailMapGetTexture(&pe->xxx->trailMap),
                         pe->config->simulation.xxx);
}
```
Consider single parameterized function + 6 inline wrappers.

**Dependencies**: `blend_compositor.h`, `trail_map.h`

---

### B2. Bloom Multi-Pass Module

**New file**: `src/render/shader_setup_bloom.cpp`

**Functions to extract** (3 total):
- `BloomRenderPass` (line 900, static helper)
- `ApplyBloomPasses` (line 912) - orchestrates prefilter/downsample/upsample
- `SetupBloom` (line 767) - composite pass uniforms

**Cohesion**: Self-contained multi-pass render pipeline with internal helper.

**Dependencies**: `bloom_config.h`, raylib render functions

---

### B3. Warp Transform Setup Module

**New file**: `src/render/shader_setup_warp.cpp`

**Functions to extract** (8 total):
- `SetupSineWarp` (line 314)
- `SetupTextureWarp` (line 347)
- `SetupWaveRipple` (line 367)
- `SetupMobius` (line 391)
- `SetupDomainWarp` (line 823)
- `SetupChladniWarp` (line 700)
- `SetupRadialPulse` (line 652)
- `SetupPhyllotaxisWarp` (line 881)

**Config headers**: Corresponding warp config headers

---

### B4. Symmetry Transform Setup Module

**New file**: `src/render/shader_setup_symmetry.cpp`

**Functions to extract** (6 total):
- `SetupKaleido` (line 257)
- `SetupKifs` (line 271)
- `SetupLatticeFold` (line 298)
- `SetupMandelbox` (line 776)
- `SetupTriangleFold` (line 807)
- `SetupPoincareDisk` (line 488)

**Config headers**: Corresponding symmetry config headers

---

### B5. Stylization Setup Module

**New file**: `src/render/shader_setup_style.cpp`

**Functions to extract** (8 total):
- `SetupAsciiArt` (line 586)
- `SetupOilPaint` (line 605)
- `SetupWatercolor` (line 613)
- `SetupNeonGlow` (line 632)
- `SetupToon` (line 505)
- `SetupCrossHatching` (line 726)
- `SetupHalftone` (line 683)
- `SetupPixelation` (line 406)

**Config headers**: Corresponding style config headers

---

### B6. Color Processing Setup Module

**New file**: `src/render/shader_setup_color.cpp`

**Functions to extract** (5 total):
- `SetupColorGrade` (line 565)
- `SetupFalseColor` (line 671)
- `SetupPaletteQuantization` (line 745)
- `SetupChromatic` (line 470)
- `SetupGamma` (line 476)

**Dependencies**: `color_lut.h` (for `SetupFalseColor`)

---

### B7. Cellular Pattern Setup Module

**New file**: `src/render/shader_setup_cellular.cpp`

**Functions to extract** (2 total):
- `SetupVoronoi` (line 92)
- `SetupPhyllotaxis` (line 844)

**Deduplication opportunity**: Both share 9 identical intensity mode uniforms (uvDistort, edgeIso, centerIso, flatFill, organicFlow, edgeGlow, ratio, determinant, edgeDetect). Consider shared helper macro.

---

### B8. Remaining Core (stays in shader_setup.cpp)

Functions that remain in the original file:
- `GetTransformEffect` (line 14) - dispatch table, requires all setup headers
- `SetupFeedback` (line 126) - core feedback loop, 30 uniforms
- `SetupBlurH` / `SetupBlurV` (lines 193, 199) - blur passes
- `SetupClarity` (line 482) - local contrast
- `SetupGlitch` (line 417) - complex multi-mode effect
- `SetupBokeh` (line 756) - depth-of-field
- `SetupInfiniteZoom` (line 323) - layered zoom
- `SetupDrosteZoom` (line 548) - recursive zoom
- `SetupRadialStreak` (line 338) - radial blur
- `SetupGradientFlow` (line 535) - edge displacement

---

## Extraction Order

Both files have **independent** extraction candidates. Recommended order by impact:

### Part A (UI)
1. **A1 Style** - largest cluster (425 lines)
2. **A2 Warp** - second largest (250 lines)
3. **A4 Cellular** - high-complexity functions
4. **A3 Symmetry** - medium cluster (185 lines)
5. **A5 Color** - small cluster (110 lines)
6. **A6 Motion** - smallest cluster (75 lines)

### Part B (Shader Setup)
1. **B1 Trail Boost** - code deduplication (6 identical functions)
2. **B2 Bloom** - self-contained multi-pass
3. **B3 Warp** - largest shader cluster (8 functions)
4. **B4 Symmetry** - medium cluster (6 functions)
5. **B5 Stylization** - medium cluster (8 functions)
6. **B6 Color** - small cluster (5 functions)
7. **B7 Cellular** - shared intensity helper opportunity

---

## Implementation Notes

### Header Changes

**`src/ui/imgui_effects_transforms.h`** - Already exports all 6 `Draw*Category` functions. No changes needed.

**`src/render/shader_setup.h`** - Add forward declarations for new module headers or include them conditionally.

### CMakeLists.txt

Add new source files to `src/ui/` and `src/render/` globbing or explicit lists:
```cmake
# In src/ui/CMakeLists.txt or main CMakeLists.txt
set(UI_SOURCES
    ...
    imgui_effects_style.cpp
    imgui_effects_warp.cpp
    imgui_effects_symmetry.cpp
    imgui_effects_cellular.cpp
    imgui_effects_color.cpp
    imgui_effects_motion.cpp
)

set(RENDER_SOURCES
    ...
    shader_setup_trail_boost.cpp
    shader_setup_bloom.cpp
    shader_setup_warp.cpp
    shader_setup_symmetry.cpp
    shader_setup_style.cpp
    shader_setup_color.cpp
    shader_setup_cellular.cpp
)
```

### Build Verification

After each module extraction:
1. Build: `cmake --build build`
2. Verify no duplicate symbol errors
3. Run application to confirm runtime behavior unchanged

### Post-Extraction

Run `/sync-architecture` to update module documentation.

---

## Blockers

None identified. All extraction candidates are fully independent with no circular dependencies or shared mutable state.
