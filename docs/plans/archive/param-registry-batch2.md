# Param Registry Batch 2: Cellular + Motion + Optical

Migrate 13 effects (3 Cellular, 5 Motion, 5 Optical) from PARAM_TABLE to per-module `RegisterParams` functions. Total: 61 params. radial_streak has 0 modulatable params — gets an empty RegisterParams. Also fixes batch 1 effects that inlined `3.14159265f` literals instead of using rotation constants.

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
| `*Speed`, `*Twist` (rates) | `ROTATION_SPEED_MAX` | radians/second |
| `*Angle`, `*Rotation` (static offsets) | `ROTATION_OFFSET_MAX` | radians |

Both constants equal `3.14159265f` but carry different semantic meaning. Always use the named constant matching the param's suffix.

### Param Table (Cellular effects — 32 params)

| Effect | Config field | Param count | Param IDs (prefix) |
|--------|-------------|-------------|---------------------|
| voronoi | `voronoi` | 13 | `voronoi.{scale, speed, edgeFalloff, isoFrequency, uvDistortIntensity, edgeIsoIntensity, centerIsoIntensity, flatFillIntensity, organicFlowIntensity, edgeGlowIntensity, determinantIntensity, ratioIntensity, edgeDetectIntensity}` |
| lattice_fold | `latticeFold` | 3 | `latticeFold.{rotationSpeed, cellScale, smoothing}` |
| phyllotaxis | `phyllotaxis` | 16 | `phyllotaxis.{scale, divergenceAngle, angleSpeed, phaseSpeed, cellRadius, isoFrequency, uvDistortIntensity, organicFlowIntensity, edgeIsoIntensity, centerIsoIntensity, flatFillIntensity, edgeGlowIntensity, ratioIntensity, determinantIntensity, edgeDetectIntensity, spinSpeed}` |

### Param Table (Motion effects — 19 params)

| Effect | Config field | Param count | Param IDs (prefix) |
|--------|-------------|-------------|---------------------|
| infinite_zoom | `infiniteZoom` | 2 | `infiniteZoom.{spiralAngle, spiralTwist}` |
| droste_zoom | `drosteZoom` | 4 | `drosteZoom.{scale, spiralAngle, shearCoeff, innerRadius}` |
| density_wave_spiral | `densityWaveSpiral` | 4 | `densityWaveSpiral.{tightness, rotationSpeed, globalRotationSpeed, thickness}` |
| shake | `shake` | 3 | `shake.{intensity, rate, samples}` |
| relativistic_doppler | `relativisticDoppler` | 6 | `relativisticDoppler.{velocity, centerX, centerY, aberration, colorShift, headlight}` |

### Param Table (Optical effects — 10 params)

| Effect | Config field | Param count | Param IDs (prefix) |
|--------|-------------|-------------|---------------------|
| bloom | `bloom` | 2 | `bloom.{threshold, intensity}` |
| anamorphic_streak | `anamorphicStreak` | 4 | `anamorphicStreak.{threshold, intensity, stretch, sharpness}` |
| bokeh | `bokeh` | 2 | `bokeh.{radius, brightnessPower}` |
| heightfield_relief | `heightfieldRelief` | 2 | `heightfieldRelief.{lightAngle, intensity}` |
| radial_streak | — | 0 | *(empty RegisterParams)* |

### Bounds Reference

Exact bounds for each param (agents copy these verbatim):

**voronoi**: scale(5.0,50.0), speed(0.1,2.0), edgeFalloff(0.1,1.0), isoFrequency(1.0,50.0), uvDistortIntensity(0.0,1.0), edgeIsoIntensity(0.0,1.0), centerIsoIntensity(0.0,1.0), flatFillIntensity(0.0,1.0), organicFlowIntensity(0.0,1.0), edgeGlowIntensity(0.0,1.0), determinantIntensity(0.0,1.0), ratioIntensity(0.0,1.0), edgeDetectIntensity(0.0,1.0)

**latticeFold**: rotationSpeed(-ROTATION_SPEED_MAX,ROTATION_SPEED_MAX), cellScale(1.0,20.0), smoothing(0.0,0.5)

**phyllotaxis**: scale(0.02,0.15), divergenceAngle(-0.4,0.4), angleSpeed(-0.035,0.035), phaseSpeed(-ROTATION_SPEED_MAX,ROTATION_SPEED_MAX), cellRadius(0.1,1.5), isoFrequency(1.0,20.0), uvDistortIntensity(0.0,1.0), organicFlowIntensity(0.0,1.0), edgeIsoIntensity(0.0,1.0), centerIsoIntensity(0.0,1.0), flatFillIntensity(0.0,1.0), edgeGlowIntensity(0.0,1.0), ratioIntensity(0.0,1.0), determinantIntensity(0.0,1.0), edgeDetectIntensity(0.0,1.0), spinSpeed(-ROTATION_SPEED_MAX,ROTATION_SPEED_MAX)

**infiniteZoom**: spiralAngle(-ROTATION_OFFSET_MAX,ROTATION_OFFSET_MAX), spiralTwist(-ROTATION_OFFSET_MAX,ROTATION_OFFSET_MAX)

**drosteZoom**: scale(1.5,10.0), spiralAngle(-ROTATION_OFFSET_MAX,ROTATION_OFFSET_MAX), shearCoeff(-1.0,1.0), innerRadius(0.0,0.5)

**densityWaveSpiral**: tightness(-3.14159f,3.14159f), rotationSpeed(-ROTATION_SPEED_MAX,ROTATION_SPEED_MAX), globalRotationSpeed(-ROTATION_SPEED_MAX,ROTATION_SPEED_MAX), thickness(0.05,0.5)

**shake**: intensity(0.0,0.2), rate(1.0,60.0), samples(1.0,16.0)

**relativisticDoppler**: velocity(0.0,0.99), centerX(0.0,1.0), centerY(0.0,1.0), aberration(0.0,1.0), colorShift(0.0,1.0), headlight(0.0,1.0)

**bloom**: threshold(0.0,2.0), intensity(0.0,2.0)

**anamorphicStreak**: threshold(0.0,2.0), intensity(0.0,2.0), stretch(0.5,8.0), sharpness(0.0,1.0)

**bokeh**: radius(0.0,0.1), brightnessPower(1.0,8.0)

**heightfieldRelief**: lightAngle(0.0,6.28), intensity(0.0,1.0)

---

## Tasks

### Wave 1: Fix batch 1 inlined rotation literals

#### Task 1.0: Replace inlined `3.14159265f` with rotation constants in batch 1 effects

**Files** (modify — 20 `.cpp` files):
- `src/effects/sine_warp.cpp`
- `src/effects/texture_warp.cpp`
- `src/effects/wave_ripple.cpp` *(only if it has inlined literals — verify)*
- `src/effects/mobius.cpp`
- `src/effects/gradient_flow.cpp` *(only if it has inlined literals — verify)*
- `src/effects/chladni_warp.cpp` *(only if it has inlined literals — verify)*
- `src/effects/domain_warp.cpp`
- `src/effects/surface_warp.cpp`
- `src/effects/interference_warp.cpp`
- `src/effects/corridor_warp.cpp`
- `src/effects/fft_radial_warp.cpp`
- `src/effects/circuit_board.cpp`
- `src/effects/kaleidoscope.cpp`
- `src/effects/kifs.cpp`
- `src/effects/poincare_disk.cpp`
- `src/effects/mandelbox.cpp`
- `src/effects/triangle_fold.cpp`
- `src/effects/moire_interference.cpp`
- `src/effects/radial_ifs.cpp`
- `src/effects/radial_pulse.cpp`

**Do**:
1. Add `#include "ui/ui_units.h"` to each file that gains a constant reference
2. Replace every `-3.14159265f, 3.14159265f` with the correct constant pair based on param suffix:

**Use `-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX`** for:
- `kaleidoscope.rotationSpeed`
- `kifs.rotationSpeed`, `kifs.twistSpeed`
- `poincareDisk.translationSpeed`, `poincareDisk.rotationSpeed`
- `mandelbox.rotationSpeed`, `mandelbox.twistSpeed`
- `triangleFold.rotationSpeed`, `triangleFold.twistSpeed`
- `radialIfs.rotationSpeed`, `radialIfs.twistSpeed`
- `moireInterference.animationSpeed`
- `surfaceWarp.rotationSpeed`
- `interferenceWarp.axisRotationSpeed`
- `corridorWarp.viewRotationSpeed`, `corridorWarp.planeRotationSpeed`
- `fftRadialWarp.phaseSpeed`
- `domainWarp.driftSpeed`
- `mobius.speed`

**Use `-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX`** for:
- `kaleidoscope.twistAngle`
- `moireInterference.rotationAngle`
- `surfaceWarp.angle`
- `textureWarp.ridgeAngle`
- `domainWarp.driftAngle`
- `circuitBoard.scrollAngle`
- `sineWarp.octaveRotation`
- `radialPulse.spiralTwist`, `radialPulse.octaveRotation`

**Verify**: `cmake.exe --build build` compiles. `grep -r "3\.14159265f" src/effects/` returns zero matches.

---

### Wave 1 (parallel): Add RegisterParams to Cellular effects (3 effects, 32 params)

#### Task 1.1: Cellular effects — RegisterParams

**Files** (modify):
- `src/effects/voronoi.h`, `src/effects/voronoi.cpp`
- `src/effects/lattice_fold.h`, `src/effects/lattice_fold.cpp`
- `src/effects/phyllotaxis.h`, `src/effects/phyllotaxis.cpp`

**Do**:
1. In each `.h`: Add `void <Name>RegisterParams(<Name>Config* cfg);` declaration before `#endif`
2. In each `.cpp`: Add `#include "automation/modulation_engine.h"` and implement `RegisterParams`
3. Add `#include "ui/ui_units.h"` to `lattice_fold.cpp` and `phyllotaxis.cpp` (they use rotation constants)
4. Each `ModEngineRegisterParam` call uses the exact param ID string, pointer (`&cfg->field`), and bounds from the Bounds Reference above
5. Use `ROTATION_SPEED_MAX` for `latticeFold.rotationSpeed`, `phyllotaxis.phaseSpeed`, `phyllotaxis.spinSpeed`

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 1 (parallel): Add RegisterParams to Motion effects (5 effects, 19 params)

#### Task 1.2: Motion effects — RegisterParams

**Files** (modify):
- `src/effects/infinite_zoom.h`, `src/effects/infinite_zoom.cpp`
- `src/effects/droste_zoom.h`, `src/effects/droste_zoom.cpp`
- `src/effects/density_wave_spiral.h`, `src/effects/density_wave_spiral.cpp`
- `src/effects/shake.h`, `src/effects/shake.cpp`
- `src/effects/relativistic_doppler.h`, `src/effects/relativistic_doppler.cpp`

**Do**:
1. In each `.h`: Add `void <Name>RegisterParams(<Name>Config* cfg);` declaration before `#endif`
2. In each `.cpp`: Add `#include "automation/modulation_engine.h"` and implement `RegisterParams`
3. Add `#include "ui/ui_units.h"` to `infinite_zoom.cpp`, `droste_zoom.cpp`, and `density_wave_spiral.cpp`
4. Each `ModEngineRegisterParam` call uses the exact param ID string, pointer (`&cfg->field`), and bounds from the Bounds Reference above
5. Use `ROTATION_OFFSET_MAX` for `infiniteZoom.spiralAngle`, `infiniteZoom.spiralTwist`, `drosteZoom.spiralAngle`
6. Use `ROTATION_SPEED_MAX` for `densityWaveSpiral.rotationSpeed`, `densityWaveSpiral.globalRotationSpeed`

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 1 (parallel): Add RegisterParams to Optical effects (5 effects, 10 params)

#### Task 1.3: Optical effects — RegisterParams

**Files** (modify):
- `src/effects/bloom.h`, `src/effects/bloom.cpp`
- `src/effects/anamorphic_streak.h`, `src/effects/anamorphic_streak.cpp`
- `src/effects/bokeh.h`, `src/effects/bokeh.cpp`
- `src/effects/heightfield_relief.h`, `src/effects/heightfield_relief.cpp`
- `src/effects/radial_streak.h`, `src/effects/radial_streak.cpp`

**Do**:
1. In each `.h`: Add `void <Name>RegisterParams(<Name>Config* cfg);` declaration before `#endif`
2. In each `.cpp`: Add `#include "automation/modulation_engine.h"` and implement `RegisterParams`
3. Each `ModEngineRegisterParam` call uses the exact param ID string, pointer (`&cfg->field`), and bounds from the Bounds Reference above
4. For `radial_streak`: empty function body (0 modulatable params), still include `modulation_engine.h`
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
1. In `PostEffectRegisterParams` (`post_effect.cpp`), add calls after the existing symmetry block:
   ```
   // Cellular effects
   VoronoiRegisterParams(&pe->effects.voronoi);
   LatticeFoldRegisterParams(&pe->effects.latticeFold);
   PhyllotaxisRegisterParams(&pe->effects.phyllotaxis);

   // Motion effects
   InfiniteZoomRegisterParams(&pe->effects.infiniteZoom);
   DrosteZoomRegisterParams(&pe->effects.drosteZoom);
   DensityWaveSpiralRegisterParams(&pe->effects.densityWaveSpiral);
   ShakeRegisterParams(&pe->effects.shake);
   RelativisticDopplerRegisterParams(&pe->effects.relativisticDoppler);

   // Optical effects
   BloomRegisterParams(&pe->effects.bloom);
   AnamorphicStreakRegisterParams(&pe->effects.anamorphicStreak);
   BokehRegisterParams(&pe->effects.bokeh);
   HeightfieldReliefRegisterParams(&pe->effects.heightfieldRelief);
   RadialStreakRegisterParams(&pe->effects.radialStreak);
   ```
2. In `param_registry.cpp`, remove all PARAM_TABLE entries with prefixes: `voronoi`, `latticeFold`, `phyllotaxis`, `infiniteZoom`, `drosteZoom`, `densityWaveSpiral`, `shake`, `relativisticDoppler`, `bloom`, `anamorphicStreak`, `bokeh`, `heightfieldRelief`
3. Keep all other entries (physarum, boids, curlFlow, curlAdvection, cymatics, attractorFlow, particleLife, flowField, feedbackFlow, proceduralWarp, effects.*, interference, pixelation, plasma, glitch, colorGrade, asciiArt, oilPaint, watercolor, neonGlow, halftone, falseColor, crossHatching, paletteQuantization, pencilSketch, matrixRain, impressionist, kuwahara, inkWash, discoBall, synthwave, legoBricks)

**Verify**: `cmake.exe --build build` compiles. PARAM_TABLE entry count drops by 61.

---

## Final Verification

- [ ] Build succeeds with no warnings related to these changes
- [ ] PARAM_TABLE `PARAM_COUNT` reduced by 61
- [ ] `grep -r "3\.14159265f" src/effects/` returns zero matches
- [ ] All 13 batch 2 effects respond to modulation via RegisterParams
- [ ] All 20 batch 1 effects still respond to modulation after constant swap
- [ ] Existing presets load without regression

## Notes

- `ModEngineRegisterParam` handles re-registration gracefully. Order of registration doesn't matter.
- Param IDs must match exactly between old PARAM_TABLE entries and new RegisterParams calls.
- `densityWaveSpiral.tightness` uses `-3.14159f, 3.14159f` (6-digit pi, not a rotation constant). This is intentional — tightness is a spiral winding parameter, not a rotation speed or angle.
- `heightfieldRelief.lightAngle` uses `0.0, 6.28` (full circle). This is a light direction, not bounded by ROTATION_OFFSET_MAX.
