# Param Registry Batch 1: Warp + Symmetry

Migrate 20 effects (12 Warp, 8 Symmetry) from PARAM_TABLE to per-module `RegisterParams` functions. Total: ~93 params.

**Research**: [docs/research/param-registry-refactor.md](../research/param-registry-refactor.md)
**Parent plan**: [docs/plans/param-registry-migration.md](param-registry-migration.md)

## Design

### Pattern

Each effect gets a `RegisterParams` function that calls `ModEngineRegisterParam` for each modulatable field. Follows the signature and call-site pattern established in batch 0.

**Declaration** (in `<effect>.h`):
```
void <Name>RegisterParams(<Name>Config* cfg);
```

**Implementation** (in `<effect>.cpp`):
- Include `modulation_engine.h`
- One `ModEngineRegisterParam(id, &cfg->field, min, max)` call per modulatable param
- Param IDs and bounds match PARAM_TABLE entries exactly

**Call site** (`post_effect.cpp` in `PostEffectRegisterParams`):
- Include each effect header
- Call `<Name>RegisterParams(&pe->effects.<configField>)`

### Param Table (Warp effects — 63 params)

| Effect | Config field | Param count | Param IDs (prefix) |
|--------|-------------|-------------|---------------------|
| sine_warp | `sineWarp` | 2 | `sineWarp.strength`, `sineWarp.octaveRotation` |
| texture_warp | `textureWarp` | 4 | `textureWarp.{strength, ridgeAngle, anisotropy, noiseAmount}` |
| wave_ripple | `waveRipple` | 8 | `waveRipple.{strength, frequency, steepness, decay, centerHole, originX, originY, shadeIntensity}` |
| mobius | `mobius` | 7 | `mobius.{spiralTightness, zoomFactor, speed, point1X, point1Y, point2X, point2Y}` |
| gradient_flow | `gradientFlow` | 2 | `gradientFlow.{strength, edgeWeight}` |
| chladni_warp | `chladniWarp` | 4 | `chladniWarp.{n, m, strength, animRange}` |
| domain_warp | `domainWarp` | 4 | `domainWarp.{warpStrength, falloff, driftSpeed, driftAngle}` |
| surface_warp | `surfaceWarp` | 5 | `surfaceWarp.{intensity, angle, rotationSpeed, scrollSpeed, depthShade}` |
| interference_warp | `interferenceWarp` | 5 | `interferenceWarp.{amplitude, axisRotationSpeed, decay, scale, speed}` |
| corridor_warp | `corridorWarp` | 8 | `corridorWarp.{horizon, perspectiveStrength, viewRotationSpeed, planeRotationSpeed, scale, scrollSpeed, strafeSpeed, fogStrength}` |
| fft_radial_warp | `fftRadialWarp` | 9 | `fftRadialWarp.{intensity, freqStart, freqEnd, maxRadius, freqCurve, bassBoost, pushPullBalance, pushPullSmoothness, phaseSpeed}` |
| circuit_board | `circuitBoard` | 5 | `circuitBoard.{scale, offset, strength, scrollSpeed, scrollAngle}` |

### Param Table (Symmetry effects — 30 params)

| Effect | Config field | Param count | Param IDs (prefix) |
|--------|-------------|-------------|---------------------|
| kaleidoscope | `kaleidoscope` | 3 | `kaleidoscope.{rotationSpeed, twistAngle, smoothing}` |
| kifs | `kifs` | 3 | `kifs.{rotationSpeed, twistSpeed, polarFoldSmoothing}` |
| poincare_disk | `poincareDisk` | 6 | `poincareDisk.{translationX, translationY, translationSpeed, translationAmplitude, diskScale, rotationSpeed}` |
| mandelbox | `mandelbox` | 4 | `mandelbox.{rotationSpeed, twistSpeed, boxIntensity, sphereIntensity}` |
| triangle_fold | `triangleFold` | 2 | `triangleFold.{rotationSpeed, twistSpeed}` |
| moire_interference | `moireInterference` | 3 | `moireInterference.{rotationAngle, scaleDiff, animationSpeed}` |
| radial_ifs | `radialIfs` | 3 | `radialIfs.{rotationSpeed, twistSpeed, smoothing}` |
| radial_pulse | `radialPulse` | 6 | `radialPulse.{radialFreq, radialAmp, angularAmp, petalAmp, spiralTwist, octaveRotation}` |

---

## Tasks

### Wave 1: Add RegisterParams to Warp effects (12 effects, 63 params)

#### Task 1.1: Warp effects — RegisterParams

**Files** (modify):
- `src/effects/sine_warp.h`, `src/effects/sine_warp.cpp`
- `src/effects/texture_warp.h`, `src/effects/texture_warp.cpp`
- `src/effects/wave_ripple.h`, `src/effects/wave_ripple.cpp`
- `src/effects/mobius.h`, `src/effects/mobius.cpp`
- `src/effects/gradient_flow.h`, `src/effects/gradient_flow.cpp`
- `src/effects/chladni_warp.h`, `src/effects/chladni_warp.cpp`
- `src/effects/domain_warp.h`, `src/effects/domain_warp.cpp`
- `src/effects/surface_warp.h`, `src/effects/surface_warp.cpp`
- `src/effects/interference_warp.h`, `src/effects/interference_warp.cpp`
- `src/effects/corridor_warp.h`, `src/effects/corridor_warp.cpp`
- `src/effects/fft_radial_warp.h`, `src/effects/fft_radial_warp.cpp`
- `src/effects/circuit_board.h`, `src/effects/circuit_board.cpp`

**Do**:
1. In each `.h`: Add `void <Name>RegisterParams(<Name>Config* cfg);` declaration
2. In each `.cpp`: Add `#include "modulation_engine.h"` and implement `RegisterParams`
3. Each `ModEngineRegisterParam` call uses the exact param ID string, pointer (`&cfg->field`), and bounds from the Param Table above

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 1 (parallel): Add RegisterParams to Symmetry effects (8 effects, 30 params)

#### Task 1.2: Symmetry effects — RegisterParams

**Files** (modify):
- `src/effects/kaleidoscope.h`, `src/effects/kaleidoscope.cpp`
- `src/effects/kifs.h`, `src/effects/kifs.cpp`
- `src/effects/poincare_disk.h`, `src/effects/poincare_disk.cpp`
- `src/effects/mandelbox.h`, `src/effects/mandelbox.cpp`
- `src/effects/triangle_fold.h`, `src/effects/triangle_fold.cpp`
- `src/effects/moire_interference.h`, `src/effects/moire_interference.cpp`
- `src/effects/radial_ifs.h`, `src/effects/radial_ifs.cpp`
- `src/effects/radial_pulse.h`, `src/effects/radial_pulse.cpp`

**Do**:
1. In each `.h`: Add `void <Name>RegisterParams(<Name>Config* cfg);` declaration
2. In each `.cpp`: Add `#include "modulation_engine.h"` and implement `RegisterParams`
3. Each `ModEngineRegisterParam` call uses the exact param ID string, pointer (`&cfg->field`), and bounds from the Param Table above

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 2: Wire up call site + remove PARAM_TABLE entries

#### Task 2.1: Add RegisterParams calls in PostEffectRegisterParams

**Files** (modify): `src/render/post_effect.cpp`

**Depends on**: Wave 1 complete

**Do**:
1. Add includes for all 20 effect headers (12 warp + 8 symmetry) at top of file
2. In `PostEffectRegisterParams`, add one call per effect:
   - `SineWarpRegisterParams(&pe->effects.sineWarp);`
   - `TextureWarpRegisterParams(&pe->effects.textureWarp);`
   - `WaveRippleRegisterParams(&pe->effects.waveRipple);`
   - `MobiusRegisterParams(&pe->effects.mobius);`
   - `GradientFlowRegisterParams(&pe->effects.gradientFlow);`
   - `ChladniWarpRegisterParams(&pe->effects.chladniWarp);`
   - `DomainWarpRegisterParams(&pe->effects.domainWarp);`
   - `SurfaceWarpRegisterParams(&pe->effects.surfaceWarp);`
   - `InterferenceWarpRegisterParams(&pe->effects.interferenceWarp);`
   - `CorridorWarpRegisterParams(&pe->effects.corridorWarp);`
   - `FftRadialWarpRegisterParams(&pe->effects.fftRadialWarp);`
   - `CircuitBoardRegisterParams(&pe->effects.circuitBoard);`
   - `KaleidoscopeRegisterParams(&pe->effects.kaleidoscope);`
   - `KifsRegisterParams(&pe->effects.kifs);`
   - `PoincareDiskRegisterParams(&pe->effects.poincareDisk);`
   - `MandelboxRegisterParams(&pe->effects.mandelbox);`
   - `TriangleFoldRegisterParams(&pe->effects.triangleFold);`
   - `MoireInterferenceRegisterParams(&pe->effects.moireInterference);`
   - `RadialIfsRegisterParams(&pe->effects.radialIfs);`
   - `RadialPulseRegisterParams(&pe->effects.radialPulse);`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Remove migrated entries from PARAM_TABLE

**Files** (modify): `src/automation/param_registry.cpp`

**Depends on**: Task 2.1 complete (ensures RegisterParams calls are wired before removing table entries)

**Do**:
1. Remove all 93 entries belonging to the 20 migrated effects from PARAM_TABLE
2. Effects to remove (by prefix): `sineWarp`, `textureWarp`, `waveRipple`, `mobius`, `gradientFlow`, `chladniWarp`, `domainWarp`, `surfaceWarp`, `interferenceWarp`, `corridorWarp`, `fftRadialWarp`, `circuitBoard`, `kaleidoscope`, `kifs`, `poincareDisk`, `mandelbox`, `triangleFold`, `moireInterference`, `radialIfs`, `radialPulse`
3. Keep all other entries (physarum, boids, curlFlow, curlAdvection, cymatics, attractorFlow, particleLife, flowField, feedbackFlow, proceduralWarp, effects.*, voronoi, latticeFold, phyllotaxis, shake, interference, pixelation, plasma, glitch, bloom, anamorphicStreak, fftRadialWarp — wait, that moves. Keep: densityWaveSpiral, relativisticDoppler, etc. — actually, only remove entries whose prefix matches the 20 batch 1 effects listed above.)

**Verify**: `cmake.exe --build build` compiles. Total PARAM_TABLE entry count drops by 93.

---

## Final Verification

- [ ] Build succeeds with no warnings related to these changes
- [ ] PARAM_TABLE `PARAM_COUNT` reduced by 93 (from current count)
- [ ] All 20 effects still respond to modulation (params registered via RegisterParams instead of PARAM_TABLE)
- [ ] Existing presets load without regression (modulation routes resolve to same param IDs)

## Notes

- `ModEngineRegisterParam` handles re-registration gracefully (updates pointer, keeps base). Order of registration (PARAM_TABLE vs RegisterParams) doesn't matter — whichever runs last wins the pointer.
- Param IDs must match exactly between old PARAM_TABLE entries and new RegisterParams calls. Any typo breaks modulation routing for that param.
- The `#include "modulation_engine.h"` in each effect `.cpp` is the only new dependency introduced.
