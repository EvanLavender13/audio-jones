# Chromatic Aberration Rework

Move chromatic aberration from the hardcoded Output pipeline slot into the Optical transform category. Upgrade the shader from a fixed 3-channel RGB split to a spectral multi-sample loop where sample count is runtime-adjustable: 3 samples reproduces the old hard RGB bands, higher counts produce smooth prismatic rainbow spreading. All three parameters are modulatable. Batch-update presets to the new config format.

**Research**: `docs/research/chromatic-aberration-rework.md`

## Design

### Types

**`ChromaticAberrationConfig`** (in `src/effects/chromatic_aberration.h`):

```
bool enabled = false;
float offset = 0.0f;    // Channel separation in pixels (0-50)
float samples = 3.0f;   // Spectral sample count (3-24), float for modulation
float falloff = 1.0f;   // Distance curve exponent (0.5-3.0)
```

Fields macro: `CHROMATIC_ABERRATION_CONFIG_FIELDS enabled, offset, samples, falloff`

**`ChromaticAberrationEffect`** (in `src/effects/chromatic_aberration.h`):

```
Shader shader;
int resolutionLoc;
int offsetLoc;
int samplesLoc;
int falloffLoc;
```

### Algorithm

The shader adapts the GM Shaders spectral multi-sample approach to our radial setup. Key differences from the reference: radial direction from center instead of linear offset, `pow(dist, falloff)` distance curve instead of exponential, no gamma square/sqrt (our pipeline handles gamma separately), and a `* 2.0` multiplier on the loop position so that 3 samples at the old `offset` value produce the same visual spread as the old 3-channel shader.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float offset;
uniform int samples;
uniform float falloff;

void main() {
    vec2 center = vec2(0.5);
    vec2 toCenter = fragTexCoord - center;
    float dist = length(toCenter);

    // Stable radial direction with center fade to avoid direction instability
    vec2 dir = dist > 0.001 ? toCenter / dist : vec2(0.0);
    float intensity = smoothstep(0.0, 0.15, dist) * pow(dist, falloff);
    float minRes = min(resolution.x, resolution.y);

    vec3 colorSum = vec3(0.0);
    vec3 weightSum = vec3(0.0);

    for (int i = 0; i < samples; i++) {
        float t = float(i) / float(samples - 1);
        vec2 sampleOffset = dir * offset * intensity * (t - 0.5) * 2.0 / minRes;
        vec3 color = texture(texture0, fragTexCoord + sampleOffset).rgb;

        // Spectral weights: R peaks at t=1, G peaks at t=0.5, B peaks at t=0
        vec3 weight = vec3(t, 1.0 - abs(2.0 * t - 1.0), 1.0 - t);

        colorSum += color * weight;
        weightSum += weight;
    }

    finalColor = vec4(colorSum / weightSum, 1.0);
}
```

At 3 samples: t = {0, 0.5, 1.0}, weights = {(0,0,1), (0.5,1,0.5), (1,0,0)}. Blue comes from -offset, green from center, red from +offset — matching the old 3-channel split. At higher counts, intermediate spectral colors fill in for smooth prismatic spreading.

The `* 2.0` multiplier ensures `(t - 0.5) * 2.0` ranges from -1.0 to +1.0, so `offset = 15` displaces R and B by 15 pixels — same visual strength as the old shader where R was sampled at `+offset` and B at `-offset`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `offset` | float | 0-50 | 0.0 | Yes | `"Offset"` |
| `samples` | float | 3-24 | 3.0 | Yes | `"Samples"` |
| `falloff` | float | 0.5-3.0 | 1.0 | Yes | `"Falloff"` |

- `offset` uses `ModulatableSlider` with `"%.0f px"` format
- `samples` uses `ModulatableSliderInt` (stored as float, displayed as int)
- `falloff` uses `ModulatableSlider` with `"%.2f"` format

### Constants

- Enum: `TRANSFORM_CHROMATIC_ABERRATION`
- Display name: `"Chromatic Aberration"`
- Badge: `"OPT"`
- Section index: `7` (Optical)
- Flags: `EFFECT_FLAG_NONE`
- Macro: `REGISTER_EFFECT`

### Setup Notes

- Setup casts `cfg->samples` to `int` before passing as `SHADER_UNIFORM_INT`
- Resolution set per frame via `GetScreenWidth()`/`GetScreenHeight()` (same as bokeh)
- Bridge function `SetupChromaticAberration(PostEffect*)` is NOT `static`

### Removal Targets

Old hardcoded chromatic code to remove:

| File | What to remove |
|------|---------------|
| `src/render/post_effect.h` | `Shader chromaticShader;`, `int chromaticResolutionLoc;`, `int chromaticOffsetLoc;` |
| `src/render/post_effect.cpp` | `LoadShader("shaders/chromatic.fs")`, location caching, `UnloadShader(pe->chromaticShader)`, resolution uniform set in `SetResolutionUniforms`, `chromaticShader.id` from init success check |
| `src/render/shader_setup.cpp` | `SetupChromatic()` function |
| `src/render/render_pipeline.cpp` | Hardcoded `RenderPass(..., pe->chromaticShader, SetupChromatic)` call and its comment |
| `src/ui/imgui_effects.cpp` | `ModulatableSlider("Chroma", ...)` in OUTPUT section |
| `src/config/effect_config.h` | `float chromaticOffset = 0.0f;` field |
| `src/automation/param_registry.cpp` | `"effects.chromaticOffset"` entry from `PARAM_TABLE` |
| `src/config/effect_serialization.cpp` | `chromaticOffset` lines in both `to_json` and `from_json` |
| `shaders/chromatic.fs` | Delete entire file |

### Preset Migration

Batch-update all 25 preset files. Three categories:

**Nonzero offset** (GRAYBOB): Remove `"chromaticOffset": 15.0`, add `"chromaticAberration": {"enabled": true, "offset": 15.0}`.

**Zero offset with LFO route** (SOUPB, SOLO, WOBBYBOB, SOUPA, JOSHIN, YUPP, RINGER, BINGBANG, GLITCHYBOB, CYMATICBOB, ICEY): Remove `"chromaticOffset": 0.0`, add `"chromaticAberration": {"enabled": true, "offset": 0.0}`, change LFO `"paramId": "effects.chromaticOffset"` to `"paramId": "chromaticAberration.offset"`.

**Zero offset, no LFO route** (all others): Remove `"chromaticOffset": 0.0` line only.

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create chromatic aberration header

**Files**: `src/effects/chromatic_aberration.h` (create)
**Creates**: Config struct and Effect struct that all other tasks include

**Do**: Create the header following the Design section types. Follow `src/effects/bokeh.h` as the pattern — include guard, `raylib.h`, `<stdbool.h>`, config struct with defaults and range comments, `CONFIG_FIELDS` macro, Effect typedef struct, and 5 function declarations (`Init`, `Setup`, `Uninit`, `ConfigDefault`, `RegisterParams`). Setup takes `(Effect*, const Config*)` — no deltaTime needed (no animation accumulator).

**Verify**: File exists and is well-formed.

---

### Wave 2: Implementation + Integration + Removal

#### Task 2.1: Create effect module

**Files**: `src/effects/chromatic_aberration.cpp` (create)

**Do**: Create the effect module following `src/effects/bokeh.cpp` as the pattern.

Include order: own header, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_descriptor.h`, `render/post_effect.h`, `imgui.h`, `ui/modulatable_slider.h`, `<stddef.h>`.

Functions:
- `Init`: Load `shaders/chromatic_aberration.fs`, cache 4 uniform locations (`resolution`, `offset`, `samples`, `falloff`). Return false if shader.id == 0.
- `Setup`: Set resolution from `GetScreenWidth()`/`GetScreenHeight()`. Set offset and falloff as `SHADER_UNIFORM_FLOAT`. Cast `cfg->samples` to `int` and set as `SHADER_UNIFORM_INT`.
- `Uninit`: Unload shader.
- `ConfigDefault`: Return `ChromaticAberrationConfig{}`.
- `RegisterParams`: Register all 3 params — `"chromaticAberration.offset"` (0, 50), `"chromaticAberration.samples"` (3, 24), `"chromaticAberration.falloff"` (0.5, 3).

UI section (`// === UI ===`): `DrawChromaticAberrationParams(EffectConfig*, const ModSources*, ImU32)` with 3 sliders:
- `ModulatableSlider("Offset##chromaticAberration", ..., "chromaticAberration.offset", "%.0f px", ms)`
- `ModulatableSliderInt("Samples##chromaticAberration", ..., "chromaticAberration.samples", ms)` range 3-24
- `ModulatableSlider("Falloff##chromaticAberration", ..., "chromaticAberration.falloff", "%.2f", ms)`

Bridge function (NOT static): `void SetupChromaticAberration(PostEffect* pe)` delegates to `ChromaticAberrationEffectSetup`.

Registration macro at bottom:
```
REGISTER_EFFECT(TRANSFORM_CHROMATIC_ABERRATION, ChromaticAberration, chromaticAberration,
                "Chromatic Aberration", "OPT", 7, EFFECT_FLAG_NONE,
                SetupChromaticAberration, NULL, DrawChromaticAberrationParams)
```

**Verify**: Compiles (after all Wave 2 tasks complete).

---

#### Task 2.2: Create shader

**Files**: `shaders/chromatic_aberration.fs` (create)

**Do**: Implement the Algorithm section from the Design above. Copy the GLSL verbatim. Uniforms: `sampler2D texture0`, `vec2 resolution`, `float offset`, `int samples`, `float falloff`.

**Verify**: File exists. Shader correctness verified at runtime.

---

#### Task 2.3: Config and build integration

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)

**Do**:

`src/config/effect_config.h`:
- Add `#include "effects/chromatic_aberration.h"` with other effect includes
- Add `TRANSFORM_CHROMATIC_ABERRATION,` before `TRANSFORM_EFFECT_COUNT` in the enum
- Add `TRANSFORM_CHROMATIC_ABERRATION` at end of `TransformOrderConfig::order` initialization
- Add `ChromaticAberrationConfig chromaticAberration;` member to `EffectConfig`
- Remove `float chromaticOffset = 0.0f;` field from `EffectConfig`

`src/render/post_effect.h`:
- Add `#include "effects/chromatic_aberration.h"` with other effect includes
- Add `ChromaticAberrationEffect chromaticAberration;` member to `PostEffect`
- Remove `Shader chromaticShader;`, `int chromaticResolutionLoc;`, `int chromaticOffsetLoc;`

`CMakeLists.txt`:
- Add `src/effects/chromatic_aberration.cpp` to `EFFECTS_SOURCES`

`src/config/effect_serialization.cpp`:
- Add `#include "effects/chromatic_aberration.h"` with other effect includes
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ChromaticAberrationConfig, CHROMATIC_ABERRATION_CONFIG_FIELDS)`
- Add `X(chromaticAberration) \` to `EFFECT_CONFIG_FIELDS` X-macro
- Remove `j["chromaticOffset"] = e.chromaticOffset;` from `to_json`
- Remove `e.chromaticOffset = j.value("chromaticOffset", e.chromaticOffset);` from `from_json`

**Verify**: Compiles (after all Wave 2 tasks complete).

---

#### Task 2.4: Remove old chromatic code

**Files**: `src/render/post_effect.cpp` (modify), `src/render/shader_setup.cpp` (modify), `src/render/render_pipeline.cpp` (modify), `src/ui/imgui_effects.cpp` (modify), `src/automation/param_registry.cpp` (modify)
**Also**: Delete `shaders/chromatic.fs`

**Do**:

`src/render/post_effect.cpp`:
- Remove `pe->chromaticShader = LoadShader(0, "shaders/chromatic.fs");`
- Remove `pe->chromaticShader.id` from the init success `&&` chain
- Remove `chromaticResolutionLoc` and `chromaticOffsetLoc` caching from `PostEffectCacheLocations`
- Remove `SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, ...)` from `SetResolutionUniforms`
- Remove `UnloadShader(pe->chromaticShader);` from `PostEffectUninit`

`src/render/shader_setup.cpp`:
- Remove the entire `SetupChromatic()` function

`src/render/render_pipeline.cpp`:
- Remove the `// Chromatic aberration before transforms` comment and `RenderPass(pe, src, ..., pe->chromaticShader, SetupChromatic)` call and the `src = ...` / `writeIdx = ...` lines that follow it
- Update the comment on `RenderPipelineApplyOutput` in `render_pipeline.h` to remove "chromatic" from the description

`src/ui/imgui_effects.cpp`:
- Remove `ModulatableSlider("Chroma", &e->chromaticOffset, "effects.chromaticOffset", ...)` from the OUTPUT section

`src/automation/param_registry.cpp`:
- Remove the `{"effects.chromaticOffset", {0.0f, 50.0f}, offsetof(EffectConfig, chromaticOffset)}` entry from `PARAM_TABLE`

Delete `shaders/chromatic.fs`.

**Verify**: Compiles (after all Wave 2 tasks complete).

---

### Wave 3: Preset Migration

#### Task 3.1: Batch-update preset files

**Files**: All 25 files in `presets/` (modify)
**Depends on**: Wave 2 complete (new config format finalized)

**Do**: Update each preset JSON file per the Preset Migration section in the Design above. Three categories:

1. **GRAYBOB**: Remove `"chromaticOffset": 15.0` line, add `"chromaticAberration": {"enabled": true, "offset": 15.0}` in the effects section (alongside other effect configs like `bloom`, `bokeh`, etc.).

2. **SOUPB, SOLO, WOBBYBOB, SOUPA, JOSHIN, YUPP, RINGER, BINGBANG, GLITCHYBOB, CYMATICBOB, ICEY**: Remove `"chromaticOffset": 0.0` line. Add `"chromaticAberration": {"enabled": true, "offset": 0.0}`. In the `lfoRoutes` array, change `"paramId": "effects.chromaticOffset"` to `"paramId": "chromaticAberration.offset"`.

3. **LINKY, ZOOP, FLOWBIUS, SMOOTHBOB, WADDAFA, SPIROL, STAYINNIT, WINNY, FLOOW, GALACTO, BUZZBUZZ, HEXOS, AIZAAWWAA, OOPY**: Remove `"chromaticOffset": 0.0` line only.

**Verify**: All JSON files parse correctly (no trailing commas, valid JSON). `grep -r chromaticOffset presets/` returns zero matches.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Optical section of transform panel
- [ ] Effect shows "OPT" badge
- [ ] Effect can be enabled, reordered, disabled
- [ ] Offset slider moves RGB channels apart radially
- [ ] Samples=3 produces hard RGB bands (old look)
- [ ] Samples=12+ produces smooth prismatic rainbow
- [ ] Falloff=1 is linear, falloff=2+ concentrates at edges
- [ ] GRAYBOB preset loads with chromatic aberration enabled at offset 15
- [ ] Presets with LFO routes correctly modulate `chromaticAberration.offset`
- [ ] Old "Chroma" slider is gone from OUTPUT section
- [ ] `shaders/chromatic.fs` no longer exists
