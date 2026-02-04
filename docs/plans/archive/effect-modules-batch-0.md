# Effect Modules Migration - Batch 0 (Pilot)

Migrate sine_warp from scattered PostEffect fields into a self-contained module at `src/effects/sine_warp.h`. Validates the modularization pattern before mass migration.

## Specification

### SineWarpConfig (user-facing parameters)

```cpp
// src/effects/sine_warp.h
struct SineWarpConfig {
  bool enabled = false;
  int octaves = 4;             // Number of cascade octaves (1-8)
  float strength = 0.5f;       // Distortion intensity (0.0-2.0)
  float speed = 0.3f;          // Animation rate (radians/second) - RENAMED from animRate
  float octaveRotation = 0.5f; // Rotation per octave in radians (±π)
  bool radialMode = false;     // false=Cartesian warp, true=Polar warp
  bool depthBlend = true;      // true=sample each octave, false=sample once
};
```

**Field rename**: `animRate` → `speed`. Task 2.10 migrates existing presets.

### SineWarpEffect (runtime state)

```cpp
// src/effects/sine_warp.h
typedef struct SineWarpEffect {
  Shader shader;
  int timeLoc;
  int rotationLoc;
  int octavesLoc;
  int strengthLoc;
  int octaveRotationLoc;
  int radialModeLoc;
  int depthBlendLoc;
  float time;  // Animation accumulator
} SineWarpEffect;
```

### Lifecycle Functions

```cpp
// Returns true on success, false if shader fails to load
bool SineWarpEffectInit(SineWarpEffect* e);

// Accumulates time, sets all uniforms
void SineWarpEffectSetup(SineWarpEffect* e, const SineWarpConfig* cfg, float deltaTime);

// Unloads shader
void SineWarpEffectUninit(SineWarpEffect* e);

// Returns default config (matches current sine_warp_config.h defaults)
SineWarpConfig SineWarpConfigDefault(void);
```

### Integration Points

| Location | Before | After |
|----------|--------|-------|
| `PostEffect` struct | `Shader sineWarpShader` + 7 uniform locs + `float sineWarpTime` | `SineWarpEffect sineWarp` |
| `PostEffectInit` | `LoadShader()` + 7x `GetShaderLocation()` | `SineWarpEffectInit(&pe->sineWarp)` |
| `PostEffectUninit` | `UnloadShader(pe->sineWarpShader)` | `SineWarpEffectUninit(&pe->sineWarp)` |
| `GetTransformEffect` | Returns `&pe->sineWarpShader` | Returns `&pe->sineWarp.shader` |
| `SetupSineWarp` | Reads from `pe->` fields | Calls `SineWarpEffectSetup()` |
| Config location | `src/config/sine_warp_config.h` | `src/effects/sine_warp.h` |

---

## Tasks

### Wave 1: Effect Module

#### Task 1.1: Create sine_warp effect module

**Files**: `src/effects/sine_warp.h`, `src/effects/sine_warp.cpp`

**Creates**: `SineWarpConfig`, `SineWarpEffect`, lifecycle functions

**Build**:
1. Create `src/effects/` directory
2. Create `src/effects/sine_warp.h`:
   - Include guards `#ifndef SINE_WARP_EFFECT_H`
   - `#include "raylib.h"` and `#include <stdbool.h>`
   - Define `SineWarpConfig` struct per spec (note: `speed` not `animRate`)
   - Define `SineWarpEffect` struct per spec
   - Declare `SineWarpEffectInit`, `SineWarpEffectSetup`, `SineWarpEffectUninit`, `SineWarpConfigDefault`
3. Create `src/effects/sine_warp.cpp`:
   - `#include "sine_warp.h"`
   - Implement `SineWarpEffectInit`: load shader from `"shaders/sine_warp.fs"`, cache all 7 uniform locations, init `time = 0.0f`, return `shader.id != 0`
   - Implement `SineWarpEffectSetup`: accumulate `e->time += cfg->speed * deltaTime`, set all uniforms (copy logic from current `SetupSineWarp` in `shader_setup_warp.cpp`)
   - Implement `SineWarpEffectUninit`: `UnloadShader(e->shader)`
   - Implement `SineWarpConfigDefault`: return struct with current defaults (enabled=false, octaves=4, strength=0.5f, speed=0.3f, octaveRotation=0.5f, radialMode=false, depthBlend=true)

**Verify**: Files exist with correct content.

---

### Wave 2: Integration (parallel tasks, no file overlap)

#### Task 2.1: Update CMakeLists.txt

**Files**: `CMakeLists.txt`

**Depends on**: Wave 1 complete

**Build**:
1. Add new source group after existing groups:
   ```cmake
   set(EFFECTS_SOURCES
       src/effects/sine_warp.cpp
   )
   ```
2. Add `${EFFECTS_SOURCES}` to the `add_executable` target sources

**Verify**: `cmake.exe -G Ninja -B build -S .` succeeds.

#### Task 2.2: Update effect_config.h

**Files**: `src/config/effect_config.h`

**Depends on**: Wave 1 complete

**Build**:
1. Replace `#include "sine_warp_config.h"` with `#include "effects/sine_warp.h"`
2. No other changes needed - `SineWarpConfig sineWarp` field type unchanged

**Verify**: Header parses correctly.

#### Task 2.3: Update post_effect.h

**Files**: `src/render/post_effect.h`

**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "effects/sine_warp.h"` after existing includes
2. Remove: `Shader sineWarpShader;`
3. Remove: `int sineWarpTimeLoc;`, `int sineWarpRotationLoc;`, `int sineWarpOctavesLoc;`, `int sineWarpStrengthLoc;`, `int sineWarpOctaveRotationLoc;`, `int sineWarpRadialModeLoc;`, `int sineWarpDepthBlendLoc;`
4. Remove: `float sineWarpTime;`
5. Add after simulation pointers: `SineWarpEffect sineWarp;`

**Verify**: Header parses correctly.

#### Task 2.4: Update post_effect.cpp

**Files**: `src/render/post_effect.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. In `LoadPostEffectShaders`:
   - Remove: `pe->sineWarpShader = LoadShader(0, "shaders/sine_warp.fs");`
   - Remove `pe->sineWarpShader.id != 0` from the return condition
2. In `GetShaderUniformLocations`:
   - Remove all 7 `pe->sineWarp*Loc = GetShaderLocation(...)` lines
3. In `PostEffectInit` (after other init calls):
   - Add: `if (!SineWarpEffectInit(&pe->sineWarp)) { /* handle error or log */ }`
4. In state initialization section:
   - Remove: `pe->sineWarpTime = 0.0f;`
5. In `PostEffectUninit`:
   - Remove: `UnloadShader(pe->sineWarpShader);`
   - Add: `SineWarpEffectUninit(&pe->sineWarp);`

**Verify**: Compiles without errors.

#### Task 2.5: Update shader_setup.cpp

**Files**: `src/render/shader_setup.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. In `GetTransformEffect`, case `TRANSFORM_SINE_WARP`:
   - Change `&pe->sineWarpShader` to `&pe->sineWarp.shader`
   - Keep `SetupSineWarp` and `&pe->effects.sineWarp.enabled` unchanged

**Verify**: Compiles without errors.

#### Task 2.6: Update shader_setup_warp.cpp

**Files**: `src/render/shader_setup_warp.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "effects/sine_warp.h"` at top
2. Replace entire `SetupSineWarp` function body:
   ```cpp
   void SetupSineWarp(PostEffect *pe) {
     SineWarpEffectSetup(&pe->sineWarp, &pe->effects.sineWarp, pe->currentDeltaTime);
   }
   ```

**Verify**: Compiles without errors.

#### Task 2.7: Update preset.cpp

**Files**: `src/config/preset.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. Find NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE for SineWarpConfig
2. Change `animRate` to `speed` in the macro field list

**Verify**: Compiles without errors.

#### Task 2.8: Update imgui_effects_warp.cpp

**Files**: `src/ui/imgui_effects_warp.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. In `DrawWarpSine` function:
   - Change `&e->sineWarp.animRate` to `&e->sineWarp.speed`
   - Update slider label from `"Anim Rate##sineWarp"` to `"Speed##sineWarp"`

**Verify**: Compiles without errors.

#### Task 2.9: Delete old config header

**Files**: `src/config/sine_warp_config.h`

**Depends on**: Task 2.2 complete (effect_config.h updated)

**Build**:
1. Delete `src/config/sine_warp_config.h`

**Verify**: File no longer exists.

#### Task 2.10: Migrate presets

**Files**: `presets/*.json`

**Depends on**: Wave 1 complete

**Build**:
1. Search all JSON files in `presets/` for `"animRate"`
2. For each file containing `sineWarp.animRate`:
   - Rename the key from `"animRate"` to `"speed"`
   - Keep the value unchanged

**Verify**: `grep -r "animRate" presets/` returns no matches.

---

### Wave 3: Validation

#### Task 3.1: Full build and runtime test

**Files**: None (validation only)

**Depends on**: All Wave 2 tasks complete

**Build**:
1. Run `cmake.exe --build build`
2. Launch application
3. Enable Sine Warp effect
4. Verify animation plays correctly
5. Verify all sliders work
6. Test radial mode toggle
7. Test depth blend toggle

**Verify**: No crashes, effect renders identically to before migration.

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S .` succeeds
- [ ] `cmake.exe --build build` succeeds with no warnings in new files
- [ ] `src/config/sine_warp_config.h` deleted
- [ ] `src/effects/sine_warp.h` and `src/effects/sine_warp.cpp` exist
- [ ] Sine Warp effect renders correctly at runtime
- [ ] No presets contain `animRate` (all migrated to `speed`)
- [ ] Presets with `sineWarp.speed` load correctly
