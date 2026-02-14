# Preset Serialization Split

Split `preset.cpp` (1143 lines) into two files and collapse 73 mechanical effect serialization lines using an X-macro.

**Research**: `docs/research/preset-split.md`

## Design

### File Split

| File | Contents | ~Lines |
|------|----------|--------|
| `src/config/effect_serialization.cpp` | All NLOHMANN macros (Color→Color, DualLissajous→AttractorLines), ColorConfig custom to_json/from_json, TransformOrder helpers, EffectConfig to_json/from_json (with X-macro) | ~550 |
| `src/config/effect_serialization.h` | Forward-declares `to_json`/`from_json` for EffectConfig | ~15 |
| `src/config/preset.cpp` (trimmed) | Audio/Drawable/LFO/Preset NLOHMANN macros, Drawable to_json/from_json, Preset to_json/from_json, file I/O, AppConfigs conversion | ~215 |

### What Moves to `effect_serialization.cpp`

Everything from current `preset.cpp` lines 1–927:

1. **Includes** needed by moved code: `preset.h`, `config/dual_lissajous_config.h`, `config/effect_descriptor.h`, `render/gradient.h`, `<algorithm>`, `<cstring>`, `<nlohmann/json.hpp>`
2. **NLOHMANN macros** (lines 18–509): `Color`, `GradientStop`, `DualLissajousConfig`, and all 70+ per-config macros through `AttractorLinesConfig`
3. **ColorConfig** custom `to_json`/`from_json` (lines 21–137) — stays hand-written
4. **TransformOrder** helpers: `TransformEffectFromName`, `TransformOrderToJson`, `TransformOrderFromJson` (lines 511–570)
5. **EffectConfig** `to_json`/`from_json` (lines 572–927) — rewritten with X-macro

### What Stays in `preset.cpp`

Everything from current `preset.cpp` lines 928–1143:

1. **Includes**: `preset.h`, `app_configs.h`, `automation/drawable_params.h`, `config/effect_serialization.h`, `render/drawable.h`, `ui/imgui_panels.h`, `<cstring>`, `<filesystem>`, `<fstream>`, `<nlohmann/json.hpp>`
2. **NLOHMANN macros**: `AudioConfig`, `DrawableBase`, `WaveformData`, `SpectrumData`, `ShapeData`, `ParametricTrailData`, `LFOConfig` (lines 928–948)
3. **Drawable** custom `to_json`/`from_json` (lines 950–999)
4. **Preset** `to_json`/`from_json`, `PresetDefault`, `PresetSave`, `PresetLoad`, `PresetListFiles`, `PresetFromAppConfigs`, `PresetToAppConfigs` (lines 1001–1143)

### X-Macro Table

Define `EFFECT_CONFIG_FIELDS(X)` listing all 82 effect config field names that have an `.enabled` guard:

```
sineWarp, kaleidoscope, voronoi, physarum, curlFlow, curlAdvection,
attractorFlow, boids, cymatics, infiniteZoom, interferenceWarp,
radialStreak, relativisticDoppler, textureWarp, waveRipple, mobius,
pixelation, glitch, poincareDisk, toon, heightfieldRelief, gradientFlow,
drosteZoom, kifs, latticeFold, multiScaleGrid, colorGrade, asciiArt,
oilPaint, watercolor, neonGlow, radialPulse, falseColor, halftone,
dotMatrix, chladniWarp, corridorWarp, crossHatching, crt,
paletteQuantization, bokeh, bloom, anamorphicStreak, mandelbox,
triangleFold, radialIfs, domainWarp, phyllotaxis, densityWaveSpiral,
moireInterference, pencilSketch, matrixRain, impressionist, kuwahara,
legoBricks, inkWash, discoBall, particleLife, surfaceWarp, shake,
circuitBoard, synthwave, constellation, plasma, interference, solidColor,
toneWarp, scanBars, pitchSpiral, spectralArcs, moireGenerator, muons,
filaments, slashes, glyphField, arcStrobe, signalFrames, nebula,
motherboard, attractorLines, phiBlur
```

Expansion in `to_json`: `if (e.name.enabled) j[#name] = e.name;`
Expansion in `from_json`: `e.name = j.value(#name, e.name);`

Non-effect fields (`halfLife`, `blurScale`, `chromaticOffset`, `feedbackDesaturate`, `motionScale`, `flowField`, `feedbackFlow`, `proceduralWarp`, `gamma`, `clarity`) and `transformOrder` stay as individual lines — no X-macro.

### Header

`effect_serialization.h` declares just what `preset.cpp` needs:

```cpp
#ifndef EFFECT_SERIALIZATION_H
#define EFFECT_SERIALIZATION_H

#include <nlohmann/json_fwd.hpp>
#include "effect_config.h"

void to_json(nlohmann::json &j, const EffectConfig &e);
void from_json(const nlohmann::json &j, EffectConfig &e);

#endif
```

Note: `to_json`/`from_json` must NOT be `static` — they need external linkage for `preset.cpp` to find them via ADL.

### Build System

Add `src/config/effect_serialization.cpp` to `CONFIG_SOURCES` in `CMakeLists.txt`.

---

## Tasks

### Wave 1: Create header + new source file

#### Task 1.1: Create `effect_serialization.h`

**Files**: `src/config/effect_serialization.h` (create)
**Creates**: Header that `preset.cpp` and `effect_serialization.cpp` both include.

**Do**: Create the header with include guard, forward-declared `to_json`/`from_json` for `EffectConfig` as specified in the Design section.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker impact yet).

#### Task 1.2: Create `effect_serialization.cpp`

**Files**: `src/config/effect_serialization.cpp` (create)
**Creates**: All effect-related serialization code moved from `preset.cpp`.

**Do**:
1. Move lines 1–927 of current `preset.cpp` into this file (all NLOHMANN macros, ColorConfig custom serialization, TransformOrder helpers, EffectConfig to_json/from_json).
2. Include `effect_serialization.h` instead of having `static` linkage on the EffectConfig functions.
3. Replace the 82 `if (e.X.enabled) j["X"] = e.X;` lines in `to_json` and the 82 `e.X = j.value("X", e.X);` lines in `from_json` with the `EFFECT_CONFIG_FIELDS(X)` X-macro as specified in the Design section.
4. Keep the `NOLINTNEXTLINE` comments only if needed (the X-macro makes the functions much shorter, so they can likely be removed).
5. The ColorConfig `to_json`/`from_json` stays `static` (only used within this file by the NLOHMANN macros that reference it). The EffectConfig `to_json`/`from_json` must NOT be `static`.

**Verify**: File exists with correct content. Build verification happens after Task 1.3.

#### Task 1.3: Trim `preset.cpp` and update CMakeLists.txt

**Files**: `src/config/preset.cpp` (modify), `CMakeLists.txt` (modify)
**Depends on**: Tasks 1.1 and 1.2 complete.

**Do**:
1. Remove lines 1–927 from `preset.cpp` (everything before the `AudioConfig` NLOHMANN macro).
2. Update includes: keep `preset.h`, `app_configs.h`, `automation/drawable_params.h`, `render/drawable.h`, `ui/imgui_panels.h`, `<cstring>`, `<filesystem>`, `<fstream>`, `<nlohmann/json.hpp>`. Add `config/effect_serialization.h`. Remove includes only needed by moved code (`config/dual_lissajous_config.h`, `config/effect_descriptor.h`, `render/gradient.h`, `<algorithm>`).
3. Add `src/config/effect_serialization.cpp` to `CONFIG_SOURCES` in `CMakeLists.txt`.

**Verify**: `cmake.exe --build build` compiles with no errors or warnings. Load a preset file and verify JSON output is byte-identical (save → compare).

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `preset.cpp` is ~215 lines (down from 1143)
- [ ] `effect_serialization.cpp` is ~550 lines
- [ ] Loading any existing preset produces identical behavior
- [ ] Saving a preset produces byte-identical JSON output
