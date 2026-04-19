---
name: add-effect
description: Use when adding a new transform effect, creating a post-process shader, or planning effect implementation. Provides the complete checklist of files and commonly-missed steps.
---

# Adding a Transform Effect

Follow this checklist when adding a new transform effect to AudioJones. Effects use a self-registering modular structure — config, runtime state, lifecycle functions, and registration all live together in `src/effects/`.

## Checklist Overview

Transform effects require changes across 7 files. The `REGISTER_EFFECT` macro at the bottom of the `.cpp` file handles lifecycle registration, descriptor metadata, UI dispatch, and pipeline integration — no central lists to edit.

Steps commonly missed:

1. **TransformOrderConfig::order array** - Effect won't appear in reorder UI
2. **REGISTER_EFFECT macro** at bottom of `.cpp` - Effect won't init, won't appear in pipeline
3. **Effect member in PostEffect struct** - No runtime state for the effect

## Phase 1: Effect Module

Create `src/effects/{effect_name}.h`:

```cpp
#ifndef {EFFECT_NAME}_EFFECT_H
#define {EFFECT_NAME}_EFFECT_H

#include "raylib.h"
#include <stdbool.h>

// Brief description of what the effect does
struct {EffectName}Config {
  bool enabled = false;
  // Add effect-specific parameters with defaults
  float speed = 1.0f;  // Animation rate (radians/second)
};



#define {EFFECT_NAME}_CONFIG_FIELDS enabled, speed

typedef struct {EffectName}Effect {
  Shader shader;
  int {param}Loc;      // One int per uniform
  float time;          // Animation accumulator (if animated)
} {EffectName}Effect;

bool {EffectName}EffectInit({EffectName}Effect* e);
void {EffectName}EffectSetup({EffectName}Effect* e, const {EffectName}Config* cfg, float deltaTime);
void {EffectName}EffectUninit({EffectName}Effect* e);
void {EffectName}RegisterParams({EffectName}Config *cfg);
#endif
```

Create `src/effects/{effect_name}.cpp`:

```cpp
#include "{effect_name}.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"

#include "imgui.h"
#include "ui/modulatable_slider.h"

#include <stddef.h>

bool {EffectName}EffectInit({EffectName}Effect* e) {
  e->shader = LoadShader(NULL, "shaders/{effect_name}.fs");
  if (e->shader.id == 0) return false;

  e->{param}Loc = GetShaderLocation(e->shader, "{param}");
  e->time = 0.0f;
  return true;
}

void {EffectName}EffectSetup({EffectName}Effect* e, const {EffectName}Config* cfg, float deltaTime) {
  e->time += cfg->speed * deltaTime;
  SetShaderValue(e->shader, e->{param}Loc, &cfg->{param}, SHADER_UNIFORM_FLOAT);
}

void {EffectName}EffectUninit({EffectName}Effect* e) {
  UnloadShader(e->shader);
}

void {EffectName}RegisterParams({EffectName}Config *cfg) {
  ModEngineRegisterParam("{effectName}.{param}", &cfg->{param}, {min}f, {max}f);
}

// === UI ===

static void Draw{EffectName}Params(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  {EffectName}Config *cfg = &e->{effectName};

  ModulatableSlider("Param##{effectName}", &cfg->{param},
                    "{effectName}.{param}", "%.2f", ms);
}

static void Setup{EffectName}(PostEffect* pe) {
  {EffectName}EffectSetup(&pe->{effectName}, &pe->effects.{effectName}, pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_{EFFECT_NAME}, {EffectName}, {effectName},
                "Effect Name", "{CAT}", {sectionIndex}, EFFECT_FLAG_NONE,
                Setup{EffectName}, NULL, Draw{EffectName}Params)
// clang-format on
```

The `REGISTER_EFFECT` macro at the bottom handles:
- Descriptor metadata (display name, category badge, section index, flags, enabledOffset)
- Lifecycle registration (init, uninit, registerParams, getShader)
- Setup function dispatch binding
- UI draw callback (the dispatch system calls `drawParams` per effect)

The `// === UI ===` section contains the colocated `Draw{EffectName}Params()` function with signature `(EffectConfig*, const ModSources*, ImU32)`. The dispatch system in `imgui_effects_dispatch.cpp` handles section begin/end, enable checkbox, and transform reordering — the draw function only renders effect-specific sliders.

The setup function (`Setup{EffectName}`) is defined as `static` in the `.cpp` file just above the macro. It delegates to the module's `{EffectName}EffectSetup()` function, bridging the `PostEffect*` context to the module's own types.

Use snake_case for filename, PascalCase for struct/function names.

### Macro Variants

Pick the right macro based on the Init signature:

| Macro | Init signature | Use when |
|-------|---------------|----------|
| `REGISTER_EFFECT` | `Init(Effect*)` | Most effects (simple init) |
| `REGISTER_EFFECT_CFG` | `Init(Effect*, Config*)` | Init needs config (e.g., LUT setup) |
| `REGISTER_EFFECT_SIZED` | `Init(Effect*, w, h)` | Init needs resolution |
| `REGISTER_EFFECT_FULL` | `Init(Effect*, Config*, w, h)` | Init needs both |
| `REGISTER_GENERATOR` | `Init(Effect*, Config*)` | Generator (bakes GEN/10/BLEND) |
| `REGISTER_GENERATOR_FULL` | `Init(Effect*, Config*, w, h)` | Generator with resize |
| `REGISTER_SIM_BOOST` | No init | Sim boost (no lifecycle) |

For multi-pass effects with composite shaders (like bloom, anamorphic streak), write a manual `EffectDescriptorRegister()` call with custom GetShader/Resize wrappers instead of using a macro.

### Macro Parameters

```
REGISTER_EFFECT(Type, Name, field, displayName, badge, section, flags, SetupFn, ResizeFn, DrawParamsFnArg)
```

- `Type`: `TransformEffectType` enum value (e.g., `TRANSFORM_SINE_WARP`)
- `Name`: PascalCase prefix for functions (e.g., `SineWarp`)
- `field`: camelCase field name on PostEffect and EffectConfig (e.g., `sineWarp`)
- `displayName`: UI display string (e.g., `"Sine Warp"`)
- `badge`: Category badge
- `section`: Category section index
- `flags`: `EFFECT_FLAG_NONE`, `EFFECT_FLAG_HALF_RES`, `EFFECT_FLAG_NEEDS_RESIZE`
- `SetupFn`: Setup function name (e.g., `SetupSineWarp`) — defined as `static` above the macro in the same `.cpp`
- `ResizeFn`: Resize function or `NULL`
- `DrawParamsFnArg`: Colocated UI draw function (e.g., `DrawSineWarpParams`) — signature: `void (*)(EffectConfig*, const ModSources*, ImU32)`

```
REGISTER_GENERATOR(Type, Name, field, displayName, SetupFn, ScratchSetupFn, section, DrawParamsFnArg, DrawOutputFnArg)
```

- `SetupFn`: Blend compositor setup function
- `ScratchSetupFn`: Generator shader setup function
- `section`: Generator sub-category section index (10=Geometric, 11=Filament, 12=Texture, 13=Atmosphere)
- `DrawParamsFnArg`: Effect-specific UI sliders function
- `DrawOutputFnArg`: Output section function (typically `DrawOutput_{field}` generated by `STANDARD_GENERATOR_OUTPUT`)

Category badges: `"SYM"` (0), `"WARP"` (1), `"CELL"` (2), `"MOT"` (3), `"ART"` (4), `"GFX"` (5), `"RET"` (6), `"OPT"` (7), `"COL"` (8), `"SIM"` (9), `"GEN"` (10=Geometric, 11=Filament, 12=Texture, 13=Atmosphere).

## Phase 2: Config Registration

Modify `src/config/effect_config.h`:

1. **Add include** at top with other effect headers:
   ```cpp
   #include "effects/{effect_name}.h"
   ```

2. **Add enum entry** in `TransformEffectType` before `TRANSFORM_EFFECT_COUNT`:
   ```cpp
   TRANSFORM_{EFFECT_NAME},
   ```

3. **Add to order array** in `TransformOrderConfig::order` initialization:
   ```cpp
   TRANSFORM_{EFFECT_NAME}
   ```
   Place at end of array, before closing brace. **COMMONLY MISSED.**

4. **Add config member** to `EffectConfig` struct:
   ```cpp
   {EffectName}Config {effectName};
   ```

## Phase 3: Shader

Create `shaders/{effect_name}.fs`.

**Attribution (REQUIRED if derived from external source):** If the research doc has an `## Attribution` section, the shader file MUST begin with an attribution comment block before the `#version` line:

```glsl
// Based on "Shader Title" by AuthorName
// https://www.shadertoy.com/view/XXXX
// License: CC BY-NC-SA 3.0 Unported
// Modified: [brief description of changes for AudioJones]
```

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
// Add effect-specific uniforms

void main() {
    vec2 uv = fragTexCoord;
    vec4 color = texture(texture0, uv);

    // Effect algorithm here

    finalColor = color;
}
```

## Phase 4: PostEffect Struct

Modify `src/render/post_effect.h`:

1. **Add include**:
   ```cpp
   #include "effects/{effect_name}.h"
   ```

2. **Add effect member** in `PostEffect` struct (after simulation pointers section):
   ```cpp
   {EffectName}Effect {effectName};
   ```

No changes needed in `post_effect.cpp` — the descriptor loop handles init, uninit, resize, and registerParams automatically.

## Phase 5: Preset Serialization

Modify `src/config/effect_serialization.cpp`:

1. **Add include** for the effect header (so the `*_CONFIG_FIELDS` macro is visible):
   ```cpp
   #include "effects/{effect_name}.h"
   ```

2. **Add JSON macro** with the other per-config macros, referencing the header's field-list macro:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT({EffectName}Config, {EFFECT_NAME}_CONFIG_FIELDS)
   ```

3. **Add field name** to the `EFFECT_CONFIG_FIELDS(X)` X-macro table:
   ```cpp
   X({effectName}) \
   ```
   This single entry handles both `to_json` (writes if enabled) and `from_json` (reads with default).

## Verification

After implementation, verify:

- [ ] Effect appears in transform order pipeline
- [ ] Effect shows correct category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to the pipeline list
- [ ] UI controls modify effect in real-time
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to registered parameters (if applicable)
- [ ] Build succeeds with no warnings

## File Summary

| File | Changes |
|------|---------|
| `src/effects/{effect}.h` | Config struct, Effect struct, lifecycle + RegisterParams declarations |
| `src/effects/{effect}.cpp` | Init, Setup, Uninit, RegisterParams, colocated `Draw*Params`, setup bridge, `REGISTER_EFFECT` macro |
| `src/config/effect_config.h` | Include, enum, order array, config member |
| `shaders/{effect}.fs` | Create fragment shader |
| `src/render/post_effect.h` | Include, Effect member |
| `src/config/effect_serialization.cpp` | JSON macro, X-macro field entry |
