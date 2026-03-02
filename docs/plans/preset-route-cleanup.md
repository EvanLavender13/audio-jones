# Preset Route Cleanup

When saving a preset, modulation routes for disabled effects persist in the file. Routes should be stripped at save time so the preset only contains routes for enabled effects. Routes remain live in the engine (toggling an effect back on keeps its routes functional) — only the serialized preset is filtered.

## Design

### Bug Mechanism

1. User creates modulation routes for effect params (e.g., `bloom.threshold`)
2. User disables the effect via checkbox
3. Nothing removes routes from the engine — `ModEngineRemoveRoutesMatching` is never called on disable
4. `PresetFromAppConfigs` → `ModulationConfigFromEngine` captures ALL routes from the engine, including those for disabled effects
5. Preset file contains stale routes

### Fix Approach

Add a `paramPrefix` field to `EffectDescriptor` so we can map route paramIds back to their owning effect. After `ModulationConfigFromEngine` captures all routes, a new `ModulationConfigStripDisabledRoutes` function walks the captured routes, checks each paramId prefix against the descriptor table, and removes routes whose owning effect is disabled.

### Types

Add one field to `EffectDescriptor`:

```
const char* paramPrefix;  // e.g., "bloom.", "hexRush." — nullptr when no params
```

New function:

```
void ModulationConfigStripDisabledRoutes(ModulationConfig* config, const EffectConfig* effects);
```

### Algorithm

For each route in `config->routes[0..count-1]`:
1. Extract prefix: find first `'.'` in `paramId`, compare substring up to and including the dot
2. Walk `EFFECT_DESCRIPTORS[0..TRANSFORM_EFFECT_COUNT-1]`:
   - If `desc.paramPrefix != nullptr` and prefix matches `desc.paramPrefix`
   - Read enabled state: `*(const bool*)((const char*)effects + desc.enabledOffset)`
   - If disabled: mark route for removal
3. Compact the array: shift remaining routes down, update `config->count`

### Registration Macro Changes

Each macro already receives `field` as a parameter. Add `#field "."` to the descriptor initializer. This produces string literals like `"bloom."`, `"hexRush."`, etc.

Manual registrations (bloom.cpp, oil_paint.cpp, anamorphic_streak.cpp, slit_scan_corridor.cpp) add explicit `"fieldName."` string. The accumComposite registration in shader_setup.cpp uses `nullptr` (no modulatable params).

---

## Tasks

### Wave 1: Add paramPrefix to EffectDescriptor

#### Task 1.1: Add paramPrefix field and update registration macros

**Files**: `src/config/effect_descriptor.h`

**Do**:
- Add `const char* paramPrefix = nullptr;` field to `EffectDescriptor` after `enabledOffset` (before `flags`)
- In all 5 registration macros (`REGISTER_EFFECT`, `REGISTER_EFFECT_CFG`, `REGISTER_GENERATOR`, `REGISTER_GENERATOR_FULL`, `REGISTER_SIM_BOOST`), add `#field "."` to the `EffectDescriptor` initializer in the correct field position

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Filter logic and manual registrations

#### Task 2.1: Add ModulationConfigStripDisabledRoutes

**Files**: `src/config/modulation_config.h`, `src/config/modulation_config.cpp`

**Do**:
- In `modulation_config.h`: add declaration `void ModulationConfigStripDisabledRoutes(ModulationConfig* config, const EffectConfig* effects);`
- In `modulation_config.cpp`: add `#include "config/effect_descriptor.h"`, implement the function per the Algorithm section above. Use `strncmp` to match the paramId prefix against each descriptor's `paramPrefix`. Compact the routes array in-place when removing entries.

**Verify**: Compiles.

#### Task 2.2: Call strip function in PresetFromAppConfigs

**Files**: `src/config/preset.cpp`

**Do**:
- In `PresetFromAppConfigs`, after the `ModulationConfigFromEngine(&preset->modulation)` call, add: `ModulationConfigStripDisabledRoutes(&preset->modulation, configs->effects);`
- No new includes needed — `modulation_config.h` is already included via `preset.h`

**Verify**: Compiles.

#### Task 2.3: Update manual registrations with paramPrefix

**Files**: `src/effects/bloom.cpp`, `src/effects/oil_paint.cpp`, `src/effects/anamorphic_streak.cpp`, `src/effects/slit_scan_corridor.cpp`, `src/render/shader_setup.cpp`

**Do**:
- Each manual `EffectDescriptorRegister` call constructs an `EffectDescriptor{...}` initializer. Add the `paramPrefix` field value in the correct position (after `enabledOffset`, before `flags`):
  - bloom.cpp: `"bloom."`
  - oil_paint.cpp: `"oilPaint."`
  - anamorphic_streak.cpp: `"anamorphicStreak."`
  - slit_scan_corridor.cpp: `"slitScanCorridor."`
  - shader_setup.cpp (accumComposite): `nullptr`

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Save a preset with a route for an enabled effect → route appears in JSON
- [ ] Disable that effect, re-save → route is gone from JSON
- [ ] Re-enable the effect → route still works in the engine (not stripped from live state)
- [ ] Routes for non-effect params (flowField.*, effects.*, drawable.*) are never stripped
