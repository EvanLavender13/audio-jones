# Effect Modules Migration - Batch 2 (Symmetry Effects)

Migrate 7 symmetry effects from scattered PostEffect fields into self-contained modules at `src/effects/`. Each effect becomes a single header containing config struct, runtime state, and lifecycle functions, plus a cpp file with the implementation.

**Effects in this batch**: kaleidoscope, kifs, poincare_disk, mandelbox, triangle_fold, moire_interference, radial_ifs

<!-- Intentional deviation: radial_pulse is listed under Batch 2 in research doc, but it was already migrated in Batch 1 (its setup function lived in shader_setup_warp.cpp). The research doc count "7 effects" is correct; it just lists 8 names in error. -->

---

## Agent Pitfall Warnings

**READ THESE BEFORE IMPLEMENTING:**

1. **Effect modules define config inline**: Do NOT include `config/*_config.h`—they get DELETED. Define `<Name>Config` struct directly in the effect header.

2. **PostEffectInit returns `PostEffect*`**: On failure, use `free(pe); return NULL;` not `return false;`.

3. **Remove stale includes**: After migration, grep `imgui_effects_symmetry.cpp` and `preset.cpp` for stale `#include.*_config.h` patterns—remove them.

4. **Remove time accumulators from render_pipeline.cpp**: Lines like `pe->currentKaleidoRotation += ...` must be deleted—the module's Setup function handles this now.

5. **Verify uniform names against shaders**: Before using a uniform name, check the actual `.fs` file. Example: kifs uses `kifsOffset` not `offset`.

---

## Specification

### Module Pattern

Each effect module follows the established pattern:

```cpp
// src/effects/<name>.h
#ifndef <NAME>_H
#define <NAME>_H

#include "raylib.h"
#include <stdbool.h>

// Config struct (user-facing parameters, serialized in presets)
struct <Name>Config { ... };

// Runtime state (shader + uniforms + animation accumulators)
typedef struct <Name>Effect {
  Shader shader;
  int <uniform>Loc;  // One per shader uniform
  float <state>;     // Animation accumulators
} <Name>Effect;

// Lifecycle functions
bool <Name>EffectInit(<Name>Effect* e);
void <Name>EffectSetup(<Name>Effect* e, const <Name>Config* cfg, float deltaTime);
void <Name>EffectUninit(<Name>Effect* e);
<Name>Config <Name>ConfigDefault(void);

#endif
```

---

### Effect Specifications

#### 1. Kaleidoscope

**Config** (from `src/config/kaleidoscope_config.h`):
```cpp
struct KaleidoscopeConfig {
  bool enabled = false;
  int segments = 6;           // Mirror segments / wedge count (1-12)
  float rotationSpeed = 0.0f; // Rotation rate (radians/second)
  float twistAngle = 0.0f;    // Radial twist offset (radians)
  float smoothing = 0.0f;     // Blend width at wedge seams (0.0-0.5, 0 = hard edge)
};
```

**Runtime**:
```cpp
typedef struct KaleidoscopeEffect {
  Shader shader;
  int segmentsLoc;
  int rotationLoc;
  int twistAngleLoc;
  int smoothingLoc;
  float rotation;  // Animation accumulator
} KaleidoscopeEffect;
```

**Shader uniforms** (from `shaders/kaleidoscope.fs`):
- `segments` (int)
- `rotation` (float)
- `twistAngle` (float)
- `smoothing` (float)

**Setup logic**:
- Accumulate `rotation += cfg->rotationSpeed * deltaTime`
- Set all uniforms

---

#### 2. Kifs

**Config** (from `src/config/kifs_config.h`):
```cpp
struct KifsConfig {
  bool enabled = false;
  int iterations = 4;         // Fold/scale/translate cycles (1-6)
  float scale = 2.0f;         // Expansion factor per iteration (1.5-2.5)
  float offsetX = 1.0f;       // X translation after fold (0.0-2.0)
  float offsetY = 1.0f;       // Y translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
  bool octantFold = false;    // Enable 8-way octant symmetry
  bool polarFold = false;     // Enable polar coordinate pre-fold
  int polarFoldSegments = 6;  // Wedge count for polar fold (2-12)
  float polarFoldSmoothing = 0.0f; // Blend width at polar fold seams (0.0-0.5)
};
```

**Runtime**:
```cpp
typedef struct KifsEffect {
  Shader shader;
  int rotationLoc;
  int twistAngleLoc;
  int iterationsLoc;
  int scaleLoc;
  int offsetLoc;  // vec2: kifsOffset
  int octantFoldLoc;
  int polarFoldLoc;
  int polarFoldSegmentsLoc;
  int polarFoldSmoothingLoc;
  float rotation;  // Animation accumulator
  float twist;     // Per-iteration rotation accumulator
} KifsEffect;
```

**Shader uniforms** (from `shaders/kifs.fs`):
- `iterations` (int)
- `scale` (float)
- `kifsOffset` (vec2)
- `rotation` (float)
- `twistAngle` (float)
- `octantFold` (bool)
- `polarFold` (bool)
- `polarFoldSegments` (int)
- `polarFoldSmoothing` (float)

**Setup logic**:
- Accumulate `rotation += cfg->rotationSpeed * deltaTime`
- Accumulate `twist += cfg->twistSpeed * deltaTime`
- Pack `offsetX`, `offsetY` into vec2 for `kifsOffset`
- Convert bools to ints for `octantFold`, `polarFold`
- Set all uniforms

---

#### 3. PoincareDisk

**Config** (from `src/config/poincare_disk_config.h`):
```cpp
struct PoincareDiskConfig {
  bool enabled = false;
  int tileP = 4;                   // Angle at origin vertex (pi/P), range 2-12
  int tileQ = 4;                   // Angle at second vertex (pi/Q), range 2-12
  int tileR = 4;                   // Angle at third vertex (pi/R), range 2-12
  float translationX = 0.0f;       // Mobius translation center X (-0.9 to 0.9)
  float translationY = 0.0f;       // Mobius translation center Y (-0.9 to 0.9)
  float translationSpeed = 0.0f;   // Circular motion angular velocity (radians/frame)
  float translationAmplitude = 0.0f; // Circular motion radius (0.0-0.9)
  float diskScale = 1.0f;          // Disk size relative to screen (0.5-2.0)
  float rotationSpeed = 0.0f;      // Euclidean rotation speed (radians/frame)
};
```

**Runtime**:
```cpp
typedef struct PoincareDiskEffect {
  Shader shader;
  int tilePLoc;
  int tileQLoc;
  int tileRLoc;
  int translationLoc;  // vec2
  int rotationLoc;
  int diskScaleLoc;
  float time;           // For circular translation motion
  float rotation;       // Euclidean rotation accumulator
  float currentTranslation[2];
} PoincareDiskEffect;
```

**Shader uniforms** (from `shaders/poincare_disk.fs`):
- `tileP` (int)
- `tileQ` (int)
- `tileR` (int)
- `translation` (vec2)
- `rotation` (float)
- `diskScale` (float)

**Setup logic**:
- Accumulate `rotation += cfg->rotationSpeed * deltaTime`
- Accumulate `time += cfg->translationSpeed * deltaTime`
- Compute `currentTranslation[0] = cfg->translationX + cfg->translationAmplitude * sinf(time)`
- Compute `currentTranslation[1] = cfg->translationY + cfg->translationAmplitude * cosf(time)`
- Set all uniforms

---

#### 4. Mandelbox

**Config** (from `src/config/mandelbox_config.h`):
```cpp
struct MandelboxConfig {
  bool enabled = false;
  int iterations = 2;         // Fold/scale/translate cycles (1-6)
  float boxLimit = 1.0f;      // Box fold boundary (0.5-2.0)
  float sphereMin = 0.5f;     // Inner sphere radius (0.1-0.5)
  float sphereMax = 1.0f;     // Outer sphere radius (0.5-2.0)
  float scale = -2.0f;        // Scale factor per iteration (-3.0 to 3.0)
  float offsetX = 1.0f;       // X translation after fold (0.0-2.0)
  float offsetY = 1.0f;       // Y translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
  float boxIntensity = 1.0f;  // Box fold contribution (0.0-1.0)
  float sphereIntensity = 1.0f; // Sphere fold contribution (0.0-1.0)
  bool polarFold = false;     // Enable polar coordinate pre-fold
  int polarFoldSegments = 6;  // Wedge count for polar fold (2-12)
};
```

**Runtime**:
```cpp
typedef struct MandelboxEffect {
  Shader shader;
  int iterationsLoc;
  int boxLimitLoc;
  int sphereMinLoc;
  int sphereMaxLoc;
  int scaleLoc;
  int offsetLoc;  // vec2: mandelboxOffset
  int rotationLoc;
  int twistAngleLoc;
  int boxIntensityLoc;
  int sphereIntensityLoc;
  int polarFoldLoc;
  int polarFoldSegmentsLoc;
  float rotation;  // Animation accumulator
  float twist;     // Per-iteration rotation accumulator
} MandelboxEffect;
```

**Shader uniforms** (from `shaders/mandelbox.fs`):
- `iterations` (int)
- `boxLimit` (float)
- `sphereMin` (float)
- `sphereMax` (float)
- `scale` (float)
- `mandelboxOffset` (vec2)
- `rotation` (float)
- `twistAngle` (float)
- `boxIntensity` (float)
- `sphereIntensity` (float)
- `polarFold` (bool)
- `polarFoldSegments` (int)

**Setup logic**:
- Accumulate `rotation += cfg->rotationSpeed * deltaTime`
- Accumulate `twist += cfg->twistSpeed * deltaTime`
- Pack `offsetX`, `offsetY` into vec2 for `mandelboxOffset`
- Convert bool to int for `polarFold`
- Set all uniforms

---

#### 5. TriangleFold

**Config** (from `src/config/triangle_fold_config.h`):
```cpp
struct TriangleFoldConfig {
  bool enabled = false;
  int iterations = 3;         // Recursion depth (1-6)
  float scale = 2.0f;         // Expansion per iteration (1.5-2.5)
  float offsetX = 0.5f;       // X translation after fold (0.0-2.0)
  float offsetY = 0.5f;       // Y translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
};
```

**Runtime**:
```cpp
typedef struct TriangleFoldEffect {
  Shader shader;
  int iterationsLoc;
  int scaleLoc;
  int offsetLoc;  // vec2: triangleOffset
  int rotationLoc;
  int twistAngleLoc;
  float rotation;  // Animation accumulator
  float twist;     // Per-iteration rotation accumulator
} TriangleFoldEffect;
```

**Shader uniforms** (from `shaders/triangle_fold.fs`):
- `iterations` (int)
- `scale` (float)
- `triangleOffset` (vec2)
- `rotation` (float)
- `twistAngle` (float)

**Setup logic**:
- Accumulate `rotation += cfg->rotationSpeed * deltaTime`
- Accumulate `twist += cfg->twistSpeed * deltaTime`
- Pack `offsetX`, `offsetY` into vec2 for `triangleOffset`
- Set all uniforms

---

#### 6. MoireInterference

**Config** (from `src/config/moire_interference_config.h`):
```cpp
struct MoireInterferenceConfig {
  bool enabled = false;
  float rotationAngle = 0.087f;  // Angle between layers (radians, ~5°)
  float scaleDiff = 1.02f;       // Scale ratio between layers
  int layers = 2;                // Number of overlaid samples (2-4)
  int blendMode = 0;             // 0=multiply, 1=min, 2=average, 3=difference
  float centerX = 0.5f;          // Rotation/scale center X
  float centerY = 0.5f;          // Rotation/scale center Y
  float animationSpeed = 0.017f; // Rotation rate (radians/second, ~1°/s)
};
```

**Runtime**:
```cpp
typedef struct MoireInterferenceEffect {
  Shader shader;
  int rotationAngleLoc;
  int scaleDiffLoc;
  int layersLoc;
  int blendModeLoc;
  int centerXLoc;
  int centerYLoc;
  int rotationAccumLoc;
  float rotationAccum;  // Animation accumulator
} MoireInterferenceEffect;
```

**Shader uniforms** (from `shaders/moire_interference.fs`):
- `rotationAngle` (float)
- `scaleDiff` (float)
- `layers` (int)
- `blendMode` (int)
- `centerX` (float)
- `centerY` (float)
- `rotationAccum` (float)

**Setup logic**:
- Accumulate `rotationAccum += cfg->animationSpeed * deltaTime`
- Set all uniforms

---

#### 7. RadialIfs

**Config** (from `src/config/radial_ifs_config.h`):
```cpp
struct RadialIfsConfig {
  bool enabled = false;
  int segments = 6;           // Wedge count per fold (3-12)
  int iterations = 4;         // Recursion depth (1-8)
  float scale = 1.8f;         // Expansion per iteration (1.2-2.5)
  float offset = 0.5f;        // Translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
  float smoothing = 0.0f;     // Blend width at wedge seams (0.0-0.5)
};
```

**Runtime**:
```cpp
typedef struct RadialIfsEffect {
  Shader shader;
  int segmentsLoc;
  int iterationsLoc;
  int scaleLoc;
  int offsetLoc;
  int rotationLoc;
  int twistAngleLoc;
  int smoothingLoc;
  float rotation;  // Animation accumulator
  float twist;     // Per-iteration rotation accumulator
} RadialIfsEffect;
```

**Shader uniforms** (from `shaders/radial_ifs.fs`):
- `segments` (int)
- `iterations` (int)
- `scale` (float)
- `offset` (float)
- `rotation` (float)
- `twistAngle` (float)
- `smoothing` (float)

**Setup logic**:
- Accumulate `rotation += cfg->rotationSpeed * deltaTime`
- Accumulate `twist += cfg->twistSpeed * deltaTime`
- Set all uniforms

---

## Tasks

### Wave 1: Create Effect Modules

All 7 effect modules can be created in parallel since they have no file dependencies on each other.

#### Task 1.1: Create kaleidoscope module

**Files**: `src/effects/kaleidoscope.h`, `src/effects/kaleidoscope.cpp`

**Creates**: `KaleidoscopeConfig`, `KaleidoscopeEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `KaleidoscopeConfig` struct per spec (copy from config header, then delete that file later)
3. Define `KaleidoscopeEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - `KaleidoscopeEffectInit`: Load `"shaders/kaleidoscope.fs"`, cache 4 uniform locs, init `rotation = 0.0f`
   - `KaleidoscopeEffectSetup`: Accumulate rotation, set all uniforms
   - `KaleidoscopeEffectUninit`: Unload shader
   - `KaleidoscopeConfigDefault`: Return defaults

**Verify**: Files compile standalone with raylib headers.

---

#### Task 1.2: Create kifs module

**Files**: `src/effects/kifs.h`, `src/effects/kifs.cpp`

**Creates**: `KifsConfig`, `KifsEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `KifsConfig` struct per spec
3. Define `KifsEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load `"shaders/kifs.fs"`, cache 9 uniform locs, init `rotation = 0.0f`, `twist = 0.0f`
   - Setup: Accumulate rotation and twist, pack offset into vec2, convert bools to ints, set uniforms
   - **CRITICAL**: Use `"kifsOffset"` for the offset uniform (verify against shader)
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.3: Create poincare_disk module

**Files**: `src/effects/poincare_disk.h`, `src/effects/poincare_disk.cpp`

**Creates**: `PoincareDiskConfig`, `PoincareDiskEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`, `#include <math.h>`
2. Define `PoincareDiskConfig` struct per spec
3. Define `PoincareDiskEffect` struct per spec (include `currentTranslation[2]`)
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load `"shaders/poincare_disk.fs"`, cache 6 uniform locs, init `time = 0.0f`, `rotation = 0.0f`, zero currentTranslation
   - Setup: Accumulate time and rotation, compute circular translation with sin/cos, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.4: Create mandelbox module

**Files**: `src/effects/mandelbox.h`, `src/effects/mandelbox.cpp`

**Creates**: `MandelboxConfig`, `MandelboxEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `MandelboxConfig` struct per spec
3. Define `MandelboxEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load `"shaders/mandelbox.fs"`, cache 12 uniform locs, init `rotation = 0.0f`, `twist = 0.0f`
   - Setup: Accumulate rotation and twist, pack offset into vec2, convert bool to int, set uniforms
   - **CRITICAL**: Use `"mandelboxOffset"` for the offset uniform (verify against shader)
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.5: Create triangle_fold module

**Files**: `src/effects/triangle_fold.h`, `src/effects/triangle_fold.cpp`

**Creates**: `TriangleFoldConfig`, `TriangleFoldEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `TriangleFoldConfig` struct per spec
3. Define `TriangleFoldEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load `"shaders/triangle_fold.fs"`, cache 5 uniform locs, init `rotation = 0.0f`, `twist = 0.0f`
   - Setup: Accumulate rotation and twist, pack offset into vec2, set uniforms
   - **CRITICAL**: Use `"triangleOffset"` for the offset uniform (verify against shader)
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.6: Create moire_interference module

**Files**: `src/effects/moire_interference.h`, `src/effects/moire_interference.cpp`

**Creates**: `MoireInterferenceConfig`, `MoireInterferenceEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `MoireInterferenceConfig` struct per spec
3. Define `MoireInterferenceEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load `"shaders/moire_interference.fs"`, cache 7 uniform locs, init `rotationAccum = 0.0f`
   - Setup: Accumulate `rotationAccum += cfg->animationSpeed * deltaTime`, set all uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.7: Create radial_ifs module

**Files**: `src/effects/radial_ifs.h`, `src/effects/radial_ifs.cpp`

**Creates**: `RadialIfsConfig`, `RadialIfsEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `RadialIfsConfig` struct per spec
3. Define `RadialIfsEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load `"shaders/radial_ifs.fs"`, cache 7 uniform locs, init `rotation = 0.0f`, `twist = 0.0f`
   - Setup: Accumulate rotation and twist, set all uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

### Wave 2: Integration Updates

All integration tasks can run in parallel since they touch different files.

#### Task 2.1: Update CMakeLists.txt

**Files**: `CMakeLists.txt`

**Depends on**: Wave 1 complete

**Build**:
1. Add 7 new cpp files to `EFFECTS_SOURCES` (after existing entries):
   ```cmake
   src/effects/kaleidoscope.cpp
   src/effects/kifs.cpp
   src/effects/poincare_disk.cpp
   src/effects/mandelbox.cpp
   src/effects/triangle_fold.cpp
   src/effects/moire_interference.cpp
   src/effects/radial_ifs.cpp
   ```

**Verify**: `cmake.exe -G Ninja -B build -S .` succeeds.

---

#### Task 2.2: Update effect_config.h

**Files**: `src/config/effect_config.h`

**Depends on**: Wave 1 complete

**Build**:
1. Remove includes for migrated config headers:
   - `#include "kaleidoscope_config.h"`
   - `#include "kifs_config.h"`
   - `#include "poincare_disk_config.h"`
   - `#include "mandelbox_config.h"`
   - `#include "triangle_fold_config.h"`
   - `#include "moire_interference_config.h"`
   - `#include "radial_ifs_config.h"`
2. Add includes for new effect modules:
   - `#include "effects/kaleidoscope.h"`
   - `#include "effects/kifs.h"`
   - `#include "effects/poincare_disk.h"`
   - `#include "effects/mandelbox.h"`
   - `#include "effects/triangle_fold.h"`
   - `#include "effects/moire_interference.h"`
   - `#include "effects/radial_ifs.h"`

**Verify**: Header parses correctly.

---

#### Task 2.3: Update post_effect.h

**Files**: `src/render/post_effect.h`

**Depends on**: Wave 1 complete

**Build**:
1. Add includes for all 7 effect modules:
   ```cpp
   #include "effects/kaleidoscope.h"
   #include "effects/kifs.h"
   #include "effects/poincare_disk.h"
   #include "effects/mandelbox.h"
   #include "effects/triangle_fold.h"
   #include "effects/moire_interference.h"
   #include "effects/radial_ifs.h"
   ```
2. Remove shader fields:
   - `kaleidoShader`
   - `kifsShader`
   - `poincareDiskShader`
   - `mandelboxShader`
   - `triangleFoldShader`
   - `moireInterferenceShader`
   - `radialIfsShader`
3. Remove all uniform location fields for these 7 effects (~50 int fields):
   - `kaleidoSegmentsLoc`, `kaleidoRotationLoc`, `kaleidoTwistLoc`, `kaleidoSmoothingLoc`
   - `kifsRotationLoc`, `kifsTwistLoc`, `kifsIterationsLoc`, `kifsScaleLoc`, `kifsOffsetLoc`, `kifsOctantFoldLoc`, `kifsPolarFoldLoc`, `kifsPolarFoldSegmentsLoc`, `kifsPolarFoldSmoothingLoc`
   - `poincareDiskTilePLoc`, `poincareDiskTileQLoc`, `poincareDiskTileRLoc`, `poincareDiskTranslationLoc`, `poincareDiskRotationLoc`, `poincareDiskDiskScaleLoc`
   - `mandelboxIterationsLoc`, `mandelboxBoxLimitLoc`, `mandelboxSphereMinLoc`, `mandelboxSphereMaxLoc`, `mandelboxScaleLoc`, `mandelboxOffsetLoc`, `mandelboxRotationLoc`, `mandelboxTwistAngleLoc`, `mandelboxBoxIntensityLoc`, `mandelboxSphereIntensityLoc`, `mandelboxPolarFoldLoc`, `mandelboxPolarFoldSegmentsLoc`
   - `triangleFoldIterationsLoc`, `triangleFoldScaleLoc`, `triangleFoldOffsetLoc`, `triangleFoldRotationLoc`, `triangleFoldTwistAngleLoc`
   - `moireInterferenceRotationAngleLoc`, `moireInterferenceScaleDiffLoc`, `moireInterferenceLayersLoc`, `moireInterferenceBlendModeLoc`, `moireInterferenceCenterXLoc`, `moireInterferenceCenterYLoc`, `moireInterferenceRotationAccumLoc`
   - `radialIfsSegmentsLoc`, `radialIfsIterationsLoc`, `radialIfsScaleLoc`, `radialIfsOffsetLoc`, `radialIfsRotationLoc`, `radialIfsTwistAngleLoc`, `radialIfsSmoothingLoc`
4. Remove animation state fields:
   - `currentKaleidoRotation`
   - `currentKifsRotation`, `currentKifsTwist`
   - `poincareTime`, `currentPoincareTranslation[2]`, `currentPoincareRotation`
   - `currentMandelboxRotation`, `currentMandelboxTwist`
   - `currentTriangleFoldRotation`, `currentTriangleFoldTwist`
   - `moireInterferenceRotationAccum`
   - `currentRadialIfsRotation`, `currentRadialIfsTwist`
5. Add effect module fields (group with other effect modules):
   ```cpp
   KaleidoscopeEffect kaleidoscope;
   KifsEffect kifs;
   PoincareDiskEffect poincareDisk;
   MandelboxEffect mandelbox;
   TriangleFoldEffect triangleFold;
   MoireInterferenceEffect moireInterference;
   RadialIfsEffect radialIfs;
   ```

**Verify**: Header parses correctly.

---

#### Task 2.4: Update post_effect.cpp

**Files**: `src/render/post_effect.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. In `LoadPostEffectShaders`:
   - Remove 7 `LoadShader` calls for migrated effects
   - Remove corresponding entries from the return condition
2. In `GetShaderUniformLocations`:
   - Remove all `GetShaderLocation` calls for the 7 migrated effects (~50 lines)
3. In `PostEffectInit`:
   - After existing effect inits, add Init calls for all 7 effects:
     ```cpp
     if (!KaleidoscopeEffectInit(&pe->kaleidoscope)) { free(pe); return NULL; }
     if (!KifsEffectInit(&pe->kifs)) { free(pe); return NULL; }
     if (!PoincareDiskEffectInit(&pe->poincareDisk)) { free(pe); return NULL; }
     if (!MandelboxEffectInit(&pe->mandelbox)) { free(pe); return NULL; }
     if (!TriangleFoldEffectInit(&pe->triangleFold)) { free(pe); return NULL; }
     if (!MoireInterferenceEffectInit(&pe->moireInterference)) { free(pe); return NULL; }
     if (!RadialIfsEffectInit(&pe->radialIfs)) { free(pe); return NULL; }
     ```
   - **CRITICAL**: Use `free(pe); return NULL;` not `return false;`
4. Remove state initializations for migrated effects (rotation accumulators, etc.)
5. In `PostEffectUninit`:
   - Remove `UnloadShader` calls for migrated effects
   - Add Uninit calls for all 7 effects

**Verify**: Compiles.

---

#### Task 2.5: Update shader_setup.cpp

**Files**: `src/render/shader_setup.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. In `GetTransformEffect`, update shader pointer for each migrated effect:
   - `TRANSFORM_KALEIDOSCOPE`: `&pe->kaleidoscope.shader`
   - `TRANSFORM_KIFS`: `&pe->kifs.shader`
   - `TRANSFORM_POINCARE_DISK`: `&pe->poincareDisk.shader`
   - `TRANSFORM_MANDELBOX`: `&pe->mandelbox.shader`
   - `TRANSFORM_TRIANGLE_FOLD`: `&pe->triangleFold.shader`
   - `TRANSFORM_MOIRE_INTERFERENCE`: `&pe->moireInterference.shader`
   - `TRANSFORM_RADIAL_IFS`: `&pe->radialIfs.shader`

**Verify**: Compiles.

---

#### Task 2.6: Update shader_setup_symmetry.cpp

**Files**: `src/render/shader_setup_symmetry.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. Remove the include for `config/radial_ifs_config.h` (line 2)
2. Add includes for all 7 effect modules:
   ```cpp
   #include "effects/kaleidoscope.h"
   #include "effects/kifs.h"
   #include "effects/poincare_disk.h"
   #include "effects/mandelbox.h"
   #include "effects/triangle_fold.h"
   #include "effects/moire_interference.h"
   #include "effects/radial_ifs.h"
   ```
3. Replace each Setup function body with a single call to the effect's Setup function:
   ```cpp
   void SetupKaleido(PostEffect *pe) {
     KaleidoscopeEffectSetup(&pe->kaleidoscope, &pe->effects.kaleidoscope, pe->currentDeltaTime);
   }
   void SetupKifs(PostEffect *pe) {
     KifsEffectSetup(&pe->kifs, &pe->effects.kifs, pe->currentDeltaTime);
   }
   void SetupPoincareDisk(PostEffect *pe) {
     PoincareDiskEffectSetup(&pe->poincareDisk, &pe->effects.poincareDisk, pe->currentDeltaTime);
   }
   void SetupMandelbox(PostEffect *pe) {
     MandelboxEffectSetup(&pe->mandelbox, &pe->effects.mandelbox, pe->currentDeltaTime);
   }
   void SetupTriangleFold(PostEffect *pe) {
     TriangleFoldEffectSetup(&pe->triangleFold, &pe->effects.triangleFold, pe->currentDeltaTime);
   }
   void SetupMoireInterference(PostEffect *pe) {
     MoireInterferenceEffectSetup(&pe->moireInterference, &pe->effects.moireInterference, pe->currentDeltaTime);
   }
   void SetupRadialIfs(PostEffect *pe) {
     RadialIfsEffectSetup(&pe->radialIfs, &pe->effects.radialIfs, pe->currentDeltaTime);
   }
   ```

**Verify**: Compiles.

---

#### Task 2.7: Update render_pipeline.cpp

**Files**: `src/render/render_pipeline.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. Remove time accumulator lines for migrated effects (around lines 331-354):
   - `pe->currentKaleidoRotation += ...`
   - `pe->currentKifsRotation += ...`
   - `pe->currentKifsTwist += ...`
   - `pe->currentMandelboxRotation += ...`
   - `pe->currentMandelboxTwist += ...`
   - `pe->currentTriangleFoldRotation += ...`
   - `pe->currentTriangleFoldTwist += ...`
   - `pe->currentRadialIfsRotation += ...`
   - `pe->currentRadialIfsTwist += ...`
   - `pe->currentPoincareRotation += ...`
   - `pe->poincareTime += ...`
   - `pe->currentPoincareTranslation[0] = ...`
   - `pe->currentPoincareTranslation[1] = ...`
2. Keep other unrelated accumulator lines (latticeFold, halftone, phyllotaxis, etc.)

**Verify**: Compiles.

---

#### Task 2.8: Update imgui_effects_symmetry.cpp

**Files**: `src/ui/imgui_effects_symmetry.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. Remove any stale includes for migrated config headers (grep for `#include.*_config.h` patterns matching the migrated effects)
2. No field renames in this batch—config field names are unchanged

**Verify**: Compiles.

---

#### Task 2.9: Update preset.cpp

**Files**: `src/config/preset.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. Remove any stale includes for migrated config headers (grep for `#include.*_config.h` patterns matching the migrated effects)
2. No NLOHMANN macro changes needed—config field names are unchanged

**Verify**: Compiles.

---

#### Task 2.10: Delete old config headers

**Files**: 7 config header files

**Depends on**: Tasks 2.2, 2.8, 2.9 complete

**Build**:
1. Delete the following files:
   - `src/config/kaleidoscope_config.h`
   - `src/config/kifs_config.h`
   - `src/config/poincare_disk_config.h`
   - `src/config/mandelbox_config.h`
   - `src/config/triangle_fold_config.h`
   - `src/config/moire_interference_config.h`
   - `src/config/radial_ifs_config.h`

**Verify**: Files no longer exist, build still succeeds.

---

### Wave 3: Validation

#### Task 3.1: Build and runtime test

**Files**: None (validation only)

**Depends on**: All Wave 2 tasks complete

**Build**:
1. `cmake.exe --build build`
2. Launch application
3. Test each of the 7 migrated effects:
   - Enable effect
   - Verify animation (rotation/twist for effects with speed params)
   - Verify sliders update visual
   - Disable effect
4. Load a preset that uses symmetry effects
5. Verify preset loads correctly

**Verify**: No crashes, effects render identically to before migration.

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S .` succeeds
- [ ] `cmake.exe --build build` succeeds with no warnings in new files
- [ ] 7 config headers deleted from `src/config/`
- [ ] 7 effect modules exist in `src/effects/`
- [ ] All 7 symmetry effects render correctly at runtime
- [ ] Presets containing symmetry effects load correctly
- [ ] No stale `pe->current*` fields referenced in any file
