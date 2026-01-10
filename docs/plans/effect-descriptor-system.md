# Effect Descriptor System

Data-driven effect registration reduces the file footprint for adding new effects from 11 files to 3-4. Each effect declares a descriptor struct that drives shader loading, dispatch, parameter registration, and serialization.

## Current State

Adding a transform effect requires coordinated changes across:

| File | Current Role |
|------|--------------|
| `src/config/*_config.h` | Config struct with defaults |
| `src/config/effect_config.h` | Enum, name switch, EffectConfig field |
| `shaders/*.fs` | Fragment shader |
| `src/render/post_effect.h` | Shader field, uniform location ints |
| `src/render/post_effect.cpp` | Load shader, get uniforms, unload |
| `src/render/shader_setup.h` | Setup function declaration |
| `src/render/shader_setup.cpp` | GetTransformEffect switch, Setup implementation |
| `src/render/render_pipeline.cpp` | Time accumulation |
| `src/ui/imgui_effects.cpp` | UI section, effect order switch |
| `src/automation/param_registry.cpp` | PARAM_TABLE + targets arrays |
| `src/config/preset.cpp` | NLOHMANN macro, to_json/from_json |

**Target state**: Config header, shader file, descriptor file (optionally merged with config).

---

## Phase 1: Descriptor Infrastructure

**Goal**: Define `EffectDescriptor` struct and registry without changing existing code paths.

**Build**:

Create `src/effects/effect_descriptor.h`:
```cpp
#define MAX_EFFECT_UNIFORMS 16
#define MAX_EFFECT_PARAMS 16

enum UniformType {
    UNIFORM_FLOAT,
    UNIFORM_INT,
    UNIFORM_VEC2,
    UNIFORM_VEC3,
    UNIFORM_VEC4
};

enum UniformSource {
    UNIFORM_FROM_CONFIG,    // Read from config struct field
    UNIFORM_FROM_POSTEFFECT // Read from PostEffect field (e.g., time accumulators)
};

struct UniformDesc {
    const char* name;           // Shader uniform name
    UniformType type;
    UniformSource source;
    size_t offset;              // offsetof() into config or PostEffect
};

struct ParamDesc {
    const char* id;             // "effectName.fieldName"
    float min;
    float max;
    size_t offset;              // offsetof() into config struct
};

struct EffectDescriptor {
    TransformEffectType id;
    const char* name;                       // Display name
    const char* shaderPath;                 // "shaders/effect.fs"
    size_t configOffset;                    // offsetof(EffectConfig, effectName)
    size_t enabledOffset;                   // offsetof(ConfigStruct, enabled)

    UniformDesc uniforms[MAX_EFFECT_UNIFORMS];
    int uniformCount;

    ParamDesc params[MAX_EFFECT_PARAMS];
    int paramCount;

    // Optional custom setup (NULL if uniforms table is sufficient)
    void (*customSetup)(struct PostEffect* pe);

    // Optional time accumulator field in PostEffect (0 if none)
    size_t timeAccumOffset;
    // Config field for animation speed (0 if none)
    size_t animSpeedOffset;
};
```

Create `src/effects/effect_registry.h`:
```cpp
void EffectRegistryInit(void);
const EffectDescriptor* EffectRegistryGet(TransformEffectType type);
const EffectDescriptor* EffectRegistryGetAll(int* outCount);
```

Create `src/effects/effect_registry.cpp`:
- Static array of `EffectDescriptor*` indexed by enum value
- `EffectRegistryRegister()` called during init for each effect
- `EffectRegistryGet()` returns descriptor by type

**Done when**: Registry compiles and can store/retrieve descriptors. No behavior changes yet.

---

## Phase 2: Migrate One Effect (Wave Ripple)

**Goal**: Create first descriptor-based effect as proof of concept.

**Build**:

Create `src/effects/descs/wave_ripple_desc.cpp`:
```cpp
#include "effects/effect_descriptor.h"
#include "config/wave_ripple_config.h"
#include "render/post_effect.h"

static const EffectDescriptor WAVE_RIPPLE_DESC = {
    .id = TRANSFORM_WAVE_RIPPLE,
    .name = "Wave Ripple",
    .shaderPath = "shaders/wave_ripple.fs",
    .configOffset = offsetof(EffectConfig, waveRipple),
    .enabledOffset = offsetof(WaveRippleConfig, enabled),
    .uniforms = {
        {"time", UNIFORM_FLOAT, UNIFORM_FROM_POSTEFFECT, offsetof(PostEffect, waveRippleTime)},
        {"octaves", UNIFORM_INT, UNIFORM_FROM_CONFIG, offsetof(WaveRippleConfig, octaves)},
        {"strength", UNIFORM_FLOAT, UNIFORM_FROM_CONFIG, offsetof(WaveRippleConfig, strength)},
        {"frequency", UNIFORM_FLOAT, UNIFORM_FROM_CONFIG, offsetof(WaveRippleConfig, frequency)},
        {"steepness", UNIFORM_FLOAT, UNIFORM_FROM_CONFIG, offsetof(WaveRippleConfig, steepness)},
        {"origin", UNIFORM_VEC2, UNIFORM_FROM_POSTEFFECT, offsetof(PostEffect, currentWaveRippleOrigin)},
        {"shadeEnabled", UNIFORM_INT, UNIFORM_FROM_CONFIG, offsetof(WaveRippleConfig, shadeEnabled)},
        {"shadeIntensity", UNIFORM_FLOAT, UNIFORM_FROM_CONFIG, offsetof(WaveRippleConfig, shadeIntensity)},
    },
    .uniformCount = 8,
    .params = {
        {"waveRipple.strength", 0.0f, 0.5f, offsetof(WaveRippleConfig, strength)},
        {"waveRipple.frequency", 1.0f, 20.0f, offsetof(WaveRippleConfig, frequency)},
        {"waveRipple.steepness", 0.0f, 1.0f, offsetof(WaveRippleConfig, steepness)},
        {"waveRipple.originX", 0.0f, 1.0f, offsetof(WaveRippleConfig, originX)},
        {"waveRipple.originY", 0.0f, 1.0f, offsetof(WaveRippleConfig, originY)},
        {"waveRipple.shadeIntensity", 0.0f, 0.5f, offsetof(WaveRippleConfig, shadeIntensity)},
    },
    .paramCount = 6,
    .customSetup = NULL,  // Uniforms table is sufficient
    .timeAccumOffset = offsetof(PostEffect, waveRippleTime),
    .animSpeedOffset = offsetof(WaveRippleConfig, animSpeed),
};

void WaveRippleDescRegister(void) {
    EffectRegistryRegister(&WAVE_RIPPLE_DESC);
}
```

Call `WaveRippleDescRegister()` from `EffectRegistryInit()`.

**Done when**: Descriptor compiles and registers. Old code paths still active.

---

## Phase 3: Descriptor-Driven Shader Loading

**Goal**: Load shaders from descriptors instead of manual LoadShader calls.

**Build**:

Modify `src/render/post_effect.h`:
- Add `Shader transformShaders[TRANSFORM_EFFECT_COUNT]` array
- Add `int transformUniformLocs[TRANSFORM_EFFECT_COUNT][MAX_EFFECT_UNIFORMS]` array
- Keep existing individual shader fields for non-transform effects (feedback, blur, etc.)

Modify `src/render/post_effect.cpp`:
- Add `LoadTransformShaders()` that iterates registered descriptors:
  ```cpp
  for (int i = 0; i < count; i++) {
      const EffectDescriptor* desc = descs[i];
      pe->transformShaders[desc->id] = LoadShader(0, desc->shaderPath);
      for (int u = 0; u < desc->uniformCount; u++) {
          pe->transformUniformLocs[desc->id][u] =
              GetShaderLocation(pe->transformShaders[desc->id], desc->uniforms[u].name);
      }
  }
  ```
- Add `UnloadTransformShaders()` loop
- Remove individual wave ripple shader loading (after all phases complete)

**Done when**: Transform shaders load from descriptors. Validation confirms all shaders load.

---

## Phase 4: Descriptor-Driven Dispatch

**Goal**: Replace `GetTransformEffect` switch with descriptor lookup.

**Build**:

Create `src/effects/effect_setup.h`:
```cpp
void EffectSetupFromDescriptor(PostEffect* pe, TransformEffectType type);
```

Create `src/effects/effect_setup.cpp`:
```cpp
void EffectSetupFromDescriptor(PostEffect* pe, TransformEffectType type) {
    const EffectDescriptor* desc = EffectRegistryGet(type);
    if (!desc) return;

    // Get config pointer
    char* configBase = (char*)&pe->effects + desc->configOffset;

    Shader shader = pe->transformShaders[type];

    for (int i = 0; i < desc->uniformCount; i++) {
        const UniformDesc* u = &desc->uniforms[i];
        int loc = pe->transformUniformLocs[type][i];

        void* valuePtr;
        if (u->source == UNIFORM_FROM_CONFIG) {
            valuePtr = configBase + u->offset;
        } else {
            valuePtr = (char*)pe + u->offset;
        }

        SetShaderValue(shader, loc, valuePtr, UniformTypeToRaylib(u->type));
    }

    // Call custom setup if provided
    if (desc->customSetup) {
        desc->customSetup(pe);
    }
}
```

Modify `src/render/shader_setup.cpp`:
- `GetTransformEffect` checks if descriptor exists, returns descriptor-based entry
- Fallback to old switch for non-migrated effects

**Done when**: Wave ripple renders through descriptor dispatch. Other effects unchanged.

---

## Phase 5: Descriptor-Driven Param Registry

**Goal**: Param registration reads from descriptors instead of parallel arrays.

**Build**:

Modify `src/automation/param_registry.cpp`:
- Add `RegisterParamsFromDescriptors()`:
  ```cpp
  void RegisterParamsFromDescriptors(EffectConfig* effects) {
      int descCount;
      const EffectDescriptor* const* descs = EffectRegistryGetAll(&descCount);

      for (int i = 0; i < descCount; i++) {
          const EffectDescriptor* desc = descs[i];
          char* configBase = (char*)effects + desc->configOffset;

          for (int p = 0; p < desc->paramCount; p++) {
              const ParamDesc* param = &desc->params[p];
              float* target = (float*)(configBase + param->offset);
              ModEngineRegisterParam(param->id, target, param->min, param->max);
          }
      }
  }
  ```
- Call from `ParamRegistryInit()` after static registrations
- Remove wave ripple entries from PARAM_TABLE and targets arrays

**Done when**: Wave ripple params register from descriptor. Modulation works.

---

## Phase 6: Descriptor-Driven Serialization

**Goal**: Auto-generate serialization from descriptors (optional fields only).

**Build**:

This phase requires storing field metadata in descriptors. Extend `ParamDesc`:
```cpp
struct FieldDesc {
    const char* jsonKey;        // "strength"
    size_t offset;
    enum { FIELD_FLOAT, FIELD_INT, FIELD_BOOL } type;
};
```

Add `FieldDesc fields[]` and `fieldCount` to `EffectDescriptor`.

Modify `src/config/preset.cpp`:
- Keep NLOHMANN macro for config struct (still needed for type generation)
- Add helper that serializes/deserializes effect configs from descriptors:
  ```cpp
  void EffectConfigToJsonFromDescriptor(json& j, const EffectDescriptor* desc,
                                         const void* config);
  void EffectConfigFromJsonFromDescriptor(const json& j, const EffectDescriptor* desc,
                                           void* config);
  ```
- `to_json` for EffectConfig calls descriptor-based serialization for migrated effects

**Done when**: Wave ripple saves/loads through descriptor-based serialization.

---

## Phase 7: Time Accumulator Automation

**Goal**: Automate time accumulation for animated effects.

**Build**:

Modify `src/render/render_pipeline.cpp`:
- In `RenderPipelineApplyOutput()`, iterate descriptors with `timeAccumOffset != 0`:
  ```cpp
  for each descriptor with timeAccumOffset:
      float* timeAccum = (float*)((char*)pe + desc->timeAccumOffset);
      float* animSpeed = (float*)((char*)&pe->effects + desc->configOffset + desc->animSpeedOffset);
      *timeAccum += deltaTime * (*animSpeed);
  ```
- Remove individual time accumulation lines

**Done when**: Time accumulation driven by descriptors. No manual accumulation code.

---

## Phase 8: Migrate Remaining Transform Effects

**Goal**: Convert all 7 transform effects to descriptor-based registration.

**Order** (simplest to most complex):
1. Radial Streak (2 uniforms, no custom setup)
2. Texture Warp (2 uniforms, no custom setup)
3. Sine Warp (5 uniforms, time accumulator)
4. Infinite Zoom (6 uniforms, time accumulator, snapped param)
5. Voronoi (13 uniforms, time accumulator)
6. Wave Ripple (already done)
7. Kaleidoscope (19 uniforms, multiple computed values, custom setup needed)

For each effect:
- Create `src/effects/descs/{name}_desc.cpp`
- Register in `EffectRegistryInit()`
- Remove old entries from param_registry, shader_setup, post_effect

**Escape hatch**: Kaleidoscope needs `customSetup` for computed focal position, rotation accumulation, and KIFS offset packing.

**Done when**: All transform effects use descriptors. Old switch statements removed.

---

## Phase 9: Cleanup

**Goal**: Remove legacy code paths.

**Build**:
- Remove `GetTransformEffect` switch (now array lookup)
- Remove individual shader fields from `PostEffect` (use array)
- Remove individual uniform location fields (use 2D array)
- Remove `TransformEffectName` switch (descriptor has name)
- Remove effect entries from PARAM_TABLE and targets arrays
- Simplify `to_json`/`from_json` for EffectConfig

**Done when**: No duplicate registration code remains.

---

## Future Extensions (Not in Scope)

These could build on the descriptor system later:

1. **UI Generation**: Descriptors include UI hints (slider ranges, groups, conditionals). `imgui_effects.cpp` generates sections from descriptors instead of manual code.

2. **Hot Reload**: Descriptors loaded from data files instead of compiled. Add effects without recompiling.

3. **Shader Embedding**: Shader source stored in descriptor. Single-file effects become truly single-file.

---

## File Inventory After Completion

**To add a new transform effect**:

| File | What to Add |
|------|-------------|
| `src/config/{name}_config.h` | Config struct with fields and defaults |
| `shaders/{name}.fs` | Fragment shader |
| `src/effects/descs/{name}_desc.cpp` | EffectDescriptor with uniforms, params |
| `src/config/effect_config.h` | Enum entry, EffectConfig field |

**Still needed in central files**:
- Enum entry in `effect_config.h` (could eliminate with X-macro, but low value)
- EffectConfig field (inherent - aggregates all configs)
- NLOHMANN macro in `preset.cpp` (could auto-generate, but adds complexity)

**Eliminated**:
- Manual shader loading in `post_effect.cpp`
- Uniform location fields in `post_effect.h`
- Switch case in `shader_setup.cpp`
- Setup function in `shader_setup.cpp`
- Entries in `param_registry.cpp` parallel arrays
- Time accumulation in `render_pipeline.cpp`
- Effect order switch in `imgui_effects.cpp` (partial - still need UI sections)

**Net reduction**: 11 files → 4 files + 3 one-line additions in central files.
