# Effect Modules Migration: Batch 3 (Cellular)

Migrate 3 cellular effects to self-contained modules: voronoi, lattice_fold, phyllotaxis.

**Research**: [docs/research/effect-modules.md](../research/effect-modules.md)
**Plan**: [docs/plans/effect-modules-migration.md](effect-modules-migration.md)

## Specification

### Pattern

Each effect becomes a single `.h`/`.cpp` pair in `src/effects/` containing:
- Config struct (moved from `src/config/*_config.h`)
- Effect struct (shader + uniform locations + animation state)
- Init/Setup/Uninit lifecycle functions
- ConfigDefault function

### Effects to Migrate

| Effect | Config Fields | Time Accumulators | Uniform Count |
|--------|--------------|-------------------|---------------|
| voronoi | 15 | `voronoiTime` | 15 |
| lattice_fold | 5 | `currentLatticeFoldRotation` | 5 |
| phyllotaxis | 18 | `phyllotaxisAngleTime`, `phyllotaxisPhaseTime`, `phyllotaxisSpinOffset` | 17 |

### Voronoi Module

```cpp
// src/effects/voronoi.h
#ifndef VORONOI_H
#define VORONOI_H

#include "raylib.h"
#include <stdbool.h>

struct VoronoiConfig {
  bool enabled = false;
  bool smoothMode = false;
  float scale = 15.0f;
  float speed = 0.5f;
  float edgeFalloff = 0.3f;
  float isoFrequency = 10.0f;
  float uvDistortIntensity = 0.0f;
  float edgeIsoIntensity = 0.0f;
  float centerIsoIntensity = 0.0f;
  float flatFillIntensity = 0.0f;
  float organicFlowIntensity = 0.0f;
  float edgeGlowIntensity = 0.0f;
  float determinantIntensity = 0.0f;
  float ratioIntensity = 0.0f;
  float edgeDetectIntensity = 0.0f;
};

typedef struct VoronoiEffect {
  Shader shader;
  int resolutionLoc;
  int scaleLoc;
  int timeLoc;
  int edgeFalloffLoc;
  int isoFrequencyLoc;
  int smoothModeLoc;
  int uvDistortIntensityLoc;
  int edgeIsoIntensityLoc;
  int centerIsoIntensityLoc;
  int flatFillIntensityLoc;
  int organicFlowIntensityLoc;
  int edgeGlowIntensityLoc;
  int determinantIntensityLoc;
  int ratioIntensityLoc;
  int edgeDetectIntensityLoc;
  float time; // Animation accumulator
} VoronoiEffect;

bool VoronoiEffectInit(VoronoiEffect *e);
void VoronoiEffectSetup(VoronoiEffect *e, const VoronoiConfig *cfg, float deltaTime);
void VoronoiEffectUninit(VoronoiEffect *e);
VoronoiConfig VoronoiConfigDefault(void);

#endif // VORONOI_H
```

### Lattice Fold Module

```cpp
// src/effects/lattice_fold.h
#ifndef LATTICE_FOLD_H
#define LATTICE_FOLD_H

#include "raylib.h"
#include <stdbool.h>

struct LatticeFoldConfig {
  bool enabled = false;
  int cellType = 6;
  float cellScale = 8.0f;
  float rotationSpeed = 0.0f;
  float smoothing = 0.0f;
};

typedef struct LatticeFoldEffect {
  Shader shader;
  int cellTypeLoc;
  int cellScaleLoc;
  int rotationLoc;
  int timeLoc;
  int smoothingLoc;
  float rotation; // Animation accumulator
} LatticeFoldEffect;

bool LatticeFoldEffectInit(LatticeFoldEffect *e);
void LatticeFoldEffectSetup(LatticeFoldEffect *e, const LatticeFoldConfig *cfg,
                            float deltaTime, float transformTime);
void LatticeFoldEffectUninit(LatticeFoldEffect *e);
LatticeFoldConfig LatticeFoldConfigDefault(void);

#endif // LATTICE_FOLD_H
```

Note: `transformTime` passed from `pe->transformTime` for the `time` uniform.

### Phyllotaxis Module

```cpp
// src/effects/phyllotaxis.h
#ifndef PHYLLOTAXIS_H
#define PHYLLOTAXIS_H

#include "raylib.h"
#include <stdbool.h>

struct PhyllotaxisConfig {
  bool enabled = false;
  bool smoothMode = false;
  float scale = 0.06f;
  float divergenceAngle = 0.0f;
  float angleSpeed = 0.0f;
  float phaseSpeed = 0.0f;
  float spinSpeed = 0.0f;
  float cellRadius = 0.8f;
  float isoFrequency = 5.0f;
  float uvDistortIntensity = 0.0f;
  float organicFlowIntensity = 0.0f;
  float edgeIsoIntensity = 0.0f;
  float centerIsoIntensity = 0.0f;
  float flatFillIntensity = 0.0f;
  float edgeGlowIntensity = 0.0f;
  float ratioIntensity = 0.0f;
  float determinantIntensity = 0.0f;
  float edgeDetectIntensity = 0.0f;
};

typedef struct PhyllotaxisEffect {
  Shader shader;
  int resolutionLoc;
  int smoothModeLoc;
  int scaleLoc;
  int divergenceAngleLoc;
  int phaseTimeLoc;
  int cellRadiusLoc;
  int isoFrequencyLoc;
  int uvDistortIntensityLoc;
  int organicFlowIntensityLoc;
  int edgeIsoIntensityLoc;
  int centerIsoIntensityLoc;
  int flatFillIntensityLoc;
  int edgeGlowIntensityLoc;
  int ratioIntensityLoc;
  int determinantIntensityLoc;
  int edgeDetectIntensityLoc;
  int spinOffsetLoc;
  float angleTime;  // Divergence angle drift accumulator
  float phaseTime;  // Per-cell pulse animation accumulator
  float spinOffset; // Mechanical spin accumulator
} PhyllotaxisEffect;

// GOLDEN_ANGLE constant: 2.39996322972865f (used in Setup)

bool PhyllotaxisEffectInit(PhyllotaxisEffect *e);
void PhyllotaxisEffectSetup(PhyllotaxisEffect *e, const PhyllotaxisConfig *cfg, float deltaTime);
void PhyllotaxisEffectUninit(PhyllotaxisEffect *e);
PhyllotaxisConfig PhyllotaxisConfigDefault(void);

#endif // PHYLLOTAXIS_H
```

---

## Agent Pitfalls

From Batch 1/2 learnings. Each task prompt MUST include these warnings:

1. **Define config inline**: Do NOT `#include "config/*_config.h"`—those files get deleted.
2. **Correct return on failure**: `PostEffectInit` returns `PostEffect*`. Use `free(pe); return NULL;` not `return false;`.
3. **Remove stale includes**: Integration files must remove deleted config header includes.
4. **Remove time accumulators from render_pipeline.cpp**: Module Setup handles animation.
5. **Verify uniform names against shader files**: Check actual `.fs` uniform declarations.

---

## Tasks

### Wave 1: Effect Modules (3 parallel tasks)

No file overlap between modules. All create new files.

#### Task 1.1: Voronoi Module

**Files**: `src/effects/voronoi.h`, `src/effects/voronoi.cpp`
**Creates**: `VoronoiEffect`, `VoronoiConfig` types

**Build**:
1. Create `src/effects/voronoi.h` per specification above
2. Create `src/effects/voronoi.cpp`:
   - `VoronoiEffectInit`: Load shader `"shaders/voronoi.fs"`, cache all 15 uniform locations (resolution, scale, time, edgeFalloff, isoFrequency, smoothMode, + 9 intensity uniforms), init `time = 0.0f`
   - `VoronoiEffectSetup`: Accumulate `time += cfg->speed * deltaTime`, set all uniforms (convert `smoothMode` bool to int)
   - `VoronoiEffectUninit`: Unload shader
   - `VoronoiConfigDefault`: Return struct with defaults from spec

**WARNING**: Do NOT include `config/voronoi_config.h`. Define `VoronoiConfig` in the header.

**Verify**: Compiles with `cmake.exe --build build`.

---

#### Task 1.2: Lattice Fold Module

**Files**: `src/effects/lattice_fold.h`, `src/effects/lattice_fold.cpp`
**Creates**: `LatticeFoldEffect`, `LatticeFoldConfig` types

**Build**:
1. Create `src/effects/lattice_fold.h` per specification above
2. Create `src/effects/lattice_fold.cpp`:
   - `LatticeFoldEffectInit`: Load shader `"shaders/lattice_fold.fs"`, cache 5 uniform locations (cellType, cellScale, rotation, time, smoothing), init `rotation = 0.0f`
   - `LatticeFoldEffectSetup`: Accumulate `rotation += cfg->rotationSpeed * deltaTime`, set all uniforms. Note: `time` uniform uses passed `transformTime` parameter, not internal state.
   - `LatticeFoldEffectUninit`: Unload shader
   - `LatticeFoldConfigDefault`: Return struct with defaults from spec

**WARNING**: Do NOT include `config/lattice_fold_config.h`. Define `LatticeFoldConfig` in the header.

**Verify**: Compiles with `cmake.exe --build build`.

---

#### Task 1.3: Phyllotaxis Module

**Files**: `src/effects/phyllotaxis.h`, `src/effects/phyllotaxis.cpp`
**Creates**: `PhyllotaxisEffect`, `PhyllotaxisConfig` types

**Build**:
1. Create `src/effects/phyllotaxis.h` per specification above
2. Create `src/effects/phyllotaxis.cpp`:
   - Define `static const float GOLDEN_ANGLE = 2.39996322972865f;`
   - `PhyllotaxisEffectInit`: Load shader `"shaders/phyllotaxis.fs"`, cache 17 uniform locations, init all 3 time accumulators to 0.0f
   - `PhyllotaxisEffectSetup`:
     - Accumulate: `angleTime += cfg->angleSpeed * deltaTime`, `phaseTime += cfg->phaseSpeed * deltaTime`, `spinOffset += cfg->spinSpeed * deltaTime`
     - Compute `divergenceAngle = GOLDEN_ANGLE + cfg->divergenceAngle + angleTime`
     - Set all uniforms (convert `smoothMode` bool to int)
   - `PhyllotaxisEffectUninit`: Unload shader
   - `PhyllotaxisConfigDefault`: Return struct with defaults from spec

**WARNING**: Do NOT include `config/phyllotaxis_config.h`. Define `PhyllotaxisConfig` in the header.

**Verify**: Compiles with `cmake.exe --build build`.

---

### Wave 2: Integration (7 parallel tasks)

All tasks depend on Wave 1. No file overlap between tasks.

#### Task 2.1: Update post_effect.h

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add includes at top (with other effect includes):
   ```cpp
   #include "effects/lattice_fold.h"
   #include "effects/phyllotaxis.h"
   #include "effects/voronoi.h"
   ```
2. Add effect module fields (near other effect modules like `KaleidoscopeEffect`):
   ```cpp
   VoronoiEffect voronoi;
   LatticeFoldEffect latticeFold;
   PhyllotaxisEffect phyllotaxis;
   ```
3. Remove shader fields: `voronoiShader`, `latticeFoldShader`, `phyllotaxisShader`
4. Remove ALL uniform location fields for these 3 effects (lines 107-126 for lattice/voronoi, lines 343-359 for phyllotaxis)
5. Remove animation state fields: `voronoiTime`, `currentLatticeFoldRotation`, `phyllotaxisAngleTime`, `phyllotaxisPhaseTime`, `phyllotaxisSpinOffset`

**Verify**: Header compiles (other files will fail until updated).

---

#### Task 2.2: Update post_effect.cpp

**Files**: `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `PostEffectInit`:
   - Replace shader loading for voronoi/lattice_fold/phyllotaxis with effect init calls:
     ```cpp
     if (!VoronoiEffectInit(&pe->voronoi)) { free(pe); return NULL; }
     if (!LatticeFoldEffectInit(&pe->latticeFold)) { free(pe); return NULL; }
     if (!PhyllotaxisEffectInit(&pe->phyllotaxis)) { free(pe); return NULL; }
     ```
   - Remove the shader load lines and shader.id checks for these 3 effects
   - Remove ALL `GetShaderLocation` calls for these 3 effects
   - Remove `pe->voronoiTime = 0.0f;` initialization
2. In `PostEffectUpdateResolution`:
   - Replace `SetShaderValue(pe->voronoiShader, pe->voronoiResolutionLoc, ...)` with `SetShaderValue(pe->voronoi.shader, pe->voronoi.resolutionLoc, ...)`
   - Replace `SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisResolutionLoc, ...)` with `SetShaderValue(pe->phyllotaxis.shader, pe->phyllotaxis.resolutionLoc, ...)`
3. In `PostEffectUninit`:
   - Replace `UnloadShader(pe->voronoiShader);` with `VoronoiEffectUninit(&pe->voronoi);`
   - Replace `UnloadShader(pe->latticeFoldShader);` with `LatticeFoldEffectUninit(&pe->latticeFold);`
   - Replace `UnloadShader(pe->phyllotaxisShader);` with `PhyllotaxisEffectUninit(&pe->phyllotaxis);`

**WARNING**: Use `free(pe); return NULL;` on init failure, NOT `return false;`.

**Verify**: Compiles with `cmake.exe --build build`.

---

#### Task 2.3: Update shader_setup_cellular.cpp

**Files**: `src/render/shader_setup_cellular.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add includes:
   ```cpp
   #include "effects/lattice_fold.h"
   #include "effects/phyllotaxis.h"
   #include "effects/voronoi.h"
   ```
2. Replace `SetupVoronoi`:
   ```cpp
   void SetupVoronoi(PostEffect *pe) {
     VoronoiEffectSetup(&pe->voronoi, &pe->effects.voronoi, pe->currentDeltaTime);
   }
   ```
3. Replace `SetupLatticeFold`:
   ```cpp
   void SetupLatticeFold(PostEffect *pe) {
     LatticeFoldEffectSetup(&pe->latticeFold, &pe->effects.latticeFold,
                            pe->currentDeltaTime, pe->transformTime);
   }
   ```
4. Replace `SetupPhyllotaxis`:
   ```cpp
   void SetupPhyllotaxis(PostEffect *pe) {
     PhyllotaxisEffectSetup(&pe->phyllotaxis, &pe->effects.phyllotaxis, pe->currentDeltaTime);
   }
   ```
5. Remove `static const float GOLDEN_ANGLE` (now in phyllotaxis.cpp)

**Verify**: Compiles with `cmake.exe --build build`.

---

#### Task 2.4: Update shader_setup.cpp

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Update dispatcher entries for TRANSFORM_VORONOI, TRANSFORM_LATTICE_FOLD, TRANSFORM_PHYLLOTAXIS:
   - Change `&pe->voronoiShader` to `&pe->voronoi.shader`
   - Change `&pe->latticeFoldShader` to `&pe->latticeFold.shader`
   - Change `&pe->phyllotaxisShader` to `&pe->phyllotaxis.shader`

**Verify**: Compiles with `cmake.exe --build build`.

---

#### Task 2.5: Update render_pipeline.cpp

**Files**: `src/render/render_pipeline.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Remove time accumulator lines (module Setup handles these):
   ```cpp
   // DELETE: pe->voronoiTime += deltaTime * pe->effects.voronoi.speed;
   // DELETE: pe->currentLatticeFoldRotation += pe->effects.latticeFold.rotationSpeed * dt;
   // DELETE: pe->phyllotaxisAngleTime += pe->effects.phyllotaxis.angleSpeed * dt;
   // DELETE: pe->phyllotaxisPhaseTime += pe->effects.phyllotaxis.phaseSpeed * dt;
   // DELETE: pe->phyllotaxisSpinOffset += pe->effects.phyllotaxis.spinSpeed * dt;
   ```

**Verify**: Compiles with `cmake.exe --build build`.

---

#### Task 2.6: Update effect_config.h and preset.cpp

**Files**: `src/config/effect_config.h`, `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `effect_config.h`:
   - Replace `#include "lattice_fold_config.h"` with `#include "effects/lattice_fold.h"`
   - Replace `#include "phyllotaxis_config.h"` with `#include "effects/phyllotaxis.h"`
   - Replace `#include "voronoi_config.h"` with `#include "effects/voronoi.h"`
2. In `preset.cpp`:
   - Remove `#include "config/lattice_fold_config.h"` (line 8)
   - The other two config headers are already included via effect_config.h

**Verify**: Compiles with `cmake.exe --build build`.

---

#### Task 2.7: Update imgui_effects_cellular.cpp

**Files**: `src/ui/imgui_effects_cellular.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Remove stale includes:
   ```cpp
   // DELETE: #include "config/lattice_fold_config.h"
   // DELETE: #include "config/phyllotaxis_config.h"
   // DELETE: #include "config/voronoi_config.h"
   ```
   (effect_config.h already includes the effect headers)

**Verify**: Compiles with `cmake.exe --build build`.

---

### Wave 3: Cleanup

#### Task 3.1: Delete Old Config Files

**Files**: Delete `src/config/voronoi_config.h`, `src/config/lattice_fold_config.h`, `src/config/phyllotaxis_config.h`
**Depends on**: Wave 2 complete

**Build**:
1. Delete the 3 config header files
2. Verify no remaining includes with: `grep -r "voronoi_config.h\|lattice_fold_config.h\|phyllotaxis_config.h" src/`

**Verify**: Build succeeds with `cmake.exe --build build`.

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe --build build 2>&1 | grep -i warning`
- [ ] No stale config includes: `grep -r "voronoi_config.h\|lattice_fold_config.h\|phyllotaxis_config.h" src/`
- [ ] Effect modules exist: `ls src/effects/voronoi.* src/effects/lattice_fold.* src/effects/phyllotaxis.*`
- [ ] Old config files deleted: `ls src/config/voronoi_config.h` returns "not found"
- [ ] Mark batch 3 complete in `docs/plans/effect-modules-migration.md`
