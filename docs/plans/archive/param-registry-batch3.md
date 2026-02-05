# Param Registry Batch 3: Artistic + Graphic + Retro

Migrate 16 effects (6 Artistic, 5 Graphic, 4 Retro, plus toon with 0 params) from PARAM_TABLE to per-module `RegisterParams` functions. Total: 66 params.

**Research**: [docs/research/param-registry-refactor.md](../research/param-registry-refactor.md)
**Parent plan**: [docs/plans/param-registry-migration.md](param-registry-migration.md)

## Design

### Pattern

Each effect gets a `RegisterParams` function that calls `ModEngineRegisterParam` for each modulatable field.

**Declaration** (in `<effect>.h`):
```
void <Name>RegisterParams(<Name>Config* cfg);
```

**Implementation** (in `<effect>.cpp`):
- Include `automation/modulation_engine.h`
- Include `ui/ui_units.h` if the effect uses `ROTATION_SPEED_MAX` or `ROTATION_OFFSET_MAX`
- One `ModEngineRegisterParam(id, &cfg->field, min, max)` call per modulatable param
- Param IDs and bounds match PARAM_TABLE entries exactly
- **Use constants for rotation bounds** — never inline `3.14159265f`

**Call site** (`post_effect.cpp` in `PostEffectRegisterParams`):
- All effect headers already included via `post_effect.h`
- Call `<Name>RegisterParams(&pe->effects.<configField>)`

### Rotation Constant Rules

Per `docs/conventions.md` angular field conventions:

| Suffix | Constant | Semantic |
|--------|----------|----------|
| `*Speed` (rates) | `ROTATION_SPEED_MAX` | radians/second |
| `*Angle` (static offsets) | `ROTATION_OFFSET_MAX` | radians |

### Param Table (Artistic effects — 23 params)

| Effect | Config field | Param count | Param IDs (prefix) |
|--------|-------------|-------------|---------------------|
| oil_paint | `oilPaint` | 3 | `oilPaint.{brushSize, strokeBend, specular}` |
| watercolor | `watercolor` | 3 | `watercolor.{strokeStep, washStrength, paperStrength}` |
| impressionist | `impressionist` | 4 | `impressionist.{splatSizeMax, strokeFreq, edgeStrength, strokeOpacity}` |
| ink_wash | `inkWash` | 5 | `inkWash.{strength, granulation, bleedStrength, bleedRadius, softness}` |
| pencil_sketch | `pencilSketch` | 4 | `pencilSketch.{strokeFalloff, paperStrength, vignetteStrength, wobbleAmount}` |
| cross_hatching | `crossHatching` | 4 | `crossHatching.{width, threshold, noise, outline}` |

### Param Table (Graphic effects — 20 params)

| Effect | Config field | Param count | Param IDs (prefix) |
|--------|-------------|-------------|---------------------|
| neon_glow | `neonGlow` | 5 | `neonGlow.{glowIntensity, edgeThreshold, originalVisibility, saturationBoost, brightnessBoost}` |
| kuwahara | `kuwahara` | 1 | `kuwahara.{radius}` |
| halftone | `halftone` | 3 | `halftone.{dotScale, rotationSpeed, rotationAngle}` |
| disco_ball | `discoBall` | 8 | `discoBall.{sphereRadius, tileSize, rotationSpeed, bumpHeight, reflectIntensity, spotIntensity, spotFalloff, brightnessThreshold}` |
| lego_bricks | `legoBricks` | 3 | `legoBricks.{brickScale, studHeight, lightAngle}` |

### Param Table (Retro effects — 23 params)

| Effect | Config field | Param count | Param IDs (prefix) |
|--------|-------------|-------------|---------------------|
| pixelation | `pixelation` | 2 | `pixelation.{cellCount, ditherScale}` |
| glitch | `glitch` | 16 | `glitch.{analogIntensity, blockThreshold, aberration, blockOffset, datamoshIntensity, datamoshMin, datamoshMax, rowSliceIntensity, colSliceIntensity, diagonalBandDisplace, blockMaskIntensity, temporalJitterAmount, temporalJitterGate, blockMultiplySize, blockMultiplyControl, blockMultiplyIntensity}` |
| ascii_art | `asciiArt` | 1 | `asciiArt.{cellSize}` |
| matrix_rain | `matrixRain` | 4 | `matrixRain.{rainSpeed, overlayIntensity, trailLength, leadBrightness}` |
| toon | — | 0 | *(empty RegisterParams)* |

### Bounds Reference

Exact bounds for each param (agents copy these verbatim):

**oilPaint**: brushSize(0.5,3.0), strokeBend(-2.0,2.0), specular(0.0,1.0)

**watercolor**: strokeStep(0.4,2.0), washStrength(0.0,1.0), paperStrength(0.0,1.0)

**impressionist**: splatSizeMax(0.05,0.25), strokeFreq(400.0,2000.0), edgeStrength(0.0,8.0), strokeOpacity(0.0,1.0)

**inkWash**: strength(0.0,2.0), granulation(0.0,1.0), bleedStrength(0.0,1.0), bleedRadius(1.0,10.0), softness(0.0,5.0)

**pencilSketch**: strokeFalloff(0.0,1.0), paperStrength(0.0,1.0), vignetteStrength(0.0,1.0), wobbleAmount(0.0,8.0)

**crossHatching**: width(0.5,4.0), threshold(0.0,2.0), noise(0.0,1.0), outline(0.0,1.0)

**neonGlow**: glowIntensity(0.5,5.0), edgeThreshold(0.0,0.5), originalVisibility(0.0,1.0), saturationBoost(0.0,1.0), brightnessBoost(0.0,1.0)

**kuwahara**: radius(2.0,12.0)

**halftone**: dotScale(2.0,20.0), rotationSpeed(-ROTATION_SPEED_MAX,ROTATION_SPEED_MAX), rotationAngle(-ROTATION_OFFSET_MAX,ROTATION_OFFSET_MAX)

**discoBall**: sphereRadius(0.2,1.5), tileSize(0.05,0.3), rotationSpeed(-ROTATION_SPEED_MAX,ROTATION_SPEED_MAX), bumpHeight(0.0,0.2), reflectIntensity(0.5,5.0), spotIntensity(0.0,3.0), spotFalloff(0.5,3.0), brightnessThreshold(0.0,0.5)

**legoBricks**: brickScale(0.01,0.2), studHeight(0.0,1.0), lightAngle(-ROTATION_OFFSET_MAX,ROTATION_OFFSET_MAX)

**pixelation**: cellCount(4.0,256.0), ditherScale(1.0,8.0)

**glitch**: analogIntensity(0.0,1.0), blockThreshold(0.0,0.9), aberration(0.0,20.0), blockOffset(0.0,0.5), datamoshIntensity(0.0,1.0), datamoshMin(4.0,32.0), datamoshMax(16.0,128.0), rowSliceIntensity(0.0,0.5), colSliceIntensity(0.0,0.5), diagonalBandDisplace(0.0,0.1), blockMaskIntensity(0.0,1.0), temporalJitterAmount(0.0,0.1), temporalJitterGate(0.0,1.0), blockMultiplySize(4.0,64.0), blockMultiplyControl(0.0,1.0), blockMultiplyIntensity(0.0,1.0)

**asciiArt**: cellSize(4.0,32.0)

**matrixRain**: rainSpeed(0.1,5.0), overlayIntensity(0.0,1.0), trailLength(5.0,40.0), leadBrightness(0.5,3.0)

---

## Tasks

### Wave 1 (parallel): Add RegisterParams to Artistic effects (6 effects, 23 params)

#### Task 1.1: Artistic effects — RegisterParams

**Files** (modify):
- `src/effects/oil_paint.h`, `src/effects/oil_paint.cpp`
- `src/effects/watercolor.h`, `src/effects/watercolor.cpp`
- `src/effects/impressionist.h`, `src/effects/impressionist.cpp`
- `src/effects/ink_wash.h`, `src/effects/ink_wash.cpp`
- `src/effects/pencil_sketch.h`, `src/effects/pencil_sketch.cpp`
- `src/effects/cross_hatching.h`, `src/effects/cross_hatching.cpp`

**Do**:
1. In each `.h`: Add `void <Name>RegisterParams(<Name>Config* cfg);` declaration before `#endif`
2. In each `.cpp`: Add `#include "automation/modulation_engine.h"` and implement `RegisterParams`
3. No rotation constants needed in this group — no `*Speed` or `*Angle` suffixed params
4. Each `ModEngineRegisterParam` call uses the exact param ID string, pointer (`&cfg->field`), and bounds from the Bounds Reference above

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 1 (parallel): Add RegisterParams to Graphic effects (5 effects, 20 params)

#### Task 1.2: Graphic effects — RegisterParams

**Files** (modify):
- `src/effects/neon_glow.h`, `src/effects/neon_glow.cpp`
- `src/effects/kuwahara.h`, `src/effects/kuwahara.cpp`
- `src/effects/halftone.h`, `src/effects/halftone.cpp`
- `src/effects/disco_ball.h`, `src/effects/disco_ball.cpp`
- `src/effects/lego_bricks.h`, `src/effects/lego_bricks.cpp`

**Do**:
1. In each `.h`: Add `void <Name>RegisterParams(<Name>Config* cfg);` declaration before `#endif`
2. In each `.cpp`: Add `#include "automation/modulation_engine.h"` and implement `RegisterParams`
3. Add `#include "ui/ui_units.h"` to `halftone.cpp`, `disco_ball.cpp`, and `lego_bricks.cpp` (they use rotation constants)
4. Each `ModEngineRegisterParam` call uses the exact param ID string, pointer (`&cfg->field`), and bounds from the Bounds Reference above
5. Use `ROTATION_SPEED_MAX` for `halftone.rotationSpeed` and `discoBall.rotationSpeed`
6. Use `ROTATION_OFFSET_MAX` for `halftone.rotationAngle` and `legoBricks.lightAngle`

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 1 (parallel): Add RegisterParams to Retro effects (5 effects, 23 params)

#### Task 1.3: Retro effects — RegisterParams

**Files** (modify):
- `src/effects/pixelation.h`, `src/effects/pixelation.cpp`
- `src/effects/glitch.h`, `src/effects/glitch.cpp`
- `src/effects/ascii_art.h`, `src/effects/ascii_art.cpp`
- `src/effects/matrix_rain.h`, `src/effects/matrix_rain.cpp`
- `src/effects/toon.h`, `src/effects/toon.cpp`

**Do**:
1. In each `.h`: Add `void <Name>RegisterParams(<Name>Config* cfg);` declaration before `#endif`
2. In each `.cpp`: Add `#include "automation/modulation_engine.h"` and implement `RegisterParams`
3. For `toon`: empty function body (0 modulatable params), still include `modulation_engine.h`
4. Each `ModEngineRegisterParam` call uses the exact param ID string, pointer (`&cfg->field`), and bounds from the Bounds Reference above
5. No rotation constants needed in this group

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 2: Wire up call site + remove PARAM_TABLE entries

#### Task 2.1: Add RegisterParams calls and remove PARAM_TABLE entries

**Files** (modify):
- `src/render/post_effect.cpp`
- `src/automation/param_registry.cpp`

**Depends on**: Wave 1 complete

**Do**:
1. In `PostEffectRegisterParams` (`post_effect.cpp`), add calls after the existing Optical effects block:
   ```
   // Artistic effects
   OilPaintRegisterParams(&pe->effects.oilPaint);
   WatercolorRegisterParams(&pe->effects.watercolor);
   ImpressionistRegisterParams(&pe->effects.impressionist);
   InkWashRegisterParams(&pe->effects.inkWash);
   PencilSketchRegisterParams(&pe->effects.pencilSketch);
   CrossHatchingRegisterParams(&pe->effects.crossHatching);

   // Graphic effects
   NeonGlowRegisterParams(&pe->effects.neonGlow);
   KuwaharaRegisterParams(&pe->effects.kuwahara);
   HalftoneRegisterParams(&pe->effects.halftone);
   DiscoBallRegisterParams(&pe->effects.discoBall);
   LegoBricksRegisterParams(&pe->effects.legoBricks);

   // Retro effects
   PixelationRegisterParams(&pe->effects.pixelation);
   GlitchRegisterParams(&pe->effects.glitch);
   AsciiArtRegisterParams(&pe->effects.asciiArt);
   MatrixRainRegisterParams(&pe->effects.matrixRain);
   ToonRegisterParams(&pe->effects.toon);
   ```
2. In `param_registry.cpp`, remove all PARAM_TABLE entries with these prefixes: `oilPaint`, `watercolor`, `impressionist`, `inkWash`, `pencilSketch`, `crossHatching`, `neonGlow`, `kuwahara`, `halftone`, `discoBall`, `legoBricks`, `pixelation`, `glitch`, `asciiArt`, `matrixRain`
3. Keep all other entries (physarum, boids, curlFlow, curlAdvection, cymatics, attractorFlow, particleLife, flowField, feedbackFlow, proceduralWarp, effects.*, interference, plasma, constellation, synthwave, falseColor, paletteQuantization)

**Verify**: `cmake.exe --build build` compiles. PARAM_TABLE entry count drops by 66.

---

## Final Verification

- [ ] Build succeeds with no warnings related to these changes
- [ ] PARAM_TABLE `PARAM_COUNT` reduced by 66
- [ ] All 16 batch 3 effects respond to modulation via RegisterParams
- [ ] Existing presets load without regression
