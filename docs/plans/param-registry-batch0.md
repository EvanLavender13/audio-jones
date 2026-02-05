# Param Registry Migration — Batch 0: Infrastructure

Add the plumbing so future batches can move param registration from PARAM_TABLE into per-module RegisterParams functions.

**Research**: [docs/research/param-registry-refactor.md](../research/param-registry-refactor.md)

## Design

### ModEngineGetParamBounds

New function in modulation_engine. Looks up `sParams` hashmap (already stores min/max from registration). Returns `bool` — false if paramId not found.

```
bool ModEngineGetParamBounds(const char *paramId, float *outMin, float *outMax);
```

### ParamRegistryGetDynamic Update

Current lookup order: `ParamRegistryGet` (PARAM_TABLE linear scan) → drawable field match → LFO rate match.

New lookup order: `ModEngineGetParamBounds` (O(1) hashmap) → drawable field match → LFO rate match.

This works because every param registered via PARAM_TABLE is already in ModEngine's hashmap. Once effect modules register directly via ModEngine, their bounds are found the same way — no PARAM_TABLE entry needed.

`ParamRegistryGet` (static PARAM_TABLE lookup) becomes dead code after this change. Remove it and its declaration.

### PostEffectRegisterParams

New function in `post_effect.h`/`post_effect.cpp`. Batch 0 adds an empty body. Future batches add `<Effect>RegisterParams(&pe->effects.<field>)` calls here as they migrate effects.

Called from main.cpp after `ParamRegistryInit` (which still registers the PARAM_TABLE entries).

```
void PostEffectRegisterParams(PostEffect *pe);
```

---

## Tasks

### Wave 1: ModEngine bounds getter

#### Task 1.1: Add ModEngineGetParamBounds

**Files**: `src/automation/modulation_engine.h`, `src/automation/modulation_engine.cpp`

**Do**: Add declaration to header. Implement in .cpp: look up `sParams` by paramId, write min/max to output pointers, return true. Return false if not found. Follow the pattern of `ModEngineGetBase` for the lookup.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Registry update + call site

#### Task 2.1: Update ParamRegistryGetDynamic

**Files**: `src/automation/param_registry.h`, `src/automation/param_registry.cpp`

**Depends on**: Wave 1 (uses ModEngineGetParamBounds)

**Do**:
- Replace the `ParamRegistryGet` call inside `ParamRegistryGetDynamic` with `ModEngineGetParamBounds`. If it returns true, populate `outDef` from the returned min/max and return true. Fall through to drawable/LFO patterns as before.
- Delete `ParamRegistryGet` function and its declaration from the header.

**Verify**: `cmake.exe --build build` compiles with no warnings about unused functions.

#### Task 2.2: Add PostEffectRegisterParams stub

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`, `src/main.cpp`

**Depends on**: Wave 1

**Do**:
- Add `void PostEffectRegisterParams(PostEffect *pe);` declaration to `post_effect.h`
- Add empty implementation in `post_effect.cpp`
- Call `PostEffectRegisterParams(ctx->postEffect)` in main.cpp after `ParamRegistryInit`

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Param count unchanged — all PARAM_TABLE entries still register via ParamRegistryInit, ModEngine hashmap still has same entries
- [ ] ParamRegistryGetDynamic returns correct bounds for an effect param (e.g., "sineWarp.strength") via ModEngine lookup instead of PARAM_TABLE scan
- [ ] ParamRegistryGet removed — no callers remain
