# Param Registry Migration — Batch 5: Simulations

Move 61 simulation params from PARAM_TABLE into per-module RegisterParams functions in `src/simulation/`. After this batch, PARAM_TABLE retains only 24 top-level/feedback/flow entries.

**Parent plan**: [docs/plans/param-registry-migration.md](param-registry-migration.md)

## Modules

| Module | Prefix | Params | File |
|--------|--------|--------|------|
| Physarum | `physarum.` | 19 | `src/simulation/physarum.cpp` |
| AttractorFlow | `attractorFlow.` | 6 | `src/simulation/attractor_flow.cpp` |
| ParticleLife | `particleLife.` | 11 | `src/simulation/particle_life.cpp` |
| Boids | `boids.` | 3 | `src/simulation/boids.cpp` |
| CurlFlow | `curlFlow.` | 1 | `src/simulation/curl_flow.cpp` |
| CurlAdvection | `curlAdvection.` | 13 | `src/simulation/curl_advection.cpp` |
| Cymatics | `cymatics.` | 8 | `src/simulation/cymatics.cpp` |

## Design

### RegisterParams Signature

Each module gets:

```
void <Module>RegisterParams(<Module>Config *cfg);
```

Declared in the corresponding `.h`, implemented in the `.cpp`. Takes a pointer to the config struct embedded in `EffectConfig`. Calls `ModEngineRegisterParam` for each modulatable field with the same param IDs and bounds currently in PARAM_TABLE.

### Parameter Table

Parameters migrate 1:1 from PARAM_TABLE. IDs, min/max bounds, and field offsets remain identical.

**Physarum** (19 params):

| Param ID | Min | Max | Field |
|----------|-----|-----|-------|
| `physarum.sensorDistance` | 1.0 | 100.0 | `sensorDistance` |
| `physarum.sensorDistanceVariance` | 0.0 | 20.0 | `sensorDistanceVariance` |
| `physarum.sensorAngle` | 0.0 | 6.28 | `sensorAngle` |
| `physarum.turningAngle` | 0.0 | 6.28 | `turningAngle` |
| `physarum.stepSize` | 0.1 | 100.0 | `stepSize` |
| `physarum.levyAlpha` | 0.1 | 3.0 | `levyAlpha` |
| `physarum.densityResponse` | 0.1 | 5.0 | `densityResponse` |
| `physarum.cauchyScale` | 0.1 | 2.0 | `cauchyScale` |
| `physarum.expScale` | 0.1 | 3.0 | `expScale` |
| `physarum.gaussianVariance` | 0.0 | 1.0 | `gaussianVariance` |
| `physarum.sprintFactor` | 0.0 | 5.0 | `sprintFactor` |
| `physarum.gradientBoost` | 0.0 | 10.0 | `gradientBoost` |
| `physarum.repulsionStrength` | 0.0 | 1.0 | `repulsionStrength` |
| `physarum.samplingExponent` | 0.0 | 10.0 | `samplingExponent` |
| `physarum.gravityStrength` | 0.0 | 1.0 | `gravityStrength` |
| `physarum.orbitOffset` | 0.0 | 1.0 | `orbitOffset` |
| `physarum.lissajous.amplitude` | 0.0 | 0.5 | `lissajous.amplitude` |
| `physarum.lissajous.motionSpeed` | 0.0 | 10.0 | `lissajous.motionSpeed` |
| `physarum.attractorBaseRadius` | 0.1 | 0.5 | `attractorBaseRadius` |

**AttractorFlow** (6 params):

| Param ID | Min | Max | Field |
|----------|-----|-----|-------|
| `attractorFlow.rotationSpeedX` | -ROTATION_SPEED_MAX | ROTATION_SPEED_MAX | `rotationSpeedX` |
| `attractorFlow.rotationSpeedY` | -ROTATION_SPEED_MAX | ROTATION_SPEED_MAX | `rotationSpeedY` |
| `attractorFlow.rotationSpeedZ` | -ROTATION_SPEED_MAX | ROTATION_SPEED_MAX | `rotationSpeedZ` |
| `attractorFlow.rotationAngleX` | -ROTATION_OFFSET_MAX | ROTATION_OFFSET_MAX | `rotationAngleX` |
| `attractorFlow.rotationAngleY` | -ROTATION_OFFSET_MAX | ROTATION_OFFSET_MAX | `rotationAngleY` |
| `attractorFlow.rotationAngleZ` | -ROTATION_OFFSET_MAX | ROTATION_OFFSET_MAX | `rotationAngleZ` |

**ParticleLife** (11 params):

| Param ID | Min | Max | Field |
|----------|-----|-----|-------|
| `particleLife.rotationSpeedX` | -ROTATION_SPEED_MAX | ROTATION_SPEED_MAX | `rotationSpeedX` |
| `particleLife.rotationSpeedY` | -ROTATION_SPEED_MAX | ROTATION_SPEED_MAX | `rotationSpeedY` |
| `particleLife.rotationSpeedZ` | -ROTATION_SPEED_MAX | ROTATION_SPEED_MAX | `rotationSpeedZ` |
| `particleLife.rotationAngleX` | -ROTATION_OFFSET_MAX | ROTATION_OFFSET_MAX | `rotationAngleX` |
| `particleLife.rotationAngleY` | -ROTATION_OFFSET_MAX | ROTATION_OFFSET_MAX | `rotationAngleY` |
| `particleLife.rotationAngleZ` | -ROTATION_OFFSET_MAX | ROTATION_OFFSET_MAX | `rotationAngleZ` |
| `particleLife.rMax` | 0.05 | 0.5 | `rMax` |
| `particleLife.forceFactor` | 0.01 | 2.0 | `forceFactor` |
| `particleLife.momentum` | 0.1 | 0.99 | `momentum` |
| `particleLife.beta` | 0.1 | 0.9 | `beta` |
| `particleLife.evolutionSpeed` | 0.0 | 5.0 | `evolutionSpeed` |

**Boids** (3 params):

| Param ID | Min | Max | Field |
|----------|-----|-----|-------|
| `boids.cohesionWeight` | 0.0 | 2.0 | `cohesionWeight` |
| `boids.separationWeight` | 0.0 | 2.0 | `separationWeight` |
| `boids.alignmentWeight` | 0.0 | 2.0 | `alignmentWeight` |

**CurlFlow** (1 param):

| Param ID | Min | Max | Field |
|----------|-----|-----|-------|
| `curlFlow.respawnProbability` | 0.0 | 0.1 | `respawnProbability` |

**CurlAdvection** (13 params):

| Param ID | Min | Max | Field |
|----------|-----|-----|-------|
| `curlAdvection.advectionCurl` | 0.0 | 1.0 | `advectionCurl` |
| `curlAdvection.curlScale` | -4.0 | 4.0 | `curlScale` |
| `curlAdvection.selfAmp` | 0.5 | 2.0 | `selfAmp` |
| `curlAdvection.laplacianScale` | 0.0 | 0.2 | `laplacianScale` |
| `curlAdvection.pressureScale` | -4.0 | 4.0 | `pressureScale` |
| `curlAdvection.divergenceScale` | -1.0 | 1.0 | `divergenceScale` |
| `curlAdvection.divergenceUpdate` | -0.1 | 0.1 | `divergenceUpdate` |
| `curlAdvection.divergenceSmoothing` | 0.0 | 0.5 | `divergenceSmoothing` |
| `curlAdvection.updateSmoothing` | 0.25 | 0.9 | `updateSmoothing` |
| `curlAdvection.injectionIntensity` | 0.0 | 1.0 | `injectionIntensity` |
| `curlAdvection.injectionThreshold` | 0.0 | 1.0 | `injectionThreshold` |
| `curlAdvection.decayHalfLife` | 0.1 | 5.0 | `decayHalfLife` |
| `curlAdvection.boostIntensity` | 0.0 | 5.0 | `boostIntensity` |

**Cymatics** (8 params):

| Param ID | Min | Max | Field |
|----------|-----|-----|-------|
| `cymatics.waveScale` | 1.0 | 50.0 | `waveScale` |
| `cymatics.falloff` | 0.0 | 5.0 | `falloff` |
| `cymatics.visualGain` | 0.5 | 5.0 | `visualGain` |
| `cymatics.boostIntensity` | 0.0 | 5.0 | `boostIntensity` |
| `cymatics.baseRadius` | 0.0 | 0.5 | `baseRadius` |
| `cymatics.reflectionGain` | 0.0 | 1.0 | `reflectionGain` |
| `cymatics.lissajous.amplitude` | 0.0 | 0.5 | `lissajous.amplitude` |
| `cymatics.lissajous.motionSpeed` | 0.0 | 5.0 | `lissajous.motionSpeed` |

### PARAM_TABLE After Cleanup

24 entries remain:

- `effects.blurScale`, `effects.chromaticOffset`, `effects.motionScale` (3)
- `flowField.*` (14)
- `proceduralWarp.*` (3)
- `feedbackFlow.*` (4)

### Includes

Each simulation `.cpp` needs `#include "automation/modulation_engine.h"`. AttractorFlow and ParticleLife also need `#include "ui/ui_units.h"` for `ROTATION_SPEED_MAX` / `ROTATION_OFFSET_MAX`.

---

## Tasks

### Wave 1: Add RegisterParams to simulation headers (7 tasks, no file overlap)

All 7 headers are independent. Each adds one function declaration.

#### Task 1.1: Physarum RegisterParams

**Files**: `src/simulation/physarum.h`, `src/simulation/physarum.cpp`
**Creates**: `PhysarumRegisterParams` declaration and implementation

**Do**: Add declaration `void PhysarumRegisterParams(PhysarumConfig *cfg);` to `physarum.h`. Add implementation to `physarum.cpp` — 19 `ModEngineRegisterParam` calls matching the parameter table above. Add `#include "automation/modulation_engine.h"`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: AttractorFlow RegisterParams

**Files**: `src/simulation/attractor_flow.h`, `src/simulation/attractor_flow.cpp`
**Creates**: `AttractorFlowRegisterParams` declaration and implementation

**Do**: Add declaration `void AttractorFlowRegisterParams(AttractorFlowConfig *cfg);` to `attractor_flow.h`. Add implementation to `attractor_flow.cpp` — 6 `ModEngineRegisterParam` calls. Add `#include "automation/modulation_engine.h"` and `#include "ui/ui_units.h"` for rotation constants.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.3: ParticleLife RegisterParams

**Files**: `src/simulation/particle_life.h`, `src/simulation/particle_life.cpp`
**Creates**: `ParticleLifeRegisterParams` declaration and implementation

**Do**: Add declaration `void ParticleLifeRegisterParams(ParticleLifeConfig *cfg);` to `particle_life.h`. Add implementation to `particle_life.cpp` — 11 `ModEngineRegisterParam` calls. Add `#include "automation/modulation_engine.h"` and `#include "ui/ui_units.h"`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.4: Boids RegisterParams

**Files**: `src/simulation/boids.h`, `src/simulation/boids.cpp`
**Creates**: `BoidsRegisterParams` declaration and implementation

**Do**: Add declaration `void BoidsRegisterParams(BoidsConfig *cfg);` to `boids.h`. Add implementation to `boids.cpp` — 3 `ModEngineRegisterParam` calls. Add `#include "automation/modulation_engine.h"`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.5: CurlFlow RegisterParams

**Files**: `src/simulation/curl_flow.h`, `src/simulation/curl_flow.cpp`
**Creates**: `CurlFlowRegisterParams` declaration and implementation

**Do**: Add declaration `void CurlFlowRegisterParams(CurlFlowConfig *cfg);` to `curl_flow.h`. Add implementation to `curl_flow.cpp` — 1 `ModEngineRegisterParam` call. Add `#include "automation/modulation_engine.h"`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.6: CurlAdvection RegisterParams

**Files**: `src/simulation/curl_advection.h`, `src/simulation/curl_advection.cpp`
**Creates**: `CurlAdvectionRegisterParams` declaration and implementation

**Do**: Add declaration `void CurlAdvectionRegisterParams(CurlAdvectionConfig *cfg);` to `curl_advection.h`. Add implementation to `curl_advection.cpp` — 13 `ModEngineRegisterParam` calls. Add `#include "automation/modulation_engine.h"`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.7: Cymatics RegisterParams

**Files**: `src/simulation/cymatics.h`, `src/simulation/cymatics.cpp`
**Creates**: `CymaticsRegisterParams` declaration and implementation

**Do**: Add declaration `void CymaticsRegisterParams(CymaticsConfig *cfg);` to `cymatics.h`. Add implementation to `cymatics.cpp` — 8 `ModEngineRegisterParam` calls. Add `#include "automation/modulation_engine.h"`.

**Verify**: `cmake.exe --build build` compiles.

### Wave 2: Call site + PARAM_TABLE cleanup (1 task)

#### Task 2.1: Wire up calls and remove PARAM_TABLE entries

**Files**: `src/render/post_effect.cpp`, `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. In `PostEffectRegisterParams` (`src/render/post_effect.cpp`), add a `// Simulation` comment block after the generators section and call all 7 RegisterParams functions: `PhysarumRegisterParams(&pe->effects.physarum)`, `AttractorFlowRegisterParams(&pe->effects.attractorFlow)`, etc.
2. In `param_registry.cpp`, remove all simulation entries from PARAM_TABLE (lines for `physarum.*`, `attractorFlow.*`, `particleLife.*`, `boids.*`, `curlFlow.*`, `curlAdvection.*`, `cymatics.*`). PARAM_TABLE should retain only `effects.*`, `flowField.*`, `proceduralWarp.*`, `feedbackFlow.*` (24 entries).

**Verify**: `cmake.exe --build build` compiles. Verify PARAM_TABLE has 24 entries.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] PARAM_TABLE has exactly 24 entries (3 effects + 14 flowField + 3 proceduralWarp + 4 feedbackFlow)
- [ ] All 61 simulation params register through ModEngine (same IDs, same bounds as before)
- [ ] Existing presets load without errors (param IDs unchanged)
