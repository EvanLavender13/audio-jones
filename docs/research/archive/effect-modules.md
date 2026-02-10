# Effect Modules

Refactor PostEffect's monolithic struct into self-contained effect modules. Each effect becomes a single file containing config, runtime state, shader handle, uniform locations, and lifecycle functions.

## Classification

- **Type**: General (software architecture)
- **Chosen Approach**: Incremental modularization with batch migration

## Problem

PostEffect struct holds 60+ shader handles, 400+ uniform location ints, and 50+ animation state floats as flat fields. Adding an effect requires touching:
- `post_effect.h` (add shader field + uniform fields + state fields)
- `post_effect.cpp` (load shader + cache uniforms)
- `shader_setup_*.cpp` (setup function)
- Plus config, UI, param registry, preset serialization

The setup functions already exist per-effect, but reach into `pe->` for scattered data.

## Solution

Each effect becomes a self-contained module:

```c
// src/effects/sine_warp.h
#ifndef SINE_WARP_EFFECT_H
#define SINE_WARP_EFFECT_H

#include "raylib.h"
#include <stdbool.h>

// Config (user-facing parameters, serialized in presets)
typedef struct SineWarpConfig {
    bool enabled;
    int octaves;
    float strength;
    float octaveRotation;
    float speed;
    bool radialMode;
    bool depthBlend;
} SineWarpConfig;

// Runtime state (shader + uniforms + animation)
typedef struct SineWarpEffect {
    Shader shader;
    int timeLoc;
    int rotationLoc;
    int octavesLoc;
    int strengthLoc;
    int octaveRotationLoc;
    int radialModeLoc;
    int depthBlendLoc;
    float time;  // animation accumulator
} SineWarpEffect;

// Lifecycle
bool SineWarpEffectInit(SineWarpEffect* e);
void SineWarpEffectSetup(SineWarpEffect* e, const SineWarpConfig* cfg, float deltaTime);
void SineWarpEffectUninit(SineWarpEffect* e);

// Default config
SineWarpConfig SineWarpConfigDefault(void);

#endif
```

```c
// src/effects/sine_warp.cpp
#include "sine_warp.h"

bool SineWarpEffectInit(SineWarpEffect* e) {
    e->shader = LoadShader(0, "shaders/sine_warp.fs");
    if (e->shader.id == 0) return false;

    e->timeLoc = GetShaderLocation(e->shader, "time");
    e->rotationLoc = GetShaderLocation(e->shader, "rotation");
    e->octavesLoc = GetShaderLocation(e->shader, "octaves");
    e->strengthLoc = GetShaderLocation(e->shader, "strength");
    e->octaveRotationLoc = GetShaderLocation(e->shader, "octaveRotation");
    e->radialModeLoc = GetShaderLocation(e->shader, "radialMode");
    e->depthBlendLoc = GetShaderLocation(e->shader, "depthBlend");
    e->time = 0.0f;
    return true;
}

void SineWarpEffectSetup(SineWarpEffect* e, const SineWarpConfig* cfg, float deltaTime) {
    e->time += cfg->speed * deltaTime;
    SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->rotationLoc, &e->time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
    SetShaderValue(e->shader, e->strengthLoc, &cfg->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->octaveRotationLoc, &cfg->octaveRotation, SHADER_UNIFORM_FLOAT);
    int radialMode = cfg->radialMode ? 1 : 0;
    SetShaderValue(e->shader, e->radialModeLoc, &radialMode, SHADER_UNIFORM_INT);
    int depthBlend = cfg->depthBlend ? 1 : 0;
    SetShaderValue(e->shader, e->depthBlendLoc, &depthBlend, SHADER_UNIFORM_INT);
}

void SineWarpEffectUninit(SineWarpEffect* e) {
    UnloadShader(e->shader);
}

SineWarpConfig SineWarpConfigDefault(void) {
    return (SineWarpConfig){
        .enabled = false,
        .octaves = 3,
        .strength = 0.02f,
        .octaveRotation = 0.5f,
        .speed = 1.0f,
        .radialMode = false,
        .depthBlend = false,
    };
}
```

## PostEffect Changes

PostEffect shrinks to infrastructure plus effect modules:

```c
typedef struct PostEffect {
    // Render infrastructure
    RenderTexture2D accumTexture;
    RenderTexture2D pingPong[2];
    RenderTexture2D outputTexture;
    RenderTexture2D bloomMips[BLOOM_MIP_COUNT];
    RenderTexture2D halfResA, halfResB;
    int screenWidth, screenHeight;

    // Feedback shaders (non-transform, stay here)
    Shader feedbackShader;
    Shader blurHShader, blurVShader;
    // ... feedback uniform locations ...

    // Output shaders (non-transform, stay here)
    Shader chromaticShader, fxaaShader, clarityShader, gammaShader;
    // ... output uniform locations ...

    // Transform effect modules
    SineWarpEffect sineWarp;
    TextureWarpEffect textureWarp;
    VoronoiEffect voronoi;
    // ... one field per transform effect ...

    // Simulations (unchanged)
    Physarum* physarum;
    Boids* boids;
    // ...

    // Configs live in EffectConfig (for preset serialization)
    EffectConfig effects;
} PostEffect;
```

## Dispatcher Changes

`GetTransformEffect` returns the module's shader and a setup wrapper:

```c
TransformEffectEntry GetTransformEffect(PostEffect* pe, TransformEffectType type) {
    switch (type) {
    case TRANSFORM_SINE_WARP:
        return {
            &pe->sineWarp.shader,
            [](PostEffect* pe) {
                SineWarpEffectSetup(&pe->sineWarp, &pe->effects.sineWarp, pe->currentDeltaTime);
            },
            &pe->effects.sineWarp.enabled
        };
    // ...
    }
}
```

Or simplify by having render_pipeline call the setup directly.

## Migration Strategy

**Batch 1: Warp effects** (12 effects)
- sine_warp, texture_warp, wave_ripple, mobius, gradient_flow, chladni_warp, domain_warp, surface_warp, interference_warp, corridor_warp, fft_radial_warp, circuit_board

**Batch 2: Symmetry effects** (7 effects)
- kaleidoscope, kifs, poincare_disk, mandelbox, triangle_fold, moire_interference, radial_ifs, radial_pulse

**Batch 3: Cellular effects** (3 effects)
- voronoi, lattice_fold, phyllotaxis

**Batch 4: Motion effects** (6 effects)
- infinite_zoom, radial_streak, droste_zoom, density_wave_spiral, shake, relativistic_doppler

**Batch 5: Artistic effects** (6 effects)
- oil_paint, watercolor, impressionist, ink_wash, pencil_sketch, cross_hatching

**Batch 6: Graphic effects** (6 effects)
- toon, neon_glow, kuwahara, halftone, disco_ball, lego_bricks

**Batch 7: Retro effects** (5 effects)
- pixelation, glitch, ascii_art, matrix_rain, synthwave

**Batch 8: Optical effects** (4 effects)
- bloom, anamorphic_streak, bokeh, heightfield_relief

**Batch 9: Color effects** (3 effects)
- color_grade, false_color, palette_quantization

**Batch 10: Generators** (3 effects)
- constellation, plasma, interference

Each batch:
1. Create `src/effects/<name>.h` and `src/effects/<name>.cpp` for each effect
2. Move config struct from `src/config/<name>_config.h` into effect header
3. Move shader/uniform/state fields from PostEffect into effect struct
4. Move setup logic from `shader_setup_*.cpp` into effect module
5. Update PostEffect to hold effect module instead of raw fields
6. Update dispatcher and render_pipeline
7. Delete empty `src/config/<name>_config.h` files
8. Update preset.cpp serialization to use new config location
9. Update param_registry.cpp to point at new config location

## File Impact Per Effect

Before (scattered across ~6 files):
- `src/config/sine_warp_config.h` - config struct
- `src/render/post_effect.h` - shader + uniform + state fields
- `src/render/post_effect.cpp` - load shader + cache uniforms
- `src/render/shader_setup_warp.cpp` - setup function
- `src/render/shader_setup.cpp` - dispatcher entry

After (1 primary file + updates):
- `src/effects/sine_warp.h` - config + runtime + lifecycle (NEW)
- `src/effects/sine_warp.cpp` - implementation (NEW)
- `src/render/post_effect.h` - one `SineWarpEffect sineWarp;` field
- `src/render/post_effect.cpp` - one `SineWarpEffectInit()` call
- `src/render/shader_setup.cpp` - dispatcher entry (simplified)

## Serialization

Preset serialization needs access to config structs. Options:

1. **Include effect headers in preset.cpp**: `#include "effects/sine_warp.h"`
2. **Forward declare configs**: Keep NLOHMANN macros working with included types

The config structs remain plain data with NLOHMANN_DEFINE macros. Moving them into effect headers doesn't change serialization, just the include path.

## Param Registry

`param_registry.cpp` uses `offsetof(EffectConfig, sineWarp.strength)` for modulation. This continues working as long as:
- EffectConfig still contains `SineWarpConfig sineWarp;`
- SineWarpConfig struct layout unchanged

The config struct can live in `effects/sine_warp.h`; EffectConfig in `effect_config.h` includes it.

## Benefits

1. **One file to read**: Understanding an effect requires reading one file
2. **One file to add**: New effects create one module file + minimal PostEffect/dispatcher updates
3. **Smaller PostEffect**: ~60 effect structs vs ~600 individual fields
4. **Testable**: Effect modules can be unit tested in isolation
5. **Cache-friendly**: Related data (shader + uniforms + state) lives together

## Risks

1. **Include graph**: effect_config.h includes 60+ effect headers (acceptable, already includes 60+ config headers)
2. **Migration effort**: ~55 effects to migrate across 10 batches
3. **Preset compatibility**: Config struct moves but layout unchanged; existing presets load correctly

## Agent Pitfalls (Batch 1 Learnings)

Parallel agents made these errors during batch 1 migration. Future batch prompts must explicitly warn against these:

### 1. Including Deleted Config Headers

**Error**: `mobius.h` included `#include "config/mobius_config.h"` instead of defining `MobiusConfig` inline.

**Fix**: Effect modules MUST define their own config struct. The old `config/<name>_config.h` gets DELETED. Do NOT include it.

**Prompt instruction**: "Define the config struct directly in this header. Do NOT include any `config/*_config.h` file—it will be deleted."

### 2. Wrong Return Type in PostEffectInit

**Error**: Agent added `return false;` in `PostEffectInit` which returns `PostEffect*`.

**Fix**: Use `free(pe); return NULL;` for init failures.

**Prompt instruction**: "PostEffectInit returns `PostEffect*`. On failure, use `free(pe); return NULL;` not `return false;`."

### 3. Stale Includes in Integration Files

**Error**: `imgui_effects_warp.cpp` and `preset.cpp` still included deleted config headers.

**Fix**: Integration task agents must remove ALL includes for migrated configs.

**Prompt instruction**: "Remove includes for ALL 12 migrated config headers. Search the file for `#include.*_config.h` patterns matching the migrated effects."

### 4. Stale Field References in render_pipeline.cpp

**Error**: Still referenced `pe->waveRippleTime`, `pe->mobiusTime`, `pe->currentMobiusPoint1[2]`, `pe->domainWarpDrift` etc.

**Fix**: The post_effect.h task removes these fields, but render_pipeline.cpp also uses them.

**Prompt instruction**: "render_pipeline.cpp accumulates time for some effects. Remove accumulation lines for migrated effects—the module's Setup function handles this now."

### 5. Lissajous Logic Still in render_pipeline.cpp

**Error**: `RenderPipelineApplyOutput` computed Lissajous positions for waveRipple and mobius origins.

**Fix**: This logic moves into the effect module's Setup function.

**Prompt instruction**: "Remove Lissajous computations from render_pipeline.cpp for effects that have `DualLissajousConfig` fields—the effect module's Setup handles this."

### Prevention Checklist for Future Batches

Add to each batch plan:

- [ ] Effect module headers define config struct inline (no config includes)
- [ ] post_effect.cpp uses `free(pe); return NULL;` not `return false;`
- [ ] Integration tasks grep for stale config includes across ALL touched files
- [ ] render_pipeline.cpp time accumulators removed for migrated effects
- [ ] render_pipeline.cpp Lissajous computations removed for migrated effects

## Notes

- Feedback effects (flow field, blur) stay in PostEffect—they're infrastructure, not transforms
- Output effects (chromatic, gamma, clarity, fxaa) could become modules later but are lower priority
- Simulations (physarum, boids, etc.) are already modular
- UI panels (`imgui_effects_*.cpp`) need updates to use new config paths

---

*To plan implementation: `/feature-plan docs/research/effect-modules.md`*
