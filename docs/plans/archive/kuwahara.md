# Kuwahara Filter

Edge-preserving smoothing that flattens color regions while maintaining hard boundaries. Produces posterized cell-like shapes distinct from Oil Paint's textured brush strokes. Single-pass Style transform using 4-sector basic mode.

## Current State

- `src/config/oil_paint_config.h` — Existing Oil Paint uses Kuwahara internally but adds brush stroke geometry and specular lighting on top
- `src/render/shader_setup.cpp:57` — `TRANSFORM_OIL_PAINT` dispatch pattern to follow
- `src/ui/imgui_effects_style.cpp:124` — Style effect UI pattern (DrawStyleOilPaint)
- `src/automation/param_registry.cpp:128` — Oil Paint param registration pattern
- `docs/research/kuwahara.md` — Complete algorithm reference with both modes
- Git commit `2bfc7bb` — Previously working 4-sector shader in this codebase

## Technical Implementation

- **Source**: [lettier/3d-game-shaders-for-beginners](https://github.com/lettier/3d-game-shaders-for-beginners/blob/master/demonstration/shaders/fragment/kuwahara-filter.frag) (basic), [Godot Shaders generalized Kuwahara](https://godotshaders.com/shader/generalized-kuwahara/) (generalized)
- **Core algorithm**: Divide pixel neighborhood into sectors, compute mean color and luminance variance per sector, output the mean of the sector with lowest variance (basic) or soft-blend sectors weighted by inverse variance (generalized)

### Basic Mode (4 Sectors)

Four overlapping square quadrants centered on current pixel. For each sector: accumulate `colorSum` and `colorSqSum`, compute `mean = colorSum / count`, `variance = dot((colorSqSum/count - mean*mean), vec3(0.299, 0.587, 0.114))`. Output `mean[minVarianceIdx]`.

Sector bounds:
```
sector 0: x in [-radius, 0], y in [-radius, 0]
sector 1: x in [0, radius],  y in [-radius, 0]
sector 2: x in [-radius, 0], y in [0, radius]
sector 3: x in [0, radius],  y in [0, radius]
```

### Generalized Mode (8 Circular Sectors)

8 wedge-shaped sectors at 45-degree intervals. Each sample weighted by:
- **Spatial Gaussian**: `g = exp(-3.125 * dot(offset/radius, offset/radius))`
- **Sector membership**: `sectorWeight = pow(max(0.0, dot(normalize(offset + 0.001), sectorDir)), sharpness)`
- **Combined**: `w = g * sectorWeight`

Sector blending uses soft inverse-variance weighting instead of hard min:
```
weight_k = 1.0 / (1.0 + pow(variance_k * 1000.0, 0.5 * hardness))
finalColor = sum(mean_k * weight_k) / sum(weight_k)
```

Samples beyond `radius` distance are discarded (`if (dist > float(radius)) continue`).

### Parameters

| Uniform | Type | Range | Default | Notes |
|---------|------|-------|---------|-------|
| radius | int | 2-12 | 4 | Kernel radius. Performance scales with radius^2 |
| quality | int | 0-1 | 0 | 0 = basic (4-sector), 1 = generalized (8-sector) |
| sharpness | float | 1.0-18.0 | 8.0 | Sector directional selectivity (generalized only) |
| hardness | float | 1.0-100.0 | 8.0 | Inverse-variance blending aggressiveness (generalized only) |

For modulation, `radius` is stored as float in config (cast to int in shader) to allow smooth interpolation.

---

## Phase 1: Config and Registration

**Goal**: Kuwahara appears in the transform enum and config system.
**Depends on**: —
**Files**: `src/config/kuwahara_config.h`, `src/config/effect_config.h`

**Build**:
- Create `src/config/kuwahara_config.h` with `KuwaharaConfig` struct: `enabled` (bool, false), `radius` (float, 4.0f), `quality` (int, 0), `sharpness` (float, 8.0f), `hardness` (float, 8.0f)
- In `effect_config.h`: add `#include "kuwahara_config.h"`, add `TRANSFORM_KUWAHARA` enum entry before `TRANSFORM_EFFECT_COUNT`, add name case returning `"Kuwahara"`, add to `TransformOrderConfig::order` array, add `KuwaharaConfig kuwahara;` member to `EffectConfig`, add `IsTransformEnabled` case

**Verify**: `cmake.exe --build build` compiles without errors.

**Done when**: `TRANSFORM_KUWAHARA` exists in the enum and `EffectConfig` holds a `KuwaharaConfig`.

---

## Phase 2: Shader

**Goal**: Working Kuwahara fragment shader with both modes.
**Depends on**: —
**Files**: `shaders/kuwahara.fs`

**Build**:
- Create `shaders/kuwahara.fs` with uniforms: `texture0` (sampler2D), `resolution` (vec2), `radius` (int), `quality` (int), `sharpness` (float), `hardness` (float)
- Implement basic mode (quality == 0): 4-sector loop per the algorithm above
- Implement generalized mode (quality == 1): 8-sector Gaussian-weighted loop with soft blending
- Branch on `quality` uniform

**Verify**: Shader file parses (verified at runtime in Phase 3).

**Done when**: `shaders/kuwahara.fs` contains both algorithm implementations matching the research doc.

---

## Phase 3: PostEffect Integration

**Goal**: Shader loads, uniform locations resolved, renders in the transform chain.
**Depends on**: Phase 1, Phase 2
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`, `src/render/shader_setup.h`, `src/render/shader_setup.cpp`

**Build**:
- In `post_effect.h`: add `Shader kuwaharaShader;`, add uniform location ints: `kuwaharaResolutionLoc`, `kuwaharaRadiusLoc`, `kuwaharaQualityLoc`, `kuwaharaSharpnessLoc`, `kuwaharaHardnessLoc`
- In `post_effect.cpp`: load shader, add to success check, get uniform locations, set resolution uniform, unload in Uninit
- In `shader_setup.h`: declare `void SetupKuwahara(PostEffect* pe);`
- In `shader_setup.cpp`: add `TRANSFORM_KUWAHARA` case in `GetTransformEffect()` returning `{ &pe->kuwaharaShader, SetupKuwahara, &pe->effects.kuwahara.enabled }`. Implement `SetupKuwahara` — cast `radius` float to int, set all uniforms

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` — enable Kuwahara, image flattens into posterized cells.

**Done when**: Kuwahara renders correctly in the transform chain with adjustable parameters.

---

## Phase 4: UI Panel

**Goal**: Kuwahara controls appear in the Style category panel.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_style.cpp`

**Build**:
- In `imgui_effects.cpp`: add `case TRANSFORM_KUWAHARA:` to Style section (section 4) in `GetTransformCategory()`
- In `imgui_effects_style.cpp`: add `static bool sectionKuwahara = false;`, implement `DrawStyleKuwahara()` with Enabled checkbox, `ModulatableSlider` for radius (cast display to int format "%.0f"), combo/radio for quality (Basic/Generalized), conditional `ModulatableSlider` for sharpness and hardness (only when quality == 1). Call from `DrawStyleCategory()` with spacing.

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Style panel shows Kuwahara section with all controls.

**Done when**: UI controls modify Kuwahara parameters in real-time, generalized-only params hidden when quality is basic.

---

## Phase 5: Serialization and Modulation

**Goal**: Presets save/load Kuwahara config; radius, sharpness, hardness respond to modulation.
**Depends on**: Phase 1
**Files**: `src/config/preset.cpp`, `src/automation/param_registry.cpp`

**Build**:
- In `preset.cpp`: add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KuwaharaConfig, enabled, radius, quality, sharpness, hardness)`, add `to_json` line (`if (e.kuwahara.enabled) { j["kuwahara"] = e.kuwahara; }`), add `from_json` line
- In `param_registry.cpp`: add three entries to `PARAM_TABLE` — `"kuwahara.radius"` {2.0f, 12.0f}, `"kuwahara.sharpness"` {1.0f, 18.0f}, `"kuwahara.hardness"` {1.0f, 100.0f}. Add matching pointers to `targets[]` — `&effects->kuwahara.radius`, `&effects->kuwahara.sharpness`, `&effects->kuwahara.hardness`

**Verify**: Save preset with Kuwahara enabled → reload → settings persist. Assign LFO to radius → radius oscillates.

**Done when**: Preset round-trip works and all three parameters respond to modulation sources.

---

## Implementation Notes

- **Generalized mode removed**: The 8-sector Gaussian-weighted mode (`quality == 1`) produced black artifacts across the image and hurt performance due to the O(radius^2 * 8) sample count. Stripped `quality`, `sharpness`, and `hardness` from config, shader, uniforms, UI, serialization, and param registry. Only the basic 4-sector mode ships.
