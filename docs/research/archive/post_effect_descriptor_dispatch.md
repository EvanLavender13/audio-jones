# PostEffect Descriptor Dispatch

Replace the ~150 named effect-state fields in `PostEffect` (`post_effect.h:170-...`) with a descriptor-driven `void* effects[TRANSFORM_EFFECT_COUNT]` array. Each effect module owns a single file-local `static <Name>Effect` instance and registers its address through the existing `EFFECT_DESCRIPTORS[]` table. `post_effect.cpp`'s init/uninit/resize orchestration collapses into descriptor loops, and adding a new effect stops requiring any edit to `post_effect.h` or `post_effect.cpp`.

## Classification

- **Category**: General (architecture)
- **Pipeline Position**: Not a pipeline component. Refactors the runtime state container that the render pipeline reads from.

## References

- `src/config/effect_descriptor.h` — the existing descriptor table that already abstracts per-effect lifecycle (init / uninit / resize / setup / getShader / drawParams). This refactor extends it with a `state` slot.
- `src/render/post_effect.h:170-...` — the ~150 named fields targeted for removal.
- `src/render/post_effect.cpp` — the init/uninit/resize orchestration that becomes descriptor-loop driven.

## Reference Code

The existing `EffectDescriptor` shape and a representative registration macro:

```cpp
struct EffectDescriptor {
  TransformEffectType type;
  const char *name;
  const char *categoryBadge;
  int categorySectionIndex;
  size_t enabledOffset;
  const char *paramPrefix = nullptr;
  uint8_t flags;

  bool (*init)(PostEffect *pe, int w, int h);
  void (*uninit)(PostEffect *pe);
  void (*resize)(PostEffect *pe, int w, int h);
  void (*registerParams)(EffectConfig *cfg);

  Shader *(*getShader)(PostEffect *pe);
  RenderPipelineShaderSetupFn setup;

  GetShaderFn getScratchShader = nullptr;
  RenderPipelineShaderSetupFn scratchSetup = nullptr;
  void (*render)(PostEffect *pe) = nullptr;

  DrawParamsFn drawParams = nullptr;
  DrawOutputFn drawOutput = nullptr;
};

// Existing REGISTER_EFFECT thunk shape (effect_descriptor.h:142-165):
#define REGISTER_EFFECT(Type, Name, field, ...)                          \
  static bool Init_##field(PostEffect *pe, int, int) {                   \
    return Name##EffectInit(&pe->field);                                 \
  }                                                                       \
  static void Uninit_##field(PostEffect *pe) {                           \
    Name##EffectUninit(&pe->field);                                      \
  }                                                                       \
  static Shader *GetShader_##field(PostEffect *pe) {                     \
    return &pe->field.shader;                                            \
  }                                                                       \
  static bool reg_##field = EffectDescriptorRegister(...);
```

Every thunk dereferences `&pe->field`. The refactor changes that dereference site, nothing else.

## Algorithm

### 1. Extend `EffectDescriptor` with a state slot

Add one field:

```cpp
struct EffectDescriptor {
  // ...existing fields...
  void *state = nullptr;  // points to the effect module's static instance
};
```

`state` is set at registration time by the macro and points to a file-local `static <Name>Effect g_<field>State;` defined in the effect's own `.cpp`. Lifetime is the program lifetime — no allocation, no construct/destruct callbacks.

### 2. Replace `PostEffect`'s named fields with a slot array

```cpp
struct PostEffect {
  // ...non-effect state stays (shaders for fxaa/clarity/gamma, halfRes textures,
  // BlendCompositor*, screen dimensions, EffectConfig effects)...

  void *effectStates[TRANSFORM_EFFECT_COUNT];  // mirrors descriptor.state
};
```

`PostEffectInit` populates the array by walking `EFFECT_DESCRIPTORS[]`:

```cpp
for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
  pe->effectStates[i] = EFFECT_DESCRIPTORS[i].state;
}
```

The slot array exists only so call sites that already have a `PostEffect*` can resolve a state pointer without going through the global descriptor table. It's a convenience indirection over `EFFECT_DESCRIPTORS[type].state`.

### 3. Update registration macros to declare a static instance and a typed accessor

The macros at the bottom of each effect `.cpp` already wrap `&pe->field` in static thunks. Change two things:

- Declare a file-local `static <Name>Effect g_<field>State;` at macro expansion.
- Replace `&pe->field` in every thunk with `((<Name>Effect*)pe->effectStates[Type])`.
- Set `EffectDescriptor::state = &g_<field>State` in the registration call.

Sketch (REGISTER_EFFECT shape):

```cpp
#define REGISTER_EFFECT(Type, Name, field, ...)                          \
  static Name##Effect g_##field##State;                                  \
  void Setup##Name(PostEffect *);                                        \
  static bool Init_##field(PostEffect *pe, int, int) {                   \
    return Name##EffectInit((Name##Effect *)pe->effectStates[Type]);     \
  }                                                                       \
  static void Uninit_##field(PostEffect *pe) {                           \
    Name##EffectUninit((Name##Effect *)pe->effectStates[Type]);          \
  }                                                                       \
  static Shader *GetShader_##field(PostEffect *pe) {                     \
    return &((Name##Effect *)pe->effectStates[Type])->shader;            \
  }                                                                      \
  static bool reg_##field = EffectDescriptorRegister(                    \
      Type,                                                              \
      EffectDescriptor{                                                  \
        Type, /* ...metadata... */,                                      \
        Init_##field, Uninit_##field, /* ...lifecycle... */,             \
        GetShader_##field, Setup##Name,                                  \
        /* ...other fields... */,                                        \
        &g_##field##State                                                \
      });
```

The same shape applies to `REGISTER_EFFECT_CFG`, `REGISTER_GENERATOR`, `REGISTER_GENERATOR_FULL`, and `REGISTER_SIM_BOOST`. Each variant differs only in which Init signature it adapts; none look at `&pe->field` for any reason other than locating the effect's state.

### 4. Replace external `pe->field` access sites with a typed accessor

Anywhere outside the registration macros that still reads `pe->bloom.shader`, `pe->kuwahara.cfg`, etc. — render pipeline orchestration, shader setup files, anywhere — replace with a one-line accessor declared in the effect's header:

```cpp
// in effects/bloom.h
inline BloomEffect *GetBloomEffect(PostEffect *pe) {
  return (BloomEffect *)pe->effectStates[BLOOM_EFFECT];
}
```

Call sites become `GetBloomEffect(pe)->shader` instead of `pe->bloom.shader`. One inline accessor per effect, declared next to the existing `BloomEffectInit/Setup/Uninit` functions.

This is the only invasive part of the migration. Every existing `pe->field` site needs to be located and converted. The scope is bounded by `grep -nE "pe->[a-zA-Z]+\.(shader|cfg|...)"` in `src/`.

### 5. Collapse `post_effect.cpp` orchestration

Init becomes:

```cpp
for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
  const EffectDescriptor &d = EFFECT_DESCRIPTORS[i];
  pe->effectStates[i] = d.state;
  if (d.init && !d.init(pe, w, h)) {
    // cascade-uninit prior slots, then fail
    for (int j = i - 1; j >= 0; j--) {
      if (EFFECT_DESCRIPTORS[j].uninit) {
        EFFECT_DESCRIPTORS[j].uninit(pe);
      }
    }
    return false;
  }
}
```

Uninit and resize follow the same loop shape. The current sequential init blocks, the matching uninit calls, the resize switch, and the register-params call site all become single descriptor loops.

### 6. Move feedback uniform-loc bloat into `FeedbackEffect`

The 30+ `feedback*Loc` ints currently sit on `PostEffect` directly (`post_effect.h:186-214`). These are feedback-only — move them into the existing `FeedbackEffect` struct so they live in `g_feedbackState` along with the rest. No new mechanism needed; this is a straight relocation that the descriptor refactor unblocks.

### Migration shape

- New effects: zero changes to `post_effect.h` or `post_effect.cpp`. The registration macro at the bottom of the effect's `.cpp` is the only edit point.
- Existing effects: one edit per effect to switch the registration macro variant (mechanical) plus locate-and-replace `pe->field` access sites with the typed accessor.
- Header: `post_effect.h` loses the ~100 `#include "effects/<name>.h"` lines and the ~150 named fields. It still needs forward declarations for non-effect state (BlendCompositor, Physarum, etc).

## Parameters

Not applicable (architecture refactor, no user-facing parameters).

## Notes

- **Single-instance assumption is already true.** `PostEffect` is currently a singleton (`g_postEffect` in `main.cpp` or equivalent). The static-instance ownership model bakes that assumption into the type system. If multi-instance support is ever needed, the path forward is making `state` a heap allocation owned by `PostEffect` (Option A from the design discussion), but doing that now would add an allocator without solving any current problem.
- **Type safety lives in the accessor functions, not the storage.** `void* effectStates[]` is intentionally untyped storage. The `GetXEffect(PostEffect*)` inlines per effect are the only place a cast happens; everything downstream is fully typed.
- **`paramPrefix`, `enabledOffset`, and `state` are now three pieces of per-effect metadata that live in the descriptor.** Treat the descriptor as the single source of truth for "what does effect X need to function" — adding a new piece of per-effect metadata means adding a descriptor field, not threading a new map through `post_effect.cpp`.
- **The 30 feedback uniform locs are not the only uniform-loc fields on `PostEffect`** — fxaa/clarity/gamma/blur/halfLife/shapeTexture all have their own loc fields. Those are post-process or utility shaders, not transform effects, so they stay on `PostEffect` (or move into purpose-named structs if growth becomes a concern). Out of scope for this refactor.
- **`EffectConfig`'s 152 named fields are NOT touched by this refactor.** That struct is read by every effect's Setup function as `cfg->bloom.threshold` and is the JSON shape the user sees. Converting it to a `void*` array would break ergonomics across every effect. The serialization-side monolith (the 157 NLOHMANN macros) is addressed separately by `effect_serialization_dispatch.md`.
