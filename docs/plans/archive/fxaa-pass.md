# FXAA Post-Processing Pass

Add FXAA (Fast Approximate Anti-Aliasing) as the final output pass to eliminate aliased edges on waveforms and effects. MSAA does not work with RenderTextures, so shader-based AA is required.

## Current State

Output pipeline in `ApplyOutputPipeline()` (`src/render/post_effect.cpp:359-406`):
- Trail boost (optional) → `tempTexture`
- Kaleidoscope (optional) → `kaleidoTexture`
- Chromatic aberration → **screen** (direct draw, no texture)

FXAA must be the final pass, requiring chromatic to render to a texture first.

---

## Phase 1: Add FXAA Shader

**Goal**: Create FXAA 3.11 fragment shader following existing patterns.

**Build**:
- Create `shaders/fxaa.fs` with FXAA 3.11 algorithm
- Use `uniform vec2 resolution` for texel size calculation
- Follow existing shader structure (`#version 330`, `fragTexCoord`, `texture0`)

**Done when**: Shader file exists with standard FXAA implementation.

---

## Phase 2: Integrate Shader into PostEffect

**Goal**: Load FXAA shader and get uniform locations during initialization.

**Build**:
- Add to `PostEffect` struct (`src/render/post_effect.h:11-58`):
  - `Shader fxaaShader`
  - `int fxaaResolutionLoc`
- Add to `LoadPostEffectShaders()` (`post_effect.cpp:182-196`):
  - Load `shaders/fxaa.fs`
- Add to `GetShaderUniformLocations()` (`post_effect.cpp:198-226`):
  - Get `resolution` uniform location
- Add to `SetResolutionUniforms()` (`post_effect.cpp:228-235`):
  - Set resolution value
- Unload shader in `PostEffectUninit()`

**Done when**: Shader loads without errors, uniform location retrieved.

---

## Phase 3: Apply FXAA as Final Output Pass

**Goal**: Insert FXAA after chromatic aberration in output pipeline.

**Build**:
- Modify `ApplyOutputPipeline()` (`post_effect.cpp:393-406`):
  - Chromatic aberration renders to `tempTexture` (not screen)
  - Add FXAA pass that reads `tempTexture`, renders to screen
  - Handle bypass case (chromatic disabled): render source to `tempTexture`, then FXAA to screen

**Pipeline after change**:
```
currentSource
    ↓ chromatic aberration (or passthrough)
tempTexture
    ↓ FXAA
SCREEN
```

**Done when**: Waveform edges appear smooth, no aliasing artifacts visible.

---

## Phase 4: Verify and Test

**Goal**: Confirm FXAA works correctly across different visual configurations.

**Build**:
- Test with multiple presets (different effects enabled/disabled)
- Verify no visual artifacts at screen edges
- Confirm performance impact is minimal
- Check that physarum trails, waveforms, and spectrum bars all benefit from AA

**Done when**: Visual quality improved with no regressions.
