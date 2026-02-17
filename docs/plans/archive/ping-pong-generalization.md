# Ping-Pong Render Pipeline Generalization

Eliminates per-effect `else if` branches in the transform dispatch loop for ping-pong generators. Adds a `render` function pointer to `EffectDescriptor` so new ping-pong generators register their render call instead of requiring pipeline modifications.

**Research**: `docs/research/ping-pong-generalization.md`

## Design

### EffectDescriptor Change

Add one field after `scratchSetup`:

```
void (*render)(PostEffect *pe) = nullptr;
```

When non-NULL, the pipeline calls `scratchSetup(pe)` → `render(pe)` → `RenderPass(blend)` instead of the per-effect branch. When NULL, existing dispatch (standard generators, half-res, etc.) is unchanged.

### Macro Change

`REGISTER_GENERATOR_FULL` gains a 7th positional parameter `RenderFn` — a function name with signature `void(PostEffect*)`. The macro forward-declares it and stores it in the descriptor.

### Static Render Wrappers

Each ping-pong generator defines a static wrapper in its `.cpp` file that adapts its effect-specific render signature to `void(PostEffect*)`:

- `attractor_lines.cpp`: wraps `AttractorLinesEffectRender(&pe->attractorLines, &pe->effects.attractorLines, pe->currentDeltaTime, pe->screenWidth, pe->screenHeight)`
- `fireworks.cpp`: wraps `FireworksEffectRender(&pe->fireworks, &pe->effects.fireworks, pe->currentDeltaTime, pe->screenWidth, pe->screenHeight, pe->fftTexture)`

### Pipeline Change

Replace the two `else if` branches (lines 321-334) with:

```
} else if (descriptor.render != nullptr) {
    descriptor.scratchSetup(pe);
    descriptor.render(pe);
    RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
}
```

This must appear BEFORE the `EFFECT_FLAG_BLEND` check (standard generators render to `generatorScratch`, not their own textures).

### Includes Cleanup

Remove `#include "effects/attractor_lines.h"` and `#include "effects/fireworks.h"` from `render_pipeline.cpp` — the pipeline no longer calls those functions directly.

---

## Tasks

### Wave 1: All files (no overlap)

#### Task 1.1: Add render field to EffectDescriptor and update macro

**Files**: `src/config/effect_descriptor.h`

**Do**: Add `void (*render)(PostEffect *pe) = nullptr;` field to `EffectDescriptor` after `scratchSetup`. Update `REGISTER_GENERATOR_FULL` macro: add `RenderFn` as the 7th parameter, forward-declare it as `void RenderFn(PostEffect *);`, and include it in the descriptor initializer.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Add render wrapper to attractor_lines.cpp

**Files**: `src/effects/attractor_lines.cpp`

**Do**: Add a static render wrapper before the registration macro:
```cpp
static void RenderAttractorLines(PostEffect *pe) {
    AttractorLinesEffectRender(&pe->attractorLines, &pe->effects.attractorLines,
                                pe->currentDeltaTime, pe->screenWidth,
                                pe->screenHeight);
}
```
Update the `REGISTER_GENERATOR_FULL` call to pass `RenderAttractorLines` as the 7th argument.

**Verify**: Compiles.

#### Task 1.3: Add render wrapper to fireworks.cpp

**Files**: `src/effects/fireworks.cpp`

**Do**: Add a static render wrapper before the registration macro:
```cpp
static void RenderFireworks(PostEffect *pe) {
    FireworksEffectRender(&pe->fireworks, &pe->effects.fireworks,
                          pe->currentDeltaTime, pe->screenWidth,
                          pe->screenHeight, pe->fftTexture);
}
```
Update the `REGISTER_GENERATOR_FULL` call to pass `RenderFireworks` as the 7th argument.

**Verify**: Compiles.

#### Task 1.4: Simplify pipeline dispatch

**Files**: `src/render/render_pipeline.cpp`

**Do**: Replace the `TRANSFORM_ATTRACTOR_LINES_BLEND` and `TRANSFORM_FIREWORKS_BLEND` branches (lines 321-334) with a single `EFFECT_DESCRIPTORS[effectType].render != nullptr` check. Place it BEFORE the `EFFECT_FLAG_BLEND` branch. Remove `#include "effects/attractor_lines.h"` and `#include "effects/fireworks.h"` from the includes.

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Attractor Lines generator still renders (enable in UI, verify ping-pong trails persist)
- [ ] Fireworks generator still renders (enable in UI, verify burst trails persist)
- [ ] Standard generators (e.g., Plasma, Nebula) still render correctly
- [ ] No functional change — purely structural refactor
