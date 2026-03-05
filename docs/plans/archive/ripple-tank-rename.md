# Ripple Tank Rename

Rename the existing "Cymatics" generator effect to "Ripple Tank" across all source code, shaders, presets, and registration. This is a mechanical rename with no behavior changes — the first step toward reorganizing cymatics-family effects under a shared category.

**Research**: `docs/research/cymatics-category.md` (Section 2: Rename)

## Design

### Naming Map

| Current | New |
|---------|-----|
| `CymaticsConfig` | `RippleTankConfig` |
| `CymaticsEffect` | `RippleTankEffect` |
| `CymaticsEffectInit()` | `RippleTankEffectInit()` |
| `CymaticsEffectSetup()` | `RippleTankEffectSetup()` |
| `CymaticsEffectRender()` | `RippleTankEffectRender()` |
| `CymaticsEffectResize()` | `RippleTankEffectResize()` |
| `CymaticsEffectUninit()` | `RippleTankEffectUninit()` |
| `CymaticsConfigDefault()` | `RippleTankConfigDefault()` |
| `CymaticsRegisterParams()` | `RippleTankRegisterParams()` |
| `TRANSFORM_CYMATICS` | `TRANSFORM_RIPPLE_TANK` |
| `CYMATICS_CONFIG_FIELDS` | `RIPPLE_TANK_CONFIG_FIELDS` |
| `src/effects/cymatics.h` | `src/effects/ripple_tank.h` |
| `src/effects/cymatics.cpp` | `src/effects/ripple_tank.cpp` |
| `shaders/cymatics.fs` | `shaders/ripple_tank.fs` |
| `EffectConfig::cymatics` | `EffectConfig::rippleTank` |
| Display name `"Cymatics"` | `"Ripple Tank"` |
| Mod params `"cymatics.*"` | `"rippleTank.*"` |

### Types

No structural changes. `RippleTankConfig` is identical to `CymaticsConfig` with renamed identifiers. `RippleTankEffect` is identical to `CymaticsEffect`.

### Constants

- Enum name: `TRANSFORM_RIPPLE_TANK` (replaces `TRANSFORM_CYMATICS` at same position in enum)
- Display name: `"Ripple Tank"`
- Category section: 13 (unchanged for now — moves to 16 in the category creation plan)
- Badge: `"GEN"` (unchanged)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create ripple_tank.h

**Files**: `src/effects/ripple_tank.h` (create)
**Creates**: `RippleTankConfig`, `RippleTankEffect` types and function declarations

**Do**: Copy `src/effects/cymatics.h` to `src/effects/ripple_tank.h`. Apply the naming map: rename all `Cymatics`→`RippleTank`, `cymatics`→`rippleTank`, `CYMATICS`→`RIPPLE_TANK`. Update include guard to `RIPPLE_TANK_H`. Update top-of-file comment to `// Ripple Tank effect module` / `// Audio-reactive wave interference from virtual point sources`.

**Verify**: File exists with correct types and no references to "Cymatics".

---

### Wave 2: Implementation + Integration

#### Task 2.1: Create ripple_tank.cpp

**Files**: `src/effects/ripple_tank.cpp` (create)

**Do**: Copy `src/effects/cymatics.cpp` to `src/effects/ripple_tank.cpp`. Apply naming map throughout:
- Include becomes `"ripple_tank.h"`
- All function names: `Cymatics`→`RippleTank`
- Bridge functions: `SetupCymatics`→`SetupRippleTank`, `SetupCymaticsBlend`→`SetupRippleTankBlend`, `RenderCymatics`→`RenderRippleTank`
- Config field access: `pe->effects.cymatics`→`pe->effects.rippleTank`, `pe->cymatics`→`pe->rippleTank`
- Mod param IDs: `"cymatics.*"`→`"rippleTank.*"`
- UI labels: `"##cym"`→`"##rt"`, `"cymatics."`→`"rippleTank."`
- Lissajous ID: `"cym_liss"`→`"rt_liss"`, `"cymatics.lissajous"`→`"rippleTank.lissajous"`
- `STANDARD_GENERATOR_OUTPUT(cymatics)`→`STANDARD_GENERATOR_OUTPUT(rippleTank)`
- Registration macro: `TRANSFORM_CYMATICS`→`TRANSFORM_RIPPLE_TANK`, field `cymatics`→`rippleTank`, display name `"Cymatics"`→`"Ripple Tank"`
- `REGISTER_GENERATOR_FULL` args: `SetupCymaticsBlend`→`SetupRippleTankBlend`, `SetupCymatics`→`SetupRippleTank`, `RenderCymatics`→`RenderRippleTank`, `DrawCymaticsParams`→`DrawRippleTankParams`, `DrawOutput_cymatics`→`DrawOutput_rippleTank`
- Shader load path: `"shaders/cymatics.fs"`→`"shaders/ripple_tank.fs"`
- HDR texture label: `"CYMATICS"`→`"RIPPLE_TANK"`
- Top-of-file comment: `// Ripple Tank effect module implementation` / `// Audio-reactive wave interference from virtual point sources`

**Verify**: No references to "cymatics" or "Cymatics" remain.

---

#### Task 2.2: Rename shader

**Files**: `shaders/ripple_tank.fs` (create)

**Do**: Copy `shaders/cymatics.fs` to `shaders/ripple_tank.fs`. No content changes needed — the shader has no self-references.

**Verify**: File exists, identical content to old cymatics.fs.

---

#### Task 2.3: Update effect_config.h

**Files**: `src/config/effect_config.h` (modify)

**Do**:
- Change `#include "effects/cymatics.h"` → `#include "effects/ripple_tank.h"`
- Rename enum value `TRANSFORM_CYMATICS` → `TRANSFORM_RIPPLE_TANK` (same position in enum)
- Rename field `CymaticsConfig cymatics` → `RippleTankConfig rippleTank`
- Update comment: `// Cymatics (interference patterns from virtual speakers)` → `// Ripple Tank (audio-reactive wave interference from virtual point sources)`

**Verify**: Compiles (header-only changes).

---

#### Task 2.4: Update post_effect.h

**Files**: `src/render/post_effect.h` (modify)

**Do**:
- Change `#include "effects/cymatics.h"` → `#include "effects/ripple_tank.h"`
- Rename field `CymaticsEffect cymatics` → `RippleTankEffect rippleTank`

**Verify**: Compiles.

---

#### Task 2.5: Update effect_serialization.cpp

**Files**: `src/config/effect_serialization.cpp` (modify)

**Do**:
- Change `#include "effects/cymatics.h"` → `#include "effects/ripple_tank.h"`
- Rename `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CymaticsConfig, CYMATICS_CONFIG_FIELDS)` → `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RippleTankConfig, RIPPLE_TANK_CONFIG_FIELDS)`
- In `EFFECT_CONFIG_FIELDS` macro: change `X(cymatics)` → `X(rippleTank)`
- Add backwards-compatible deserialization: in `from_json` for `EffectConfig`, after the `EFFECT_CONFIG_FIELDS(DESERIALIZE_EFFECT)` block, add: `if (j.contains("cymatics")) { e.rippleTank = j["cymatics"].get<RippleTankConfig>(); }`

**Verify**: Compiles. Old presets with `"cymatics"` key still load correctly.

---

#### Task 2.6: Update preset

**Files**: `presets/CYMATICBOB.json` (modify)

**Do**: Rename JSON key `"cymatics"` → `"rippleTank"`. Content stays identical.

**Verify**: JSON is valid.

---

### Wave 3: Cleanup

#### Task 3.1: Delete old files

**Files**: `src/effects/cymatics.h` (delete), `src/effects/cymatics.cpp` (delete), `shaders/cymatics.fs` (delete)

**Do**: Delete the three old files. Update `src/main.cpp` line 239 comment: `"cymatics"` → `"ripple tank"`. Update `src/analysis/analysis_pipeline.h` comments referencing "cymatics" → "ripple tank". Update `src/ui/ui_units.h` comment referencing `"cymatics.lissajous"` → `"rippleTank.lissajous"`.

**Verify**: `cmake.exe --build build` compiles with no errors. No remaining references to "cymatics" in `.h`, `.cpp`, or `.fs` files (excluding `docs/` and `presets/` migration fallback).

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] No source references to `Cymatics`/`cymatics`/`CYMATICS` remain (excluding docs, research, and the `"cymatics"` fallback in effect_serialization.cpp)
- [ ] CYMATICBOB preset loads correctly
- [ ] Effect appears as "Ripple Tank" in UI under section 13 (Field)
