# Clarity Output Effect

**Status**: Completed

Add a local contrast enhancement (clarity) pass to the output pipeline, positioned between chromatic aberration and FXAA. Enhances mid-frequency detail to make soft visuals "pop" without amplifying pixel-level noise.

## Implementation

### Shader: `shaders/clarity.fs`

Multi-radius local contrast enhancement:
- Samples at 8, 16, 24 pixel radii in 8 directions (24 samples total)
- Computes regional blur average
- Extracts mid-frequency detail: `center - blur`
- Boosts local contrast: `result = center + detail * clarity`
- Soft shoulder compression prevents white/color overflow (knee at 0.9)

Uniforms: `sampler2D texture0`, `vec2 resolution`, `float clarity`

### PostEffect Integration

- `src/render/post_effect.h:23` - `Shader clarityShader`
- `src/render/post_effect.h:69-70` - `int clarityResolutionLoc`, `int clarityAmountLoc`
- `src/render/post_effect.cpp:33` - Loads `shaders/clarity.fs`
- `src/render/post_effect.cpp:87-88` - Caches uniform locations
- `src/render/post_effect.cpp:104` - Sets resolution uniform
- `src/render/post_effect.cpp:180` - Unloads shader

### Pipeline Pass

- `src/render/render_pipeline.cpp:145-149` - `SetupClarity()` callback
- `src/render/render_pipeline.cpp:262` - RenderPass after chromatic, before FXAA

### Config and UI

- `src/config/effect_config.h:26` - `float clarity = 0.0f`
- `src/ui/imgui_effects.cpp:31` - Slider (0.0–2.0) in Core Effects section
- `src/config/preset.cpp:88` - Serialization macro

## Design Notes

Initial implementation used unsharp mask (1px kernel) but this amplified pixel noise rather than enhancing color region boundaries. Switched to clarity algorithm with larger sampling radii to target mid-frequency detail—the soft gradients from physarum/blur effects—without affecting fine detail.

Soft shoulder compression (knee at 0.9) prevents color overflow when boosting bright regions.
