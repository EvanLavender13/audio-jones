# Remove Trail Buffer from Vortex and Shell

Remove the decay + blur trail buffer that was blindly copied from Muons into Vortex and Shell. Muons needs trail persistence because it renders thin single-pixel filaments that flicker without it. Vortex and Shell are solid volumetric raymarchers that produce a complete image every frame — they don't need trail persistence.

The changes follow the exact pattern from commit `4a1589de` (Synapse Tree strip).

## Design

### What Gets Removed

Per effect (vortex, shell):

**Config fields**: `decayHalfLife`, `trailBlur`

**Effect struct fields**: `pingPong[2]`, `readIdx`, `previousFrameLoc`, `decayFactorLoc`, `trailBlurLoc`

**Functions removed**: `InitPingPong()`, `UnloadPingPong()`, `<Name>EffectRender()`, `<Name>EffectResize()`, `Render<Name>()`

**Shader uniforms removed**: `previousFrame`, `decayFactor`, `trailBlur`

**Shader code removed**: Entire trail buffer section (3x3 kernel blur + decay blend). Replace with `finalColor = vec4(color, 1.0);`

### What Changes

- `<Name>EffectInit()` signature drops `int width, int height` params (no ping-pong to allocate)
- `<Name>EffectSetup()` gains texture binding calls (`SetShaderValueTexture` for gradientLUT, fftTexture) that were previously in the Render function
- `Setup<Name>Blend()` reads from `pe->generatorScratch.texture` instead of `pe-><name>.pingPong[pe-><name>.readIdx].texture`
- Registration switches from `REGISTER_GENERATOR_FULL` to `REGISTER_GENERATOR` (no custom render)

### What Stays

FFT audio, tonemap, all other parameters — unchanged.

---

## Tasks

### Wave 1: Headers

#### Task 1.1: Strip vortex.h

**Files**: `src/effects/vortex.h`

**Do**:
- Remove `decayHalfLife` and `trailBlur` from `VortexConfig`
- Remove them from `VORTEX_CONFIG_FIELDS` macro
- Remove `pingPong[2]`, `readIdx`, `previousFrameLoc`, `decayFactorLoc`, `trailBlurLoc` from `VortexEffect`
- Remove `VortexEffectRender()` and `VortexEffectResize()` declarations
- Change `VortexEffectInit()` signature: drop `int width, int height`
- Update comment on Init (no ping-pong), remove "Renders into ping-pong" comment on Render, remove "Reallocates ping-pong" comment on Resize

**Verify**: Reads clean.

#### Task 1.2: Strip shell.h

**Files**: `src/effects/shell.h`

**Do**: Same as Task 1.1 but for Shell types.
- Remove `decayHalfLife` and `trailBlur` from `ShellConfig`
- Remove them from `SHELL_CONFIG_FIELDS` macro
- Remove `pingPong[2]`, `readIdx`, `previousFrameLoc`, `decayFactorLoc`, `trailBlurLoc` from `ShellEffect`
- Remove `ShellEffectRender()` and `ShellEffectResize()` declarations
- Change `ShellEffectInit()` signature: drop `int width, int height`

**Verify**: Reads clean.

---

### Wave 2: Implementation + Shaders

#### Task 2.1: Strip vortex.cpp

**Files**: `src/effects/vortex.cpp`

**Depends on**: Wave 1

**Do**:
- Remove `#include "render/render_utils.h"` (only used for ping-pong)
- Remove `InitPingPong()` and `UnloadPingPong()` static helpers
- In `VortexEffectInit()`: drop `width`/`height` params, remove `GetShaderLocation` for `previousFrame`/`decayFactor`/`trailBlur`, remove `InitPingPong` + `RenderUtilsClearTexture` calls, remove `readIdx = 0`
- In `VortexEffectSetup()`: remove decay factor computation (safeHalfLife, expf, SetShaderValue for decayFactor and trailBlur). Add texture binds at end: `SetShaderValueTexture` for gradientLUT, fftTexture (moved from Render)
- Remove `VortexEffectRender()` entirely
- Remove `VortexEffectResize()` entirely
- In `VortexEffectUninit()`: remove `UnloadPingPong(e)` call
- In `VortexRegisterParams()`: remove `decayHalfLife` and `trailBlur` registrations
- Remove `RenderVortex()` bridge function
- In `SetupVortexBlend()`: change to `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, ...)`
- In UI: remove entire "Trails" section (SeparatorText + two sliders)
- Change `REGISTER_GENERATOR_FULL` to `REGISTER_GENERATOR` (drop `RenderVortex` arg)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Strip shell.cpp

**Files**: `src/effects/shell.cpp`

**Depends on**: Wave 1

**Do**: Same pattern as Task 2.1 but for Shell.
- Remove `#include "render/render_utils.h"`
- Remove `InitPingPong()` and `UnloadPingPong()`
- In `ShellEffectInit()`: drop `width`/`height`, remove loc caching for `previousFrame`/`decayFactor`/`trailBlur`, remove ping-pong init, remove `readIdx = 0`
- In `ShellEffectSetup()`: remove decay computation + SetShaderValue for decay/blur. Add texture binds for gradientLUT, fftTexture
- Remove `ShellEffectRender()` and `ShellEffectResize()`
- In `ShellEffectUninit()`: remove `UnloadPingPong(e)`
- In `ShellRegisterParams()`: remove decay/blur registrations
- Remove `RenderShell()` bridge function
- In `SetupShellBlend()`: use `pe->generatorScratch.texture`
- UI: remove "Trails" section
- Switch to `REGISTER_GENERATOR`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Strip vortex.fs

**Files**: `shaders/vortex.fs`

**Depends on**: Wave 1

**Do**:
- Remove `uniform sampler2D previousFrame;`, `uniform float decayFactor;`, `uniform float trailBlur;`
- Remove the entire trail buffer block (lines 107-121): the 3x3 kernel blur, `mix`, NaN guard, and `max(color, prev * decayFactor)` blend
- Replace with: `finalColor = vec4(color, 1.0);`
- Update header comment: remove "added trail buffer with decay/blur"

**Verify**: Shader reads clean.

#### Task 2.4: Strip shell.fs

**Files**: `shaders/shell.fs`

**Depends on**: Wave 1

**Do**: Same as Task 2.3 but for shell.fs.
- Remove `previousFrame`, `decayFactor`, `trailBlur` uniforms
- Remove trail buffer block (lines 101-115)
- Replace with: `finalColor = vec4(color, 1.0);`
- Update header comment

**Verify**: Shader reads clean.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Vortex and Shell render without trails (solid per-frame output)
- [ ] No references to `decayFactor`, `trailBlur`, `pingPong`, `previousFrame` remain in vortex or shell files
- [ ] Muons trail buffer is untouched
