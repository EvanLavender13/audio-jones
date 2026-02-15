# Random Walk Trail

Add a random walk motion mode to the ParametricTrail drawable. A hash-driven cursor wanders unpredictably, leaving shapes on the feedback texture. Smoothness controls motion character from jerky staccato snaps to smooth floating drift. Complements the existing deterministic Lissajous motion with organic, unpredictable movement.

**Research**: `docs/research/random-walk-trail.md`

## Design

### Types

**New file `src/config/random_walk_config.h`:**

```
typedef enum {
  WALK_BOUNDARY_CLAMP = 0,
  WALK_BOUNDARY_WRAP = 1,
  WALK_BOUNDARY_DRIFT = 2,
} WalkBoundaryMode;

struct RandomWalkConfig {
  // Modulatable
  float stepSize = 0.02f;       // Distance per discrete step (0.001-0.1)
  float smoothness = 0.5f;      // 0=jerky snaps, 1=smooth glide (0.0-1.0)

  // Non-modulatable (cause discontinuities)
  float tickRate = 20.0f;       // Discrete steps per second (1.0-60.0)
  WalkBoundaryMode boundaryMode = WALK_BOUNDARY_DRIFT;
  float driftStrength = 0.3f;   // Pull toward center in Drift mode (0.0-2.0)
  int seed = 0;                 // 0 = auto from drawableId
};

struct RandomWalkState {
  float walkX = 0.0f;           // Current discrete position (offset from base, clamped ±0.48)
  float walkY = 0.0f;
  float prevX = 0.0f;           // Previous discrete position (for interpolation)
  float prevY = 0.0f;
  float timeAccum = 0.0f;       // Fractional time within current tick
  uint32_t tickCounter = 0;     // Step counter for hash input
};
```

**New enum in `drawable_config.h`:**

```
typedef enum {
  TRAIL_MOTION_LISSAJOUS = 0,
  TRAIL_MOTION_RANDOM_WALK = 1,
} TrailMotionType;
```

**Modified `ParametricTrailData`** — add fields:

```
TrailMotionType motionType = TRAIL_MOTION_LISSAJOUS;
RandomWalkConfig randomWalk;
RandomWalkState walkState;  // Runtime, not serialized
```

### Algorithm

**Hash function** — splitmix-style integer hash (in `random_walk_config.h`, inline):

```
uint32_t hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x45d9f3b;
    x ^= x >> 16;
    x *= 0x45d9f3b;
    x ^= x >> 16;
    return x;
}
```

Convert to float in [0, 1]: `(float)(hash(seed) & 0xFFFFFF) / (float)0xFFFFFF`

Two calls per step: `hash(baseSeed + tickCounter * 2)` for dx, `hash(baseSeed + tickCounter * 2 + 1)` for dy. When `seed == 0`, `baseSeed = hash(drawableId)`.

**`RandomWalkUpdate()` function** (inline in `random_walk_config.h`, mirrors `DualLissajousUpdate()` pattern):

1. Accumulate time: `state->timeAccum += deltaTime * cfg->tickRate`
2. While `state->timeAccum >= 1.0f`: advance one discrete step
   - Hash `baseSeed + tickCounter * 2` and `baseSeed + tickCounter * 2 + 1` → floats in [0,1]
   - Map to direction: `dx = (hashX - 0.5f) * 2.0f * cfg->stepSize`, same for dy
   - Store `prevX/prevY = walkX/walkY`
   - Add `dx/dy` to `walkX/walkY`
   - Apply boundary:
     - **Clamp**: clamp walkX/Y to [-0.48, 0.48]
     - **Wrap**: fmodf wrap to [-0.48, 0.48] range
     - **Drift**: `walkX = lerp(walkX, 0.0f, cfg->driftStrength * (1.0f / cfg->tickRate))`
   - Increment `tickCounter`, subtract 1.0f from `timeAccum`
3. Compute interpolated output:
   - `frac = state->timeAccum` (fractional time within tick, 0-1)
   - `smoothPos = lerp(prevPos, walkPos, frac)`
   - `outputPos = lerp(walkPos, smoothPos, cfg->smoothness)`
   - Write to `outX/outY`

Signature: `void RandomWalkUpdate(RandomWalkConfig *cfg, RandomWalkState *state, uint32_t drawableId, float deltaTime, float *outX, float *outY)`

**`RandomWalkReset()`** — zeroes all `RandomWalkState` fields. Called on motion mode switch and preset load.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| motionType | enum | Lissajous / Random Walk | Lissajous | No | "Motion" |
| stepSize | float | 0.001–0.1 | 0.02 | Yes | "Step Size" |
| smoothness | float | 0.0–1.0 | 0.5 | Yes | "Smoothness" |
| tickRate | float | 1.0–60.0 | 20.0 | No | "Tick Rate" |
| boundaryMode | enum | Clamp / Wrap / Drift | Drift | No | "Boundary" |
| driftStrength | float | 0.0–2.0 | 0.3 | No | "Drift" |
| seed | int | 0–9999 | 0 | No | "Seed" |

### Constants

- Enum values: `TRAIL_MOTION_LISSAJOUS = 0`, `TRAIL_MOTION_RANDOM_WALK = 1`
- Boundary enum: `WALK_BOUNDARY_CLAMP = 0`, `WALK_BOUNDARY_WRAP = 1`, `WALK_BOUNDARY_DRIFT = 2`

---

## Tasks

### Wave 1: Config Foundation

#### Task 1.1: Create RandomWalkConfig header

**Files**: `src/config/random_walk_config.h` (create)
**Creates**: `RandomWalkConfig`, `RandomWalkState`, `WalkBoundaryMode` enum, `RandomWalkUpdate()`, `RandomWalkReset()`, hash function

**Do**: Create header with include guard. Define `WalkBoundaryMode` enum, `RandomWalkConfig` struct, `RandomWalkState` struct, inline hash function, inline `RandomWalkUpdate()` and `RandomWalkReset()`. Follow the `dual_lissajous_config.h` pattern — all inline functions, no .cpp file. Use the types, defaults, and algorithm from the Design section above.

**Verify**: `cmake.exe --build build` compiles (header-only, no includes yet).

#### Task 1.2: Add motion type enum and walk fields to ParametricTrailData

**Files**: `src/config/drawable_config.h` (modify)
**Creates**: `TrailMotionType` enum, new fields in `ParametricTrailData`
**Depends on**: Task 1.1 complete

**Do**:
1. Add `#include "config/random_walk_config.h"` at top
2. Add `TrailMotionType` enum (Lissajous=0, RandomWalk=1) near the other trail enums
3. Add to `ParametricTrailData`: `TrailMotionType motionType = TRAIL_MOTION_LISSAJOUS;`, `RandomWalkConfig randomWalk;`, `RandomWalkState walkState;`
4. The `walkState` field is runtime-only (not serialized — handled by preset.cpp only serializing `RandomWalkConfig`)

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Update drawable rendering

**Files**: `src/render/drawable.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: In `DrawableRenderParametricTrail()`, dispatch on `trail.motionType`:
- `TRAIL_MOTION_LISSAJOUS`: existing `DualLissajousUpdate()` call (unchanged)
- `TRAIL_MOTION_RANDOM_WALK`: call `RandomWalkUpdate(&trail.randomWalk, &trail.walkState, d->id, deltaTime, &offsetX, &offsetY)`

The rest of the function (gate check, shape drawing) stays the same for both modes.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Update preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `RandomWalkConfig` — serialize: `stepSize`, `smoothness`, `tickRate`, `boundaryMode`, `driftStrength`, `seed`. Do NOT serialize `RandomWalkState`.
2. Update the existing `ParametricTrailData` serialization macro to include `motionType` and `randomWalk` fields.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Update UI controls

**Files**: `src/ui/drawable_type_controls.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Modify `DrawParametricTrailControls()`:
1. Add a `static bool sectionRandomWalk = true;` toggle at file scope
2. At the top of the function (before the Path section), add a Motion combo box:
   - Labels: `{"Lissajous", "Random Walk"}`
   - When motion type changes, call `RandomWalkReset(&d->parametricTrail.walkState)` and reset `d->parametricTrail.lissajous.phase = 0.0f`
3. Show the existing Path section only when `motionType == TRAIL_MOTION_LISSAJOUS`
4. Show a new "Random Walk" section (using `sectionRandomWalk` toggle, `Theme::GLOW_CYAN`) only when `motionType == TRAIL_MOTION_RANDOM_WALK`:
   - `ModulatableDrawableSlider("Step Size", &trail.randomWalk.stepSize, ...)` with `"%.3f"` format
   - `ModulatableDrawableSlider("Smoothness", &trail.randomWalk.smoothness, ...)` with `"%.2f"` format
   - `ImGui::SliderFloat("Tick Rate", &trail.randomWalk.tickRate, 1.0f, 60.0f, "%.0f /s")`
   - Boundary combo: labels `{"Clamp", "Wrap", "Drift"}`, cast to/from `WalkBoundaryMode`
   - Show drift strength slider only when `boundaryMode == WALK_BOUNDARY_DRIFT`: `ImGui::SliderFloat("Drift", &trail.randomWalk.driftStrength, 0.0f, 2.0f, "%.2f")`
   - `ImGui::SliderInt("Seed", &trail.randomWalk.seed, 0, 9999)`
5. Include `config/random_walk_config.h`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Update modulation param registration

**Files**: `src/automation/drawable_params.cpp` (modify), `src/automation/param_registry.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. In `drawable_params.cpp`, inside the `DRAWABLE_PARAMETRIC_TRAIL` block, add registration for random walk modulatable params:
   - `"drawable.<id>.randomWalk.stepSize"` → `&d->parametricTrail.randomWalk.stepSize`, range 0.001–0.1
   - `"drawable.<id>.randomWalk.smoothness"` → `&d->parametricTrail.randomWalk.smoothness`, range 0.0–1.0
2. In `param_registry.cpp`, add entries to `DRAWABLE_FIELD_TABLE`:
   - `{"randomWalk.stepSize", {0.001f, 0.1f}, 0}`
   - `{"randomWalk.smoothness", {0.0f, 1.0f}, 0}`

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Lissajous mode still works identically (no regression)
- [ ] Random Walk mode produces visible wandering shapes on screen
- [ ] Motion combo box switches between modes, resets walk state
- [ ] Boundary modes: Clamp stops at edges, Wrap teleports across, Drift pulls toward center
- [ ] Smoothness slider: 0 = jerky snaps, 1 = smooth interpolated glide
- [ ] Seed = 0 auto-derives from drawableId (two trails with seed=0 walk differently)
- [ ] Preset save/load round-trips all random walk parameters
- [ ] Modulation works on stepSize and smoothness
