# Drawable Parameter Modulation

Extend the existing modulation system to support drawable parameters. Drawables use stable IDs to survive reorder/delete operations. Start with x/y position parameters, designed to scale to all float parameters.

## Current State

- `src/automation/modulation_engine.cpp` - Routes stored by string paramId, registers `float*` pointers
- `src/automation/param_registry.cpp:11-26` - Static `PARAM_TABLE` for effect params
- `src/config/drawable_config.h:44-73` - Drawable struct with DrawableBase (x, y, etc.) + type union
- `src/ui/imgui_drawables.cpp:260-268` - Delete left-shifts array, reorder swaps elements
- `src/ui/modulatable_slider.cpp` - Drop-in slider with modulation popup

**Challenge**: Drawables live in a dynamic array. Indices change on delete/reorder, invalidating stored pointers.

**Solution**: Stable uint32_t ID per drawable + re-register pointers after array modifications.

## Phase 1: Stable Drawable IDs

**Goal**: Each drawable gets a unique ID that persists across reorder/delete.

**Modify**:
- `src/config/drawable_config.h` - Add `uint32_t id` field to Drawable struct (before `type`)
- `src/ui/imgui_drawables.cpp` - Add static `sNextDrawableId` counter, assign on create
- `src/config/preset.cpp` - Sync counter to `max(loaded IDs) + 1` after load

**Done when**: Create 3 drawables, delete middle one, create another. New drawable has ID=4 (not 2).

---

## Phase 2: Route Cleanup Infrastructure

**Goal**: Remove modulation routes when a drawable is deleted.

**Modify**:
- `src/automation/modulation_engine.h` - Add `ModEngineRemoveRoutesMatching(const char* prefix)`
- `src/automation/modulation_engine.cpp` - Implement: iterate routes, remove those starting with prefix, reset params to base

**Done when**: Unit test passes - add route "drawable.5.x", call `ModEngineRemoveRoutesMatching("drawable.5.")`, route gone.

---

## Phase 3: Drawable Params Module

**Goal**: Centralized management of drawable parameter registration.

**Create**:
- `src/automation/drawable_params.h` - Declare `DrawableParamsRegister`, `DrawableParamsUnregister`, `DrawableParamsSyncAll`
- `src/automation/drawable_params.cpp` - Implement registration for x/y params using paramId format `"drawable.<id>.x"`

**Functions**:
```
DrawableParamsRegister(Drawable* d)      - Register x/y for one drawable
DrawableParamsUnregister(uint32_t id)    - Remove routes matching "drawable.<id>."
DrawableParamsSyncAll(Drawable* arr, int count) - Re-register all drawables (call after delete/reorder)
```

**Done when**: Can call `DrawableParamsRegister` and see params in ModEngine.

---

## Phase 4: Dynamic Param Lookup

**Goal**: ModulatableSlider can look up bounds for drawable params.

**Modify**:
- `src/automation/param_registry.h` - Add `ParamRegistryGetDynamic(paramId, defaultMin, defaultMax, outDef)`
- `src/automation/param_registry.cpp` - Check static table first, then recognize `"drawable.*"` pattern and return provided defaults
- `src/ui/modulatable_slider.cpp:276` - Replace `ParamRegistryGet` with `ParamRegistryGetDynamic`

**Done when**: `ParamRegistryGetDynamic("drawable.1.x", 0.0f, 1.0f, &def)` returns true with min=0, max=1.

---

## Phase 5: Drawable Slider Wrapper

**Goal**: Reduce boilerplate when adding modulatable sliders to drawable UI.

**Create**:
- `src/ui/modulatable_drawable_slider.h` - Declare wrapper function
- `src/ui/modulatable_drawable_slider.cpp` - Build paramId, delegate to ModulatableSlider

**Signature**:
```cpp
bool ModulatableDrawableSlider(const char* label, float* value,
                                uint32_t drawableId, const char* field,
                                const char* format, const ModSources* sources);
```

**Done when**: Wrapper compiles and calls through to ModulatableSlider.

---

## Phase 6: UI Integration

**Goal**: X/Y sliders in drawable panel support modulation.

**Modify**:
- `src/ui/imgui_panels.h` - Add `const ModSources* sources` param to `ImGuiDrawDrawablesPanel`
- `src/main.cpp` - Pass `&ctx->modSources` to drawable panel
- `src/ui/imgui_drawables.cpp`:
  - Include new headers
  - Replace `ImGui::SliderFloat("X"/Y")` with `ModulatableDrawableSlider` in all 3 Draw*Controls functions
  - Call `DrawableParamsRegister` on create (after ID assignment)
  - Call `DrawableParamsUnregister` + `DrawableParamsSyncAll` on delete
  - Call `DrawableParamsSyncAll` after reorder (Up/Down buttons)

**Done when**: Diamond indicator appears on X/Y sliders, clicking opens modulation popup.

---

## Phase 7: Preset Persistence

**Goal**: Modulation routes survive save/load.

**Modify**:
- `src/config/preset.cpp`:
  - On load: after copying drawables, call `DrawableParamsSyncAll` to register with current pointers
  - On load: sync ID counter to prevent collisions

**Done when**: Save preset with bass→X modulation, load it, modulation still active.

---

## File Summary

**Create (4 files)**:
- `src/automation/drawable_params.h`
- `src/automation/drawable_params.cpp`
- `src/ui/modulatable_drawable_slider.h`
- `src/ui/modulatable_drawable_slider.cpp`

**Modify (9 files)**:
- `src/config/drawable_config.h`
- `src/automation/modulation_engine.h`
- `src/automation/modulation_engine.cpp`
- `src/automation/param_registry.h`
- `src/automation/param_registry.cpp`
- `src/ui/modulatable_slider.cpp`
- `src/ui/imgui_drawables.cpp`
- `src/ui/imgui_panels.h`
- `src/main.cpp`
- `src/config/preset.cpp`
