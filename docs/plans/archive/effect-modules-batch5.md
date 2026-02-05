# Effect Modules Migration — Batch 5 (Artistic)

Migrate 6 artistic effects from monolithic PostEffect into self-contained modules: oil_paint, watercolor, impressionist, ink_wash, pencil_sketch, cross_hatching.

**Research**: [docs/research/effect-modules.md](../research/effect-modules.md)

## Design

### Module Pattern

Each effect becomes `src/effects/{name}.h` + `src/effects/{name}.cpp` containing:
- Config struct (moved from `src/config/{name}_config.h`, deleted after)
- Effect struct (shader handles, uniform locations, animation state)
- Lifecycle functions: `Init`, `Setup`, `Uninit`, `ConfigDefault`

Follow the same pattern as Batch 4 effects (e.g., `shake.h`/`shake.cpp`).

### Special Cases

**Oil Paint** — owns TWO shaders (`oil_paint_stroke.fs` for brush pass, `oil_paint.fs` for specular composite), a noise texture, and an intermediate render texture. The `OilPaintEffect` struct holds all of these. `ApplyHalfResOilPaint` in `shader_setup.cpp` updates to reference `pe->oilPaint.*` fields instead of flat `pe->` fields.

**Pencil Sketch** — has a wobble time accumulator (`pencilSketchWobbleTime` on PostEffect). Moves into `PencilSketchEffect.wobbleTime`. Setup function accumulates: `e->wobbleTime += cfg->wobbleSpeed * deltaTime`.

**Cross Hatching** — has a time accumulator (`crossHatchingTime` on PostEffect). Moves into `CrossHatchingEffect.time`. Setup function accumulates: `e->time += deltaTime`.

**Impressionist & Watercolor** — listed in `HALF_RES_EFFECTS[]` in `render_pipeline.cpp`. No changes to that array; `IsHalfResEffect` and `ApplyHalfResEffect` continue to work because they use the shader from `GetTransformEffect` which the module provides.

**Ink Wash** — `softness` is `float` in config but cast to `int` before `SetShaderValue`. The module Setup replicates this cast.

### Watercolor Preset Fix

The existing NLOHMANN macro omits `edgePool`, `flowCenter`, `flowWidth`. Add them so presets serialize all parameters.

### Uniform Names (verified against .fs files)

| Effect | Shader | Uniforms |
|--------|--------|----------|
| oil_paint_stroke | oil_paint_stroke.fs | resolution, brushSize, strokeBend, layers, texture1 (noise) |
| oil_paint | oil_paint.fs | resolution, specular |
| watercolor | watercolor.fs | resolution, samples, strokeStep, washStrength, paperScale, paperStrength, edgePool, flowCenter, flowWidth |
| impressionist | impressionist.fs | resolution, splatCount, splatSizeMin, splatSizeMax, strokeFreq, strokeOpacity, outlineStrength, edgeStrength, edgeMaxDarken, grainScale, grainAmount, exposure |
| ink_wash | ink_wash.fs | resolution, strength, granulation, bleedStrength, bleedRadius, softness |
| pencil_sketch | pencil_sketch.fs | resolution, angleCount, sampleCount, strokeFalloff, gradientEps, paperStrength, vignetteStrength, wobbleTime, wobbleAmount |
| cross_hatching | cross_hatching.fs | resolution, time, width, threshold, noise, outline |

---

## Tasks

### Wave 1: Effect Modules (6 parallel tasks)

All tasks create NEW files only. No file overlap between tasks.

#### Task 1.1: Oil Paint Module

**Files**: `src/effects/oil_paint.h` (create), `src/effects/oil_paint.cpp` (create)

**Do**:
- Move `OilPaintConfig` from `src/config/oil_paint_config.h` into header. Keep all fields and defaults.
- Define `OilPaintEffect` struct with: `strokeShader` (Shader), `compositeShader` (Shader), `noiseTex` (Texture2D), `intermediate` (RenderTexture2D), plus all uniform location ints for BOTH shaders: `strokeResolutionLoc`, `brushSizeLoc`, `strokeBendLoc`, `layersLoc`, `noiseTexLoc` (for stroke shader), `compositeResolutionLoc`, `specularLoc` (for composite shader).
- `OilPaintEffectInit`: Load both shaders (`oil_paint_stroke.fs`, `oil_paint.fs`). Cache all uniform locations. Generate noise texture (white noise image, same as current `post_effect.cpp` lines ~970-978). Create intermediate HDR render texture via `RenderUtilsInitTextureHDR`. Return false if either shader fails (check `.id == 0`).
- `OilPaintEffectSetup`: Set uniforms on composite shader (specular). The stroke shader uniforms are set by `ApplyHalfResOilPaint` since it needs resolution override for half-res.
- `OilPaintEffectUninit`: Unload both shaders, noise texture, intermediate RT.
- `OilPaintEffectResize(OilPaintEffect* e, int width, int height)`: Additional function — recreate intermediate RT at new dimensions. Called from `PostEffectResize`.
- `OilPaintConfigDefault`: Return default-initialized `OilPaintConfig{}`.
- Follow `shake.h`/`shake.cpp` as structural reference.

**Verify**: File compiles in isolation (`#include` correctness).

#### Task 1.2: Watercolor Module

**Files**: `src/effects/watercolor.h` (create), `src/effects/watercolor.cpp` (create)

**Do**:
- Move `WatercolorConfig` from `src/config/watercolor_config.h`. Keep all fields and defaults.
- Define `WatercolorEffect` struct: `shader` + all 9 uniform location ints (resolution, samples, strokeStep, washStrength, paperScale, paperStrength, edgePool, flowCenter, flowWidth).
- `WatercolorEffectInit`: Load shader (`watercolor.fs`), cache all uniform locations. Return false if `shader.id == 0`.
- `WatercolorEffectSetup`: Set all uniforms from config. Resolution set via `GetScreenWidth()`/`GetScreenHeight()` (note: `ApplyHalfResEffect` overrides resolution before calling setup, so this is the full-res fallback).
- `WatercolorEffectUninit`: Unload shader.
- `WatercolorConfigDefault`: Return default `WatercolorConfig{}`.
- Follow `shake.h`/`shake.cpp` as structural reference.

**Verify**: File compiles in isolation.

#### Task 1.3: Impressionist Module

**Files**: `src/effects/impressionist.h` (create), `src/effects/impressionist.cpp` (create)

**Do**:
- Move `ImpressionistConfig` from `src/config/impressionist_config.h`. Keep all fields and defaults.
- Define `ImpressionistEffect` struct: `shader` + all 12 uniform location ints (resolution, splatCount, splatSizeMin, splatSizeMax, strokeFreq, strokeOpacity, outlineStrength, edgeStrength, edgeMaxDarken, grainScale, grainAmount, exposure).
- `ImpressionistEffectInit`: Load shader (`impressionist.fs`), cache all uniform locations. Return false if `shader.id == 0`.
- `ImpressionistEffectSetup`: Set all uniforms from config. Set resolution from `GetScreenWidth()`/`GetScreenHeight()`.
- `ImpressionistEffectUninit`: Unload shader.
- `ImpressionistConfigDefault`: Return default `ImpressionistConfig{}`.

**Verify**: File compiles in isolation.

#### Task 1.4: Ink Wash Module

**Files**: `src/effects/ink_wash.h` (create), `src/effects/ink_wash.cpp` (create)

**Do**:
- Move `InkWashConfig` from `src/config/ink_wash_config.h`. Keep all fields and defaults.
- Define `InkWashEffect` struct: `shader` + all 6 uniform location ints (resolution, strength, granulation, bleedStrength, bleedRadius, softness).
- `InkWashEffectInit`: Load shader (`ink_wash.fs`), cache all uniform locations. Return false if `shader.id == 0`.
- `InkWashEffectSetup`: Set all uniforms. Cast `softness` to `int` before `SetShaderValue` (matches existing behavior in `shader_setup_artistic.cpp:66`). Set resolution from `GetScreenWidth()`/`GetScreenHeight()`.
- `InkWashEffectUninit`: Unload shader.
- `InkWashConfigDefault`: Return default `InkWashConfig{}`.

**Verify**: File compiles in isolation.

#### Task 1.5: Pencil Sketch Module

**Files**: `src/effects/pencil_sketch.h` (create), `src/effects/pencil_sketch.cpp` (create)

**Do**:
- Move `PencilSketchConfig` from `src/config/pencil_sketch_config.h`. Keep all fields and defaults.
- Define `PencilSketchEffect` struct: `shader` + all 9 uniform location ints (resolution, angleCount, sampleCount, strokeFalloff, gradientEps, paperStrength, vignetteStrength, wobbleTime, wobbleAmount) + `float wobbleTime` (animation accumulator, moved from `pe->pencilSketchWobbleTime`).
- `PencilSketchEffectInit`: Load shader (`pencil_sketch.fs`), cache all uniform locations. Initialize `wobbleTime = 0.0f`. Return false if `shader.id == 0`.
- `PencilSketchEffectSetup`: Accumulate `e->wobbleTime += cfg->wobbleSpeed * deltaTime`. Set all uniforms from config, using `e->wobbleTime` for the wobbleTime uniform. Set resolution from `GetScreenWidth()`/`GetScreenHeight()`.
- `PencilSketchEffectUninit`: Unload shader.
- `PencilSketchConfigDefault`: Return default `PencilSketchConfig{}`.

**Verify**: File compiles in isolation.

#### Task 1.6: Cross Hatching Module

**Files**: `src/effects/cross_hatching.h` (create), `src/effects/cross_hatching.cpp` (create)

**Do**:
- Move `CrossHatchingConfig` from `src/config/cross_hatching_config.h`. Keep all fields and defaults.
- Define `CrossHatchingEffect` struct: `shader` + all 6 uniform location ints (resolution, time, width, threshold, noise, outline) + `float time` (animation accumulator, moved from `pe->crossHatchingTime`).
- `CrossHatchingEffectInit`: Load shader (`cross_hatching.fs`), cache all uniform locations. Initialize `time = 0.0f`. Return false if `shader.id == 0`.
- `CrossHatchingEffectSetup`: Accumulate `e->time += deltaTime`. Set all uniforms from config, using `e->time` for the time uniform. Set resolution from `GetScreenWidth()`/`GetScreenHeight()`.
- `CrossHatchingEffectUninit`: Unload shader.
- `CrossHatchingConfigDefault`: Return default `CrossHatchingConfig{}`.

**Verify**: File compiles in isolation.

---

### Wave 2: Integration (1 task)

#### Task 2.1: Integration Updates

**Files** (all modify):
- `src/config/effect_config.h`
- `src/render/post_effect.h`
- `src/render/post_effect.cpp`
- `src/render/shader_setup_artistic.cpp`
- `src/render/shader_setup.cpp`
- `src/render/shader_setup.h`
- `src/render/render_pipeline.cpp`
- `src/ui/imgui_effects_artistic.cpp`
- `src/config/preset.cpp`
- `CMakeLists.txt`

**Depends on**: Wave 1 complete

**Do**:

1. **`src/config/effect_config.h`**:
   - Remove 6 config header includes (`cross_hatching_config.h`, `impressionist_config.h`, `ink_wash_config.h`, `oil_paint_config.h`, `pencil_sketch_config.h`, `watercolor_config.h`)
   - Add 6 effect module includes (`effects/oil_paint.h`, `effects/watercolor.h`, `effects/impressionist.h`, `effects/ink_wash.h`, `effects/pencil_sketch.h`, `effects/cross_hatching.h`)
   - Config struct member names, enum values, `TransformEffectName` entries, `IsTransformEnabled` cases, and `TransformOrderConfig::order` entries remain UNCHANGED.

2. **`src/render/post_effect.h`**:
   - Add 6 effect module includes (`effects/oil_paint.h`, etc.)
   - Remove flat shader/uniform/state fields for all 6 effects:
     - Oil paint: `oilPaintShader`, `oilPaintStrokeShader`, `oilPaintNoiseTex`, `oilPaintIntermediate`, plus 7 uniform location ints
     - Watercolor: `watercolorShader` plus 9 uniform location ints
     - Impressionist: `impressionistShader` plus 12 uniform location ints
     - Ink wash: `inkWashShader` plus 6 uniform location ints
     - Pencil sketch: `pencilSketchShader` plus 9 uniform location ints
     - Cross hatching: `crossHatchingShader` plus 6 uniform location ints
   - Remove time accumulators: `crossHatchingTime`, `pencilSketchWobbleTime`
   - Add 6 effect struct members: `OilPaintEffect oilPaint;`, `WatercolorEffect watercolor;`, `ImpressionistEffect impressionist;`, `InkWashEffect inkWash;`, `PencilSketchEffect pencilSketch;`, `CrossHatchingEffect crossHatching;`

3. **`src/render/post_effect.cpp`**:
   - Replace 6 shader LoadShader calls with module Init calls. Pattern: `if (!OilPaintEffectInit(&pe->oilPaint)) { free(pe); return NULL; }`. Use `TraceLog(LOG_ERROR, ...)` before `free(pe); return NULL;`.
   - Remove all GetShaderLocation calls for these 6 effects.
   - Remove SetShaderValue resolution calls in `PostEffectResize` for these 6 effects (modules set their own resolution in Setup).
   - Add `OilPaintEffectResize(&pe->oilPaint, width, height);` in `PostEffectResize`.
   - Replace UnloadShader/UnloadTexture/UnloadRenderTexture calls with module Uninit calls.
   - Remove noise texture generation (moved to oil paint module Init).
   - Remove `oilPaintIntermediate` creation and recreation.
   - The shader validity check block: remove the 6 artistic shader checks from the big `&&` chain.

4. **`src/render/shader_setup_artistic.cpp`**:
   - Add 6 effect module includes.
   - Replace each Setup function body with module delegation. Pattern:
     ```
     void SetupOilPaint(PostEffect *pe) {
       OilPaintEffectSetup(&pe->oilPaint, &pe->effects.oilPaint, pe->currentDeltaTime);
     }
     ```
   - Remove time accumulation lines from `SetupPencilSketch` and `SetupCrossHatching` (now in modules).

5. **`src/render/shader_setup.cpp`**:
   - Update `GetTransformEffect` dispatcher entries for all 6 effects to use module shader. Pattern: `case TRANSFORM_OIL_PAINT: return {&pe->oilPaint.compositeShader, SetupOilPaint, &pe->effects.oilPaint.enabled};`
   - Update `ApplyHalfResOilPaint` to reference `pe->oilPaint.strokeShader`, `pe->oilPaint.strokeResolutionLoc`, `pe->oilPaint.brushSizeLoc`, `pe->oilPaint.strokeBendLoc`, `pe->oilPaint.layersLoc`, `pe->oilPaint.noiseTexLoc`, `pe->oilPaint.noiseTex`, `pe->oilPaint.compositeShader`, `pe->oilPaint.compositeResolutionLoc`, `pe->oilPaint.specularLoc` instead of flat `pe->` fields.
   - Also update intermediate RT reference: `pe->oilPaint.intermediate` instead of `pe->oilPaintIntermediate`.
   - Delete `ApplyOilPaintStrokePass` function definition (dead code at ~line 371 that references flat `pe->` fields being removed).

6. **`src/render/shader_setup.h`**:
   - Remove `ApplyOilPaintStrokePass` declaration (dead code, never called outside its definition).

7. **`src/render/render_pipeline.cpp`**:
   - No changes needed. Time accumulators for these effects don't exist in render_pipeline. The `HALF_RES_EFFECTS` array and `IsHalfResEffect` remain unchanged. The `TRANSFORM_OIL_PAINT` special case in the transform loop remains unchanged.

8. **`src/ui/imgui_effects_artistic.cpp`**:
   - No include changes needed. The file includes `config/effect_config.h` which will now transitively include the effect module headers.

9. **`src/config/preset.cpp`**:
   - No include changes needed. `preset.cpp` includes `config/effect_config.h` which transitively includes the effect headers.
   - Update watercolor NLOHMANN macro: add `edgePool`, `flowCenter`, `flowWidth`.
   - NLOHMANN macros, `to_json`, and `from_json` entries remain unchanged (struct names and field names are identical).

10. **`CMakeLists.txt`**:
    - Add 6 entries to `EFFECTS_SOURCES`: `src/effects/oil_paint.cpp`, `src/effects/watercolor.cpp`, `src/effects/impressionist.cpp`, `src/effects/ink_wash.cpp`, `src/effects/pencil_sketch.cpp`, `src/effects/cross_hatching.cpp`.

11. **Delete old config headers**: Remove `src/config/oil_paint_config.h`, `src/config/watercolor_config.h`, `src/config/impressionist_config.h`, `src/config/ink_wash_config.h`, `src/config/pencil_sketch_config.h`, `src/config/cross_hatching_config.h`.

12. **Grep for stale references**: Search entire `src/` for any remaining `#include.*oil_paint_config\|watercolor_config\|impressionist_config\|ink_wash_config\|pencil_sketch_config\|cross_hatching_config` and remove.

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` compiles with no errors.

---

## Pitfall Prevention Checklist

- [ ] Effect module headers define config struct inline (NO `#include "config/*_config.h"`)
- [ ] `free(pe); return NULL;` not `return false;` in PostEffectInit
- [ ] Grep for stale config includes across ALL touched files
- [ ] Time accumulators removed from PostEffect (`crossHatchingTime`, `pencilSketchWobbleTime`)
- [ ] Oil paint intermediate RT + noise texture moved to module
- [ ] Watercolor NLOHMANN macro updated with missing fields
- [ ] No null-checks in Init/Setup/Uninit functions (Batch 3 pitfall #7)
- [ ] Use `shader.id == 0` pattern, not `IsShaderValid()` (Batch 3 pitfall #8)
- [ ] ConfigDefault returns default-initialized struct `{}` (Batch 3 pitfall #9)

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All 6 effects render identically to before migration
- [ ] Presets with artistic effects load correctly
- [ ] Modulatable parameters respond to LFOs
- [ ] Oil paint multi-pass rendering works at half resolution
- [ ] Window resize recreates oil paint intermediate RT
