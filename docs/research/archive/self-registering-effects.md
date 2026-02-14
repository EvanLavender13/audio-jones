# Self-Registering Effect Modules

Each effect module declares its own lifecycle via a macro in its .cpp file. PostEffect discovers all registered effects at startup and loops over them — no central init/uninit/resize/register lists. Adding a new effect means writing the .cpp/.h and placing one macro call.

## Classification

- **Category**: General / Architecture
- **Scope**: PostEffect struct, effect lifecycle, shader_setup dispatch

## References

- [C++ patterns: static registration](https://dxuuu.xyz/cpp-static-registration.html) — Core pattern: static variable whose constructor calls a global registry
- [Factory With Self-Registering Types](https://www.cppstories.com/2018/02/factory-selfregister/) — Macro-based self-registration with lazy-initialized singleton registry
- [Self-Registration Factory Pattern in C++](https://tsaihao.github.io/blog/Self-Registration-Factory-Pattern-in-Cpp/) — Practical adaptation for game-engine-style systems

## Reference Code

The fundamental mechanism (from dxuuu.xyz, adapted):

```cpp
// Registry: lazy-initialized function-local static avoids init-order fiasco
template <typename T>
class Registry {
public:
    static bool add(const T& entry) {
        entries().push_back(entry);
        return true;
    }
    static const std::vector<T>& all() { return entries(); }
private:
    static std::vector<T>& entries() {
        static std::vector<T> v;
        return v;
    }
};

// Registration macro: static bool triggers add() before main()
#define REGISTER_PLUGIN(name, create_func) \
    static bool name##_entry = Registry<Entry>::add({#name, create_func})
```

Key property: each translation unit (.cpp) registers itself independently. No central list to maintain.

## Current Problem

PostEffect has 73 named `XxxEffect` fields and 4 hand-maintained lists that must stay in sync:

| Function | Lines | What it does |
|----------|-------|-------------|
| `PostEffectInit` | 184-509 | 73 sequential `if (!XxxEffectInit()) goto cleanup` |
| `PostEffectUninit` | 621-725 | 73 `XxxEffectUninit()` calls (different order than init) |
| `PostEffectRegisterParams` | 511-619 | 73 `XxxRegisterParams()` calls |
| `PostEffectResize` | 727-767 | ~10 `XxxEffectResize()` calls |

Adding a new effect requires edits in all 4 functions plus a struct field in the header.

## Algorithm

### Effect Entry Struct

Each registered effect provides a uniform lifecycle interface via function pointers:

```
EffectEntry {
    init(void* effect, void* config, int w, int h) -> bool
    uninit(void* effect)
    resize(void* effect, int w, int h)     // NULL when not needed
    registerParams(void* config)
    getShader(void* effect) -> Shader*     // primary shader for dispatch
    setup(void* effect, void* config, float dt, int w, int h)  // per-frame uniform binding
    effectOffset   // offsetof(PostEffect, field) — where in PostEffect this lives
    configOffset   // offsetof(EffectConfig, field) — where config lives
}
```

### Registration Macro

Goes at bottom of each effect .cpp. Exploits the codebase naming convention (`XxxEffect`, `XxxConfig`, `XxxEffectInit`, etc.) via token pasting:

```
REGISTER_EFFECT(Name, field)
```

Expands to:
- Static wrapper functions that cast `void*` to the correct types and forward to `NameEffectInit`, `NameEffectUninit`, `NameEffectSetup`, `NameRegisterParams`
- A static `EffectEntry` struct with offsets and function pointers
- A static `bool` whose initializer calls `EffectRegistryAdd(&entry)` before `main()`

Variants for effects with non-standard init signatures:
- `REGISTER_EFFECT(Name, field)` — simple init: `Init(Effect*)`
- `REGISTER_EFFECT_SIZED(Name, field)` — needs dimensions: `Init(Effect*, w, h)`
- `REGISTER_EFFECT_CFG(Name, field)` — needs config: `Init(Effect*, Config*)`
- `REGISTER_EFFECT_SIZED_CFG(Name, field)` — needs both: `Init(Effect*, Config*, w, h)`

Optional flag for resize: `REGISTER_EFFECT_RESIZABLE(Name, field)` or an `_R` suffix variant.

### PostEffect Changes

**Init** becomes a loop:
```
for each entry in registry:
    effect = (char*)pe + entry.effectOffset
    config = (char*)&pe->effects + entry.configOffset
    if (!entry.init(effect, config, w, h)) goto cleanup
```

**Uninit**, **Resize**, **RegisterParams** become similar loops.

### Shader Dispatch

`GetTransformEffect()` currently has an 80-case switch. Each effect could also register its dispatch info (primary shader pointer, setup function, enabled flag pointer). The switch becomes a registry lookup.

### Access from shader_setup Files

Currently: `pe->sineWarp.shader`, `pe->effects.sineWarp`

With registry, effect state moves to a `void* effects[]` array on PostEffect (or stays as named fields during migration). A typed accessor provides safe access:

```
// Accessor that casts from the void* array
SineWarpEffect* e = EffectGet<SineWarpEffect>(pe, idx);
```

## Key Design Decision: Named Fields vs Opaque Array

Two migration strategies:

**Option A — Keep named fields, eliminate ceremony:**
- PostEffect struct still has `SineWarpEffect sineWarp;` etc.
- The macro uses `offsetof(PostEffect, sineWarp)` to find the field
- Init/uninit/resize/register loops replace hand-coded lists
- shader_setup files unchanged (`pe->sineWarp.shader` still works)
- Drawback: adding an effect still requires adding a struct field + include to post_effect.h

**Option B — Replace named fields with void* array:**
- PostEffect has `void* effects[TRANSFORM_EFFECT_COUNT]` (heap-allocated per effect)
- The macro records `sizeof(XxxEffect)` so the registry can allocate
- shader_setup files use typed accessor: `EffectGet<SineWarpEffect>(pe, TRANSFORM_SINE_WARP)`
- Drawback: bigger migration, loses IDE autocomplete on `pe->effectName`

Recommendation: **Option A first** (eliminates 90% of the pain with minimal churn), Option B as a future follow-up if needed.

## What This Eliminates vs What Remains

**Eliminated by self-registration:**
- All 73 init calls in PostEffectInit → one loop
- All 73 uninit calls in PostEffectUninit → one loop
- All 73 register calls in PostEffectRegisterParams → one loop
- All resize calls in PostEffectResize → one loop (NULL-checked)
- The 80-case GetTransformEffect switch → registry lookup
- LoadPostEffectShaders helper → absorbed into per-effect init

**Still present (Option A):**
- 73 named struct fields in PostEffect (one line each)
- 69 #include lines in post_effect.h
- Config fields in EffectConfig (unchanged — serialization/UI depend on names)

**Still present (Option B):**
- Config fields in EffectConfig (unchanged)
- EffectConfig is a separate concern — it's user-facing data, not runtime state

## Risks

**Static init order**: Not a concern — the registry uses a function-local static (lazy init), and entries are independent data with no cross-dependencies.

**Linker stripping**: Static libraries can strip unreferenced translation units, dropping registrations. AudioJones links object files directly (CMake executable target), so this is not an issue. If ever moved to a static lib, use `WHOLE_ARCHIVE` linker flag.

**offsetof with non-POD**: The `XxxEffect` structs are POD-like (no virtual functions, no non-trivial constructors). `offsetof` is well-defined for standard-layout types in C++20.

**Registration ordering**: Effects init in registration order (which is undefined across TUs). If any effect depends on another being init'd first, that's a problem. Currently no such dependencies exist — each effect is independent.

## Notes

- The feedback shader uniforms (30+ `int feedbackXxxLoc` fields) are NOT part of the effect module pattern — they stay on PostEffect as-is
- Simulations (physarum, boids, etc.) have a different lifecycle pattern (heap-allocated, different init/reset/resize API) — they could get their own `REGISTER_SIMULATION` macro later but are out of scope here
- The `EffectConfig` struct (73 config sub-structs) is a separate concern — it's accessed by UI, serialization, and modulation, all by named field. Changing it would be a much larger refactor
- `post_effect.cpp` drops from ~828 lines to ~150 (core resources + loops)
