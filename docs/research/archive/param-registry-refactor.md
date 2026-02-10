# Param Registry Refactor

Move parameter registration from a monolithic table into individual effect modules.

## Classification

- **Type**: General (software architecture)
- **Chosen Approach**: Per-module registration with modulation engine as source of truth

## Problem

`param_registry.cpp` contains a 1000-line `PARAM_TABLE` with 369 entries. Adding a modulatable field to an effect requires editing both the effect module AND this central table. This violates the "one file per effect" goal from the effect-modules refactor.

56 effect modules exist in `src/effects/`. Their 210 param entries should move into per-module `RegisterParams` functions. The remaining 159 entries (simulations, feedback, flow, top-level effects) stay in PARAM_TABLE.

## Current Flow

```
Startup:
  ParamRegistryInit(effects)
  → loops PARAM_TABLE (369 entries)
  → calls ModEngineRegisterParam(id, ptr, min, max) for each
  → ModEngine stores {ptr, min, max} in sParams hashmap

Bounds Lookup (UI sliders, preset validation):
  ParamRegistryGet(paramId)
  → linear search of PARAM_TABLE
  → returns {min, max}

  NOTE: ModEngine already stores min/max but exposes no getter
```

## Solution

### Add bounds getter to ModEngine

Declare `ModEngineGetParamBounds(paramId, &min, &max)` in `modulation_engine.h`. Implement as a hashmap lookup in `modulation_engine.cpp` — returns the min/max already stored during registration.

### Effect modules register their own params

Each effect module declares `<Name>RegisterParams(<Name>Config* cfg)` in its header and implements it in its `.cpp`. The function calls `ModEngineRegisterParam` for each modulatable field with the param ID string, pointer, and bounds.

Effects with zero modulatable params (toon, radial_streak) get an empty `RegisterParams` for consistency.

### ParamRegistryGetDynamic queries ModEngine

Replace `ParamRegistryGetDynamic`'s static table lookup with a `ModEngineGetParamBounds` call first. Falls through to DRAWABLE_FIELD_TABLE and LFO rate patterns as before.

`ParamRegistryGet` (static lookup) can be deleted once all callers switch to `ParamRegistryGetDynamic` or query ModEngine directly.

### Call registration from main.cpp

After `PostEffectInit` (which initializes effect modules), call each module's `RegisterParams` with a pointer to its config in `EffectConfig`. This replaces the portion of `ParamRegistryInit` that registers effect params.

### Shrink PARAM_TABLE

Remove migrated effect entries from PARAM_TABLE. Keep simulation, feedback, flow, and top-level entries (159 params) until those systems become modules.

## Before vs After

**Before** (adding modulatable field):
1. Edit `sine_warp.h` — add field to config struct
2. Edit `param_registry.cpp` — add entry to PARAM_TABLE

**After**:
1. Edit `sine_warp.h` — add field to config struct
2. Edit `sine_warp.cpp` — add `ModEngineRegisterParam` call in `SineWarpRegisterParams`

Everything stays in one file per effect.

## File Changes Summary

| File | Change |
|------|--------|
| `modulation_engine.h` | Add `ModEngineGetParamBounds()` declaration |
| `modulation_engine.cpp` | Implement bounds getter (~5 lines) |
| `param_registry.cpp` | Update `ParamRegistryGetDynamic` to query ModEngine first; remove migrated entries from PARAM_TABLE |
| `src/effects/*.h` | Add `RegisterParams()` declaration to each |
| `src/effects/*.cpp` | Add `RegisterParams()` implementation to each |
| `src/main.cpp` | Call `RegisterParams()` for each effect module after init |

## What Stays The Same

- ModEngine internal storage (already has min/max)
- Modulation routing logic
- Preset serialization
- UI slider code
- DRAWABLE_FIELD_TABLE (drawables are dynamic, not modules)
- Simulation/feedback/flow entries in PARAM_TABLE (until those systems become modules)

---

*To plan implementation: `/feature-plan docs/research/param-registry-refactor.md`*
