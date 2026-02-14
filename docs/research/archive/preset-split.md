# Preset Serialization Split

Split `preset.cpp` (1132 lines) into two files and collapse mechanical serialization code using an X-macro.

## Classification

- **Category**: General (software architecture)
- **Pipeline Position**: N/A — serialization infrastructure
- **Affected files**: `src/config/preset.cpp`, new `src/config/effect_serialization.cpp`

## Problem

`preset.cpp` mixes three unrelated concerns in one file:

1. **NLOHMANN macros** (lines 18-508, ~490 lines): 70+ `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macros for config structs, plus custom `to_json`/`from_json` for `ColorConfig`
2. **EffectConfig to_json/from_json** (lines 570-928, ~360 lines): hand-written per-field serialization with 73 identical `if (e.X.enabled) j["X"] = e.X` / `e.X = j.value("X", e.X)` patterns
3. **Preset I/O** (lines 929-1144, ~215 lines): top-level `Preset` serialization, file load/save, `AppConfigs` conversion

Every new effect adds 4 mechanical lines to this file: one NLOHMANN macro, one to_json line, one from_json line, plus the field in `EffectConfig`.

## Approach

### File Split

| File | Contents | ~Lines |
|------|----------|--------|
| `src/config/effect_serialization.cpp` | All NLOHMANN macros + EffectConfig to_json/from_json (with X-macro) | ~550 |
| `src/config/effect_serialization.h` | Declares `to_json`/`from_json` for EffectConfig (needed by preset.cpp) | ~15 |
| `src/config/preset.cpp` (trimmed) | Drawable/LFO/Preset serialization, file I/O, AppConfigs conversion | ~215 |

### X-Macro for EffectConfig Serialization

Define a table of all effect config field names:

```cpp
#define EFFECT_CONFIG_FIELDS(X) \
    X(kaleidoscope) \
    X(kifs) \
    X(latticeFold) \
    X(voronoi) \
    X(physarum) \
    ... // all 73 fields
```

Expand differently for to_json and from_json:

```cpp
// to_json: only serialize enabled effects (keeps preset files small)
#define SERIALIZE_EFFECT(name) \
    if (e.name.enabled) j[#name] = e.name;
EFFECT_CONFIG_FIELDS(SERIALIZE_EFFECT)
#undef SERIALIZE_EFFECT

// from_json: always deserialize (missing keys get defaults via j.value)
#define DESERIALIZE_EFFECT(name) \
    e.name = j.value(#name, e.name);
EFFECT_CONFIG_FIELDS(DESERIALIZE_EFFECT)
#undef DESERIALIZE_EFFECT
```

Non-effect fields (halfLife, blurScale, flowField, gamma, clarity, etc.) and the transformOrder stay as individual lines — there are only ~11 of these.

### Special Cases

- **ColorConfig**: Has custom to_json/from_json (mode-switched serialization) — stays as hand-written functions
- **DualLissajousConfig, FlowFieldConfig, etc.**: Non-effect config macros — move to effect_serialization.cpp alongside the effect macros
- **Drawable, AudioConfig, LFOConfig macros**: Stay in preset.cpp since they're Preset-level concerns

### Header for Cross-File Visibility

`effect_serialization.h` declares just what `preset.cpp` needs:

```cpp
#include <nlohmann/json_fwd.hpp>
#include "effect_config.h"

void to_json(nlohmann::json &j, const EffectConfig &e);
void from_json(const nlohmann::json &j, EffectConfig &e);
```

## Parameters

N/A — no visual parameters.

## Notes

- No preset format change — JSON output is byte-identical
- The X-macro table also serves as the single source of truth for "which fields does EffectConfig serialize?" — adding a new effect means adding one line to the X-macro and one NLOHMANN macro
- MoireLayerConfig (embedded in MoireGeneratorConfig) needs its own NLOHMANN macro in effect_serialization.cpp — it's currently there already
- The NOLINTNEXTLINE for readability-function-size on `to_json`/`from_json` can be removed since the X-macro makes the functions much shorter
- Build system: add `src/config/effect_serialization.cpp` to CMakeLists.txt
