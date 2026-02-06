---
name: add-effect
description: Use when adding a new transform effect, creating a post-process shader, or planning effect implementation. Provides the complete checklist of files and commonly-missed steps.
---

# Adding a Transform Effect

Follow this checklist when adding a new transform effect to AudioJones. Effects use a modular structure where config, runtime state, and lifecycle functions live together in `src/effects/`.

## Checklist Overview

Transform effects require changes across 12 files. Three steps are commonly missed:

1. **TransformOrderConfig::order array** - Effect won't appear in reorder UI
2. **GetTransformCategory() case** - Effect shows "???" badge in pipeline list
3. **RegisterParams call in PostEffectRegisterParams** - Modulatable parameters won't respond to LFOs/audio

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

typedef struct {EffectName}Effect {
  Shader shader;
  int {param}Loc;      // One int per uniform
  float time;          // Animation accumulator (if animated)
} {EffectName}Effect;

bool {EffectName}EffectInit({EffectName}Effect* e);
void {EffectName}EffectSetup({EffectName}Effect* e, const {EffectName}Config* cfg, float deltaTime);
void {EffectName}EffectUninit({EffectName}Effect* e);
{EffectName}Config {EffectName}ConfigDefault(void);

#endif
```

Create `src/effects/{effect_name}.cpp`:

```cpp
#include "{effect_name}.h"
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

{EffectName}Config {EffectName}ConfigDefault(void) {
  {EffectName}Config cfg;
  cfg.enabled = false;
  cfg.speed = 1.0f;
  return cfg;
}
```

Use snake_case for filename, PascalCase for struct/function names.

## Phase 2: Effect Registration

Modify `src/config/effect_config.h`:

1. **Add include** at top with other effect headers:
   ```cpp
   #include "effects/{effect_name}.h"
   ```

2. **Add enum entry** in `TransformEffectType` before `TRANSFORM_EFFECT_COUNT`:
   ```cpp
   TRANSFORM_{EFFECT_NAME},
   ```

3. **Add name case** in `TransformEffectName()`:
   ```cpp
   case TRANSFORM_{EFFECT_NAME}: return "Effect Name";
   ```

4. **Add to order array** in `TransformOrderConfig::order` initialization:
   ```cpp
   TRANSFORM_{EFFECT_NAME}
   ```
   Place at end of array, before closing brace. **COMMONLY MISSED.**

5. **Add config member** to `EffectConfig` struct:
   ```cpp
   {EffectName}Config {effectName};
   ```

6. **Add enabled case** in `IsTransformEnabled()`:
   ```cpp
   case TRANSFORM_{EFFECT_NAME}: return e->{effectName}.enabled;
   ```

## Phase 3: Shader

Create `shaders/{effect_name}.fs`:

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

## Phase 4: PostEffect Integration

Modify `src/render/post_effect.h`:

1. **Add include**:
   ```cpp
   #include "effects/{effect_name}.h"
   ```

2. **Add effect member** in `PostEffect` struct (after simulation pointers section):
   ```cpp
   {EffectName}Effect {effectName};
   ```

Modify `src/render/post_effect.cpp`:

3. **Init effect** in `PostEffectInit()` after other effect inits:
   ```cpp
   if (!{EffectName}EffectInit(&pe->{effectName})) {
     TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize {effect_name}");
     free(pe);
     return NULL;
   }
   ```

4. **Uninit effect** in `PostEffectUninit()`:
   ```cpp
   {EffectName}EffectUninit(&pe->{effectName});
   ```

## Phase 5: Shader Setup

Modify `src/render/shader_setup_{category}.h`:

1. **Declare setup function**:
   ```cpp
   void Setup{EffectName}(PostEffect* pe);
   ```

Modify `src/render/shader_setup_{category}.cpp`:

2. **Add include**:
   ```cpp
   #include "effects/{effect_name}.h"
   ```

3. **Implement setup function** (delegates to module):
   ```cpp
   void Setup{EffectName}(PostEffect* pe) {
     {EffectName}EffectSetup(&pe->{effectName}, &pe->effects.{effectName}, pe->currentDeltaTime);
   }
   ```

Modify `src/render/shader_setup.cpp`:

4. **Add include** (if not already present for this category):
   ```cpp
   #include "shader_setup_{category}.h"
   ```

5. **Add dispatch case** in `GetTransformEffect()`:
   ```cpp
   case TRANSFORM_{EFFECT_NAME}:
       return { &pe->{effectName}.shader, Setup{EffectName}, &pe->effects.{effectName}.enabled };
   ```

## Phase 6: Build System

Modify `CMakeLists.txt`:

1. **Add to EFFECTS_SOURCES**:
   ```cmake
   set(EFFECTS_SOURCES
       src/effects/sine_warp.cpp
       src/effects/{effect_name}.cpp  # Add here
   )
   ```

## Phase 7: UI Panel

Modify `src/ui/imgui_effects.cpp`:

1. **Add category mapping** in `GetTransformCategory()` switch statement. Match the category to whichever `Draw*Category()` function contains your UI controls. **COMMONLY MISSED.**

Modify `src/ui/imgui_effects_{category}.cpp`:

2. **Add section state** at file top with other static bools:
   ```cpp
   static bool section{EffectName} = false;
   ```

3. **Add helper function** before the appropriate `Draw*Category()`:
   ```cpp
   static void Draw{Category}{EffectName}(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
   {
       if (DrawSectionBegin("Effect Name", categoryGlow, &section{EffectName})) {
           const bool wasEnabled = e->{effectName}.enabled;
           ImGui::Checkbox("Enabled##{id}", &e->{effectName}.enabled);
           if (!wasEnabled && e->{effectName}.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_{EFFECT_NAME}); }
           if (e->{effectName}.enabled) {
               ModulatableSlider("Param", &e->{effectName}.param, "effectName.param", "%.2f", modSources);
           }
           DrawSectionEnd();
       }
   }
   ```

4. **Add helper call** in the orchestrator with spacing:
   ```cpp
   ImGui::Spacing();
   Draw{Category}{EffectName}(e, modSources, categoryGlow);
   ```

   Use `ModulatableSlider` for parameters that should respond to modulation.
   Use `ModulatableSliderAngleDeg` for angular parameters (displays degrees, stores radians).

## Phase 8: Preset Serialization

Modify `src/config/preset.cpp`:

1. **Add JSON macro** with other config macros:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT({EffectName}Config, enabled, {param1}, {param2})
   ```

2. **Add to_json entry** in `to_json(json& j, const EffectConfig& e)`:
   ```cpp
   if (e.{effectName}.enabled) { j["{effectName}"] = e.{effectName}; }
   ```

3. **Add from_json entry** in `from_json(const json& j, EffectConfig& e)`:
   ```cpp
   e.{effectName} = j.value("{effectName}", e.{effectName});
   ```

## Phase 9: Parameter Registration (if modulatable)

Each effect registers its own modulatable params via a `RegisterParams` function in its module. `PARAM_TABLE` in `param_registry.cpp` holds only legacy entries — new effects never touch it.

Add to `src/effects/{effect_name}.h`:

1. **Declare RegisterParams**:
   ```cpp
   void {EffectName}RegisterParams({EffectName}Config *cfg);
   ```

Add to `src/effects/{effect_name}.cpp`:

2. **Implement RegisterParams** using `ModEngineRegisterParam`:
   ```cpp
   #include "automation/modulation_engine.h"

   void {EffectName}RegisterParams({EffectName}Config *cfg) {
     ModEngineRegisterParam("{effectName}.{param}", &cfg->{param}, {min}f, {max}f);
   }
   ```

   Each call takes: parameter ID string (matches UI slider's `paramId` argument), pointer to the field, min bound, max bound.

   For angular parameters, use the constants from `ui_units.h`:
   - `ROTATION_SPEED_MAX` for speed fields (radians/second)
   - `ROTATION_OFFSET_MAX` for angle/twist fields (radians)

Add to `src/render/post_effect.cpp`:

3. **Call RegisterParams** in `PostEffectRegisterParams()`:
   ```cpp
   {EffectName}RegisterParams(&pe->effects.{effectName});
   ```

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
| `src/effects/{effect}.h` | Config struct, Effect struct, lifecycle declarations |
| `src/effects/{effect}.cpp` | Init, Setup, Uninit, ConfigDefault, RegisterParams implementations |
| `src/config/effect_config.h` | Include, enum, name, order array, member, IsTransformEnabled case |
| `shaders/{effect}.fs` | Create fragment shader |
| `src/render/post_effect.h` | Include, Effect member |
| `src/render/post_effect.cpp` | Init, Uninit, and RegisterParams calls |
| `src/render/shader_setup_{category}.h` | Declare Setup function |
| `src/render/shader_setup_{category}.cpp` | Include, Setup delegates to module |
| `src/render/shader_setup.cpp` | Include and dispatch case |
| `CMakeLists.txt` | Add to EFFECTS_SOURCES |
| `src/ui/imgui_effects.cpp` | GetTransformCategory case |
| `src/ui/imgui_effects_{category}.cpp` | Section state and UI controls |
| `src/config/preset.cpp` | JSON macro, to_json, from_json |
