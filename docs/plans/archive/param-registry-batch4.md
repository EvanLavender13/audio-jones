# Param Registry Batch 4: Color + Generators + Cleanup

Migrate the final 7 effects from PARAM_TABLE to per-module RegisterParams functions. Remove all migrated entries from PARAM_TABLE. After this batch, PARAM_TABLE retains only simulation/feedback/flow/top-level params.

**Parent plan**: [docs/plans/param-registry-migration.md](param-registry-migration.md)

## Design

### Pattern

Each effect gains a `RegisterParams` function that calls `ModEngineRegisterParam` for each modulatable float param. Identical to batches 1-3.

**Declaration** (in `.h`):
```
void <Effect>RegisterParams(<Effect>Config *cfg);
```

**Implementation** (in `.cpp`):
```
void <Effect>RegisterParams(<Effect>Config *cfg) {
  ModEngineRegisterParam("<paramId>", &cfg-><field>, <min>, <max>);
  ...
}
```

**Include**: `#include "automation/modulation_engine.h"` in each `.cpp` that gains RegisterParams.

### Parameters

#### color_grade (8 params)

| Param ID | Field | Min | Max |
|----------|-------|-----|-----|
| colorGrade.hueShift | hueShift | 0.0f | 1.0f |
| colorGrade.saturation | saturation | 0.0f | 2.0f |
| colorGrade.brightness | brightness | -2.0f | 2.0f |
| colorGrade.contrast | contrast | 0.5f | 2.0f |
| colorGrade.temperature | temperature | -1.0f | 1.0f |
| colorGrade.shadowsOffset | shadowsOffset | -0.5f | 0.5f |
| colorGrade.midtonesOffset | midtonesOffset | -0.5f | 0.5f |
| colorGrade.highlightsOffset | highlightsOffset | -0.5f | 0.5f |

#### false_color (1 param)

| Param ID | Field | Min | Max |
|----------|-------|-----|-----|
| falseColor.intensity | intensity | 0.0f | 1.0f |

#### palette_quantization (2 params)

| Param ID | Field | Min | Max |
|----------|-------|-----|-----|
| paletteQuantization.colorLevels | colorLevels | 2.0f | 16.0f |
| paletteQuantization.ditherStrength | ditherStrength | 0.0f | 1.0f |

#### plasma (7 params)

| Param ID | Field | Min | Max |
|----------|-------|-----|-----|
| plasma.animSpeed | animSpeed | 0.0f | 5.0f |
| plasma.coreBrightness | coreBrightness | 0.5f | 3.0f |
| plasma.displacement | displacement | 0.0f | 2.0f |
| plasma.driftAmount | driftAmount | 0.0f | 1.0f |
| plasma.driftSpeed | driftSpeed | 0.0f | 2.0f |
| plasma.flickerAmount | flickerAmount | 0.0f | 1.0f |
| plasma.glowRadius | glowRadius | 0.01f | 0.3f |

#### interference (9 params)

| Param ID | Field | Min | Max |
|----------|-------|-----|-----|
| interference.baseRadius | baseRadius | 0.0f | 1.0f |
| interference.chromaSpread | chromaSpread | 0.0f | 0.1f |
| interference.falloffStrength | falloffStrength | 0.0f | 5.0f |
| interference.lissajous.amplitude | lissajous.amplitude | 0.0f | 0.5f |
| interference.lissajous.motionSpeed | lissajous.motionSpeed | 0.0f | 5.0f |
| interference.reflectionGain | reflectionGain | 0.0f | 1.0f |
| interference.visualGain | visualGain | 0.5f | 5.0f |
| interference.waveFreq | waveFreq | 5.0f | 100.0f |
| interference.waveSpeed | waveSpeed | 0.0f | 10.0f |

#### constellation (9 params)

| Param ID | Field | Min | Max |
|----------|-------|-----|-----|
| constellation.animSpeed | animSpeed | 0.0f | 5.0f |
| constellation.gridScale | gridScale | 5.0f | 50.0f |
| constellation.lineOpacity | lineOpacity | 0.0f | 1.0f |
| constellation.maxLineLen | maxLineLen | 0.5f | 2.0f |
| constellation.pointBrightness | pointBrightness | 0.0f | 2.0f |
| constellation.pointSize | pointSize | 0.3f | 3.0f |
| constellation.radialAmp | radialAmp | 0.0f | 4.0f |
| constellation.radialSpeed | radialSpeed | 0.0f | 5.0f |
| constellation.wanderAmp | wanderAmp | 0.0f | 0.5f |

#### synthwave (6 params)

| Param ID | Field | Min | Max |
|----------|-------|-----|-----|
| synthwave.horizonY | horizonY | 0.3f | 0.7f |
| synthwave.colorMix | colorMix | 0.0f | 1.0f |
| synthwave.gridOpacity | gridOpacity | 0.0f | 1.0f |
| synthwave.gridGlow | gridGlow | 1.0f | 3.0f |
| synthwave.stripeIntensity | stripeIntensity | 0.0f | 1.0f |
| synthwave.horizonIntensity | horizonIntensity | 0.0f | 1.0f |

---

## Tasks

### Wave 1: Add RegisterParams to all 7 effects (parallel — no file overlap)

#### Task 1.1: color_grade RegisterParams

**Files**: `src/effects/color_grade.h`, `src/effects/color_grade.cpp`

**Do**:
- Add declaration to header: `void ColorGradeRegisterParams(ColorGradeConfig *cfg);`
- Add implementation to .cpp: 8 `ModEngineRegisterParam` calls per table above
- Add `#include "automation/modulation_engine.h"` to .cpp

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: false_color RegisterParams

**Files**: `src/effects/false_color.h`, `src/effects/false_color.cpp`

**Do**:
- Add declaration to header: `void FalseColorRegisterParams(FalseColorConfig *cfg);`
- Add implementation to .cpp: 1 `ModEngineRegisterParam` call per table above
- Add `#include "automation/modulation_engine.h"` to .cpp

**Verify**: Compiles.

#### Task 1.3: palette_quantization RegisterParams

**Files**: `src/effects/palette_quantization.h`, `src/effects/palette_quantization.cpp`

**Do**:
- Add declaration to header: `void PaletteQuantizationRegisterParams(PaletteQuantizationConfig *cfg);`
- Add implementation to .cpp: 2 `ModEngineRegisterParam` calls per table above
- Add `#include "automation/modulation_engine.h"` to .cpp

**Verify**: Compiles.

#### Task 1.4: plasma RegisterParams

**Files**: `src/effects/plasma.h`, `src/effects/plasma.cpp`

**Do**:
- Add declaration to header: `void PlasmaRegisterParams(PlasmaConfig *cfg);`
- Add implementation to .cpp: 7 `ModEngineRegisterParam` calls per table above
- Add `#include "automation/modulation_engine.h"` to .cpp

**Verify**: Compiles.

#### Task 1.5: interference RegisterParams

**Files**: `src/effects/interference.h`, `src/effects/interference.cpp`

**Do**:
- Add declaration to header: `void InterferenceRegisterParams(InterferenceConfig *cfg);`
- Add implementation to .cpp: 9 `ModEngineRegisterParam` calls per table above. Note nested `lissajous.amplitude` and `lissajous.motionSpeed` fields.
- Add `#include "automation/modulation_engine.h"` to .cpp

**Verify**: Compiles.

#### Task 1.6: constellation RegisterParams

**Files**: `src/effects/constellation.h`, `src/effects/constellation.cpp`

**Do**:
- Add declaration to header: `void ConstellationRegisterParams(ConstellationConfig *cfg);`
- Add implementation to .cpp: 9 `ModEngineRegisterParam` calls per table above
- Add `#include "automation/modulation_engine.h"` to .cpp

**Verify**: Compiles.

#### Task 1.7: synthwave RegisterParams

**Files**: `src/effects/synthwave.h`, `src/effects/synthwave.cpp`

**Do**:
- Add declaration to header: `void SynthwaveRegisterParams(SynthwaveConfig *cfg);`
- Add implementation to .cpp: 6 `ModEngineRegisterParam` calls per table above
- Add `#include "automation/modulation_engine.h"` to .cpp

**Verify**: Compiles.

---

### Wave 2: Dispatcher + Cleanup (sequential — both touch param_registry.cpp and post_effect.cpp)

#### Task 2.1: Add RegisterParams calls to PostEffectRegisterParams

**Files**: `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Do**: Add calls to `PostEffectRegisterParams` in three new category sections after the existing Retro section:

```
// Color effects
ColorGradeRegisterParams(&pe->effects.colorGrade);
FalseColorRegisterParams(&pe->effects.falseColor);
PaletteQuantizationRegisterParams(&pe->effects.paletteQuantization);

// Generator effects
PlasmaRegisterParams(&pe->effects.plasma);
InterferenceRegisterParams(&pe->effects.interference);
ConstellationRegisterParams(&pe->effects.constellation);

// Graphic effects (continued)
SynthwaveRegisterParams(&pe->effects.synthwave);
```

Synthwave appends to the existing Graphic section (where `shader_setup_graphic.cpp` already handles it).

No new `#include` needed — `post_effect.h` already includes all 7 effect headers.

**Verify**: Compiles.

#### Task 2.2: Remove migrated entries from PARAM_TABLE

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Task 2.1 complete

**Do**: Remove all 42 entries for: `interference.*`, `plasma.*`, `colorGrade.*`, `falseColor.*`, `constellation.*`, `paletteQuantization.*`, `synthwave.*` from `PARAM_TABLE`.

PARAM_TABLE retains only: physarum, effects.*, flowField, proceduralWarp, feedbackFlow, attractorFlow, particleLife, boids, curlFlow, curlAdvection, cymatics.

**Verify**: Compiles. All retained categories are simulation/feedback/flow/top-level.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe --build build`
- [ ] PARAM_TABLE contains only simulation/feedback/flow/top-level entries
- [ ] `PostEffectRegisterParams` calls all 7 new RegisterParams functions
- [ ] Each effect .h declares RegisterParams, each .cpp implements it
- [ ] Param IDs and bounds match the tables above exactly
