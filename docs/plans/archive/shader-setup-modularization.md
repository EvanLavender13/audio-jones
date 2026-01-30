# Shader Setup Modularization

Split `src/render/shader_setup.cpp` (1403 lines, 56 Setup functions) into category-based modules matching the existing effect taxonomy from `docs/effects.md`. Reduces merge conflicts and improves maintainability.

## Module Structure

| Module | Functions | Est. Lines |
|--------|-----------|------------|
| shader_setup.cpp (core) | Feedback, BlurH/V, Chromatic, Gamma, Clarity, 7 TrailBoosts, GetTransformEffect, Apply* helpers | ~350 |
| shader_setup_symmetry.cpp | Kaleido, KIFS, PoincareDisk, RadialPulse, Mandelbox, TriangleFold, MoireInterference | ~180 |
| shader_setup_warp.cpp | SineWarp, TextureWarp, WaveRipple, Mobius, GradientFlow, ChladniWarp, DomainWarp, SurfaceWarp | ~200 |
| shader_setup_cellular.cpp | Voronoi, LatticeFold, Phyllotaxis, DiscoBall | ~120 |
| shader_setup_motion.cpp | InfiniteZoom, RadialStreak, DrosteZoom, DensityWaveSpiral | ~100 |
| shader_setup_style.cpp | Pixelation, Glitch, Toon, HeightfieldRelief, AsciiArt, OilPaint, Watercolor, NeonGlow, CrossHatching, Bokeh, Bloom, PencilSketch, MatrixRain, Impressionist, Kuwahara, InkWash | ~400 |
| shader_setup_color.cpp | ColorGrade, FalseColor, Halftone, PaletteQuantization | ~80 |

## Function-to-Module Mapping

### shader_setup.cpp (core)

```
SetupFeedback (158-254)
SetupBlurH (256-260)
SetupBlurV (262-273)
SetupTrailBoost (275-281)
SetupCurlFlowTrailBoost (283-289)
SetupCurlAdvectionTrailBoost (291-297)
SetupAttractorFlowTrailBoost (299-305)
SetupBoidsTrailBoost (307-313)
SetupParticleLifeTrailBoost (315-321)
SetupCymaticsTrailBoost (323-329)
SetupChromatic (622-626)
SetupGamma (628-632)
SetupClarity (634-638)
GetTransformEffect (16-122)
BloomRenderPass (1249-1259) [static helper]
ApplyBloomPasses (1261-1309)
ApplyHalfResEffect (1311-1351)
ApplyHalfResOilPaint (1353-1403)
```

### shader_setup_symmetry.cpp

```
SetupKaleido (331-343)
SetupKifs (345-370)
SetupPoincareDisk (640-655)
SetupRadialPulse (825-842)
SetupMandelbox (949-978)
SetupTriangleFold (980-994)
SetupMoireInterference (1056-1077)
```

### shader_setup_warp.cpp

```
SetupSineWarp (388-396)
SetupTextureWarp (422-440)
SetupWaveRipple (442-466)
SetupMobius (468-481)
SetupGradientFlow (687-699)
SetupChladniWarp (873-897)
SetupDomainWarp (996-1015)
SetupSurfaceWarp (1229-1247)
```

### shader_setup_cellular.cpp

```
SetupVoronoi (124-156)
SetupLatticeFold (372-386)
SetupPhyllotaxis (1017-1054)
SetupDiscoBall (1204-1227)
```

### shader_setup_motion.cpp

```
SetupInfiniteZoom (398-411)
SetupRadialStreak (413-420)
SetupDrosteZoom (701-716)
SetupDensityWaveSpiral (1079-1100)
```

### shader_setup_style.cpp

```
SetupPixelation (483-492)
SetupGlitch (494-620)
SetupToon (657-670)
SetupHeightfieldRelief (672-685)
SetupAsciiArt (739-756)
SetupOilPaint (758-782)
ApplyOilPaintStrokePass (784-797) [if exists, else inline]
SetupWatercolor (784-797)
SetupNeonGlow (799-823)
SetupCrossHatching (899-916)
SetupBokeh (929-938)
SetupBloom (940-947)
SetupPencilSketch (1102-1125)
SetupMatrixRain (1127-1151)
SetupImpressionist (1153-1178)
SetupKuwahara (1180-1186)
SetupInkWash (1188-1202)
```

### shader_setup_color.cpp

```
SetupColorGrade (718-737)
SetupFalseColor (844-854)
SetupHalftone (856-871)
SetupPaletteQuantization (918-927)
```

---

## Tasks

### Wave 1: Create Category Headers

All category headers can be created in parallel. Each declares only the Setup functions for that category.

#### Task 1.1: Create shader_setup_symmetry.h

**Files**: `src/render/shader_setup_symmetry.h`

**Build**:
1. Create header with include guard `SHADER_SETUP_SYMMETRY_H`
2. Include `"post_effect.h"` (forward-declares PostEffect)
3. Declare: `SetupKaleido`, `SetupKifs`, `SetupPoincareDisk`, `SetupRadialPulse`, `SetupMandelbox`, `SetupTriangleFold`, `SetupMoireInterference`

**Verify**: Header compiles when included from a test .cpp

---

#### Task 1.2: Create shader_setup_warp.h

**Files**: `src/render/shader_setup_warp.h`

**Build**:
1. Create header with include guard `SHADER_SETUP_WARP_H`
2. Include `"post_effect.h"`
3. Declare: `SetupSineWarp`, `SetupTextureWarp`, `SetupWaveRipple`, `SetupMobius`, `SetupGradientFlow`, `SetupChladniWarp`, `SetupDomainWarp`, `SetupSurfaceWarp`

**Verify**: Header compiles

---

#### Task 1.3: Create shader_setup_cellular.h

**Files**: `src/render/shader_setup_cellular.h`

**Build**:
1. Create header with include guard `SHADER_SETUP_CELLULAR_H`
2. Include `"post_effect.h"`
3. Declare: `SetupVoronoi`, `SetupLatticeFold`, `SetupPhyllotaxis`, `SetupDiscoBall`

**Verify**: Header compiles

---

#### Task 1.4: Create shader_setup_motion.h

**Files**: `src/render/shader_setup_motion.h`

**Build**:
1. Create header with include guard `SHADER_SETUP_MOTION_H`
2. Include `"post_effect.h"`
3. Declare: `SetupInfiniteZoom`, `SetupRadialStreak`, `SetupDrosteZoom`, `SetupDensityWaveSpiral`

**Verify**: Header compiles

---

#### Task 1.5: Create shader_setup_style.h

**Files**: `src/render/shader_setup_style.h`

**Build**:
1. Create header with include guard `SHADER_SETUP_STYLE_H`
2. Include `"post_effect.h"`
3. Declare: `SetupPixelation`, `SetupGlitch`, `SetupToon`, `SetupHeightfieldRelief`, `SetupAsciiArt`, `SetupOilPaint`, `SetupWatercolor`, `SetupNeonGlow`, `SetupCrossHatching`, `SetupBokeh`, `SetupBloom`, `SetupPencilSketch`, `SetupMatrixRain`, `SetupImpressionist`, `SetupKuwahara`, `SetupInkWash`

**Verify**: Header compiles

---

#### Task 1.6: Create shader_setup_color.h

**Files**: `src/render/shader_setup_color.h`

**Build**:
1. Create header with include guard `SHADER_SETUP_COLOR_H`
2. Include `"post_effect.h"`
3. Declare: `SetupColorGrade`, `SetupFalseColor`, `SetupHalftone`, `SetupPaletteQuantization`

**Verify**: Header compiles

---

### Wave 2: Create Category Implementations

All implementations can be created in parallel. Each extracts functions from shader_setup.cpp.

#### Task 2.1: Create shader_setup_symmetry.cpp

**Files**: `src/render/shader_setup_symmetry.cpp`

**Build**:
1. Include `"shader_setup_symmetry.h"`, `"post_effect.h"`, `<math.h>`
2. Move these functions from shader_setup.cpp (copy exact implementation):
   - `SetupKaleido`
   - `SetupKifs`
   - `SetupPoincareDisk`
   - `SetupRadialPulse`
   - `SetupMandelbox`
   - `SetupTriangleFold`
   - `SetupMoireInterference`

**Verify**: File compiles standalone

---

#### Task 2.2: Create shader_setup_warp.cpp

**Files**: `src/render/shader_setup_warp.cpp`

**Build**:
1. Include `"shader_setup_warp.h"`, `"post_effect.h"`, `<math.h>`
2. Move these functions:
   - `SetupSineWarp`
   - `SetupTextureWarp`
   - `SetupWaveRipple`
   - `SetupMobius`
   - `SetupGradientFlow`
   - `SetupChladniWarp`
   - `SetupDomainWarp`
   - `SetupSurfaceWarp`

**Verify**: File compiles standalone

---

#### Task 2.3: Create shader_setup_cellular.cpp

**Files**: `src/render/shader_setup_cellular.cpp`

**Build**:
1. Include `"shader_setup_cellular.h"`, `"post_effect.h"`, `<math.h>`
2. Move these functions:
   - `SetupVoronoi`
   - `SetupLatticeFold`
   - `SetupPhyllotaxis`
   - `SetupDiscoBall`

**Verify**: File compiles standalone

---

#### Task 2.4: Create shader_setup_motion.cpp

**Files**: `src/render/shader_setup_motion.cpp`

**Build**:
1. Include `"shader_setup_motion.h"`, `"post_effect.h"`, `<math.h>`
2. Move these functions:
   - `SetupInfiniteZoom`
   - `SetupRadialStreak`
   - `SetupDrosteZoom`
   - `SetupDensityWaveSpiral`

**Verify**: File compiles standalone

---

#### Task 2.5: Create shader_setup_style.cpp

**Files**: `src/render/shader_setup_style.cpp`

**Build**:
1. Include `"shader_setup_style.h"`, `"post_effect.h"`, `"color_lut.h"`, `<math.h>`
2. Move these functions:
   - `SetupPixelation`
   - `SetupGlitch`
   - `SetupToon`
   - `SetupHeightfieldRelief`
   - `SetupAsciiArt`
   - `SetupOilPaint`
   - `SetupWatercolor`
   - `SetupNeonGlow`
   - `SetupCrossHatching`
   - `SetupBokeh`
   - `SetupBloom`
   - `SetupPencilSketch`
   - `SetupMatrixRain`
   - `SetupImpressionist`
   - `SetupKuwahara`
   - `SetupInkWash`

**Verify**: File compiles standalone

---

#### Task 2.6: Create shader_setup_color.cpp

**Files**: `src/render/shader_setup_color.cpp`

**Build**:
1. Include `"shader_setup_color.h"`, `"post_effect.h"`, `<math.h>`
2. Move these functions:
   - `SetupColorGrade`
   - `SetupFalseColor`
   - `SetupHalftone`
   - `SetupPaletteQuantization`

**Verify**: File compiles standalone

---

### Wave 3: Update Core Files

These tasks modify existing files and depend on Wave 1-2 completion.

#### Task 3.1: Update shader_setup.h

**Files**: `src/render/shader_setup.h`

**Build**:
1. Add includes for all 6 category headers after existing includes:
   ```cpp
   #include "shader_setup_symmetry.h"
   #include "shader_setup_warp.h"
   #include "shader_setup_cellular.h"
   #include "shader_setup_motion.h"
   #include "shader_setup_style.h"
   #include "shader_setup_color.h"
   ```
2. Remove declarations for functions now in category headers (keep only core functions):
   - Keep: `SetupFeedback`, `SetupBlurH`, `SetupBlurV`, `SetupChromatic`, `SetupGamma`, `SetupClarity`
   - Keep: All `Setup*TrailBoost` functions
   - Keep: `GetTransformEffect`, `ApplyBloomPasses`, `ApplyHalfResEffect`, `ApplyHalfResOilPaint`
   - Remove: All other Setup* declarations (now in category headers)

**Verify**: Header compiles, declares correct subset

---

#### Task 3.2: Update shader_setup.cpp

**Files**: `src/render/shader_setup.cpp`

**Build**:
1. Add includes for all 6 category headers after `#include "shader_setup.h"`
2. Delete the function implementations that moved to category files:
   - Delete symmetry functions (SetupKaleido, SetupKifs, etc.)
   - Delete warp functions (SetupSineWarp, SetupTextureWarp, etc.)
   - Delete cellular functions (SetupVoronoi, SetupLatticeFold, etc.)
   - Delete motion functions (SetupInfiniteZoom, SetupRadialStreak, etc.)
   - Delete style functions (SetupPixelation, SetupGlitch, etc.)
   - Delete color functions (SetupColorGrade, SetupFalseColor, etc.)
3. Keep only:
   - `GetTransformEffect` (lines 16-122)
   - `SetupFeedback` (lines 158-254)
   - `SetupBlurH`, `SetupBlurV` (lines 256-273)
   - All `Setup*TrailBoost` functions (lines 275-329)
   - `SetupChromatic`, `SetupGamma`, `SetupClarity` (lines 622-638)
   - `BloomRenderPass` (static helper, lines 1249-1259)
   - `ApplyBloomPasses`, `ApplyHalfResEffect`, `ApplyHalfResOilPaint` (lines 1261-1403)

**Verify**: File compiles, ~350 lines remaining

---

#### Task 3.3: Update CMakeLists.txt

**Files**: `CMakeLists.txt`

**Build**:
1. Add new source files to `RENDER_SOURCES` after `src/render/shader_setup.cpp`:
   ```cmake
   src/render/shader_setup_symmetry.cpp
   src/render/shader_setup_warp.cpp
   src/render/shader_setup_cellular.cpp
   src/render/shader_setup_motion.cpp
   src/render/shader_setup_style.cpp
   src/render/shader_setup_color.cpp
   ```

**Verify**: CMake configures successfully

---

## Final Verification

After all waves complete:

- [ ] `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release` succeeds
- [ ] `cmake.exe --build build` compiles with no errors
- [ ] `./build/AudioJones.exe` launches and all effects still work
- [ ] shader_setup.cpp is ~350 lines (down from 1403)
- [ ] Each category module contains only functions for that category
- [ ] No duplicate function definitions across files
