# Fix Parameter Registration Bug

Parameters only register when `ModulatableSlider` renders, breaking preset loading when sliders are hidden. Fix by creating a parameter registry that registers at startup and provides min/max to widgets.

## Current State

- `src/ui/modulatable_slider.cpp:100` - registers param on render (bug)
- `src/automation/modulation_engine.cpp:106-109` - skips unregistered params
- `src/ui/imgui_effects.cpp:70-77` - min/max hardcoded in widget calls

---

## Phase 1: Create Parameter Registry

**Goal**: Register all params at startup, provide min/max lookup for widgets.

**Build**:

Create `src/automation/param_registry.h`:
- `struct ParamDef { float min; float max; }`
- `void ParamRegistryInit(EffectConfig* effects)`
- `const ParamDef* ParamRegistryGet(const char* paramId)`

Create `src/automation/param_registry.cpp`:
- Static table with 4 entries: id -> {min, max}
- `ParamRegistryInit`: for each entry, call `ModEngineRegisterParam` with field pointer and min/max from table
- `ParamRegistryGet`: return entry by paramId

Modify `CMakeLists.txt`:
- Add `src/automation/param_registry.cpp`

Modify `src/main.cpp`:
- Include `param_registry.h`
- Call `ParamRegistryInit(&ctx->effects)` after `ModEngineInit()`

**Done when**: Params registered at startup.

---

## Phase 2: Update Widget

**Goal**: Widget uses registry for min/max, no longer registers.

**Build**:

Modify `src/ui/modulatable_slider.h`:
- Remove min/max from signature

Modify `src/ui/modulatable_slider.cpp`:
- Include `param_registry.h`
- Query `ParamRegistryGet(paramId)` for min/max
- Remove `ModEngineRegisterParam` call

Modify `src/ui/imgui_effects.cpp`:
- Remove min/max args from 4 widget calls

**Done when**: No duplication, widget compiles.

---

## Files Summary

**Created:**
- `src/automation/param_registry.h`
- `src/automation/param_registry.cpp`

**Modified:**
- `src/main.cpp` - one include, one init call
- `src/ui/modulatable_slider.h` - remove 2 params
- `src/ui/modulatable_slider.cpp` - query registry, remove registration
- `src/ui/imgui_effects.cpp` - update 4 call sites
- `CMakeLists.txt` - add source
