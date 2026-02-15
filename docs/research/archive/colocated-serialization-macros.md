# Colocated Serialization Macros

Move each effect's JSON field list from the centralized `effect_serialization.cpp` into its own header, so adding a config field only requires editing one file.

## Classification

- **Category**: General (software architecture)
- **Pipeline Position**: N/A — build system / code organization change
- **Visual Impact**: None

## Problem

Currently, `effect_serialization.cpp` contains 89 `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(...)` macro invocations listing every serializable field for every config struct. When adding a field to e.g. `BloomConfig` in `bloom.h`, you must also update the macro call in `effect_serialization.cpp` — a separate file with no structural link to the struct definition. Fields are routinely forgotten.

The NLOHMANN macros expand to `inline` function bodies requiring the full `<nlohmann/json.hpp>` header (~25k lines). Including it in 80+ effect headers would devastate compile times. So the macros can't simply move into headers as-is.

## Approach: Field-List Macros

Each effect header defines a preprocessor macro listing its serializable fields. The centralized `effect_serialization.cpp` expands these macros inside the NLOHMANN call. The JSON header stays in one translation unit.

### Before (current)

```cpp
// bloom.h
struct BloomConfig {
  bool enabled = false;
  float threshold = 0.8f;
  float knee = 0.5f;
  float intensity = 0.5f;
  int iterations = 5;
};

// effect_serialization.cpp (hundreds of lines away)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    BloomConfig, enabled, threshold, knee, intensity, iterations)
```

### After (proposed)

```cpp
// bloom.h — field list lives next to the struct
struct BloomConfig {
  bool enabled = false;
  float threshold = 0.8f;
  float knee = 0.5f;
  float intensity = 0.5f;
  int iterations = 5;
};

#define BLOOM_CONFIG_FIELDS enabled, threshold, knee, intensity, iterations

// effect_serialization.cpp — just expands the macro
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BloomConfig, BLOOM_CONFIG_FIELDS)
```

### Preprocessor Mechanics

The C preprocessor expands `BLOOM_CONFIG_FIELDS` before passing arguments to the NLOHMANN variadic macro. The result is identical to the current hand-written field list. No special tricks needed — standard macro expansion.

### Macro Naming Convention

`<UPPER_SNAKE_NAME>_CONFIG_FIELDS` — matches the struct name:

| Struct | Macro |
|--------|-------|
| `BloomConfig` | `BLOOM_CONFIG_FIELDS` |
| `KaleidoscopeConfig` | `KALEIDOSCOPE_CONFIG_FIELDS` |
| `PhysarumConfig` | `PHYSARUM_CONFIG_FIELDS` |
| `DualLissajousConfig` | `DUAL_LISSAJOUS_CONFIG_FIELDS` |
| `FlowFieldConfig` | `FLOW_FIELD_CONFIG_FIELDS` |

### Scope

**Move to effect headers** (~77 effects in `src/effects/*.h`):
- Each effect's `*_CONFIG_FIELDS` macro goes right after its config struct definition

**Move to shared config headers** (~12 configs in `src/config/*.h` and `src/render/*.h`):
- `DualLissajousConfig` → `dual_lissajous_config.h`
- `FlowFieldConfig` → `effect_config.h` (or wherever the struct lives)
- `FeedbackFlowConfig`, `ProceduralWarpConfig` → their respective headers
- Simulation configs (`PhysarumConfig`, `BoidsConfig`, etc.) → their effect headers in `src/effects/` or `src/simulation/`

**Keep in effect_serialization.cpp** (no macro indirection):
- `Color` and `GradientStop` — `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE` (non-defaulted, different macro)
- `ColorConfig` — manual `to_json`/`from_json` with switch on mode (discriminated union)
- `EffectConfig` — master serializer with `EFFECT_CONFIG_FIELDS` batch macro and custom logic
- `TransformOrderConfig` — custom string-based order serialization with legacy support

**effect_serialization.cpp after migration:**
- ~89 one-liner `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Type, TYPE_CONFIG_FIELDS)` calls
- The custom `to_json`/`from_json` for `ColorConfig`, `EffectConfig`, `TransformOrderConfig`
- The `EFFECT_CONFIG_FIELDS` / `SERIALIZE_EFFECT` / `DESERIALIZE_EFFECT` batch macros (unchanged)

## Notes

- Zero compile-time impact: `json.hpp` is still only included in `effect_serialization.cpp` and `preset.cpp`
- Zero runtime impact: preprocessor expansion produces identical code
- If a field is added to the struct but not the macro, it won't serialize — same failure mode as today, but now the macro is 3 lines below the struct instead of 500 lines away in another file
- The `EFFECT_CONFIG_FIELDS` batch macro in `effect_serialization.cpp` (the one listing all effect member names like `sineWarp`, `kaleidoscope`, etc.) is unrelated and stays put — it lists EffectConfig *members*, not per-effect struct fields
