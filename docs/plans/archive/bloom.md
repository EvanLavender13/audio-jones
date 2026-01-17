# Bloom

Dual Kawase bloom effect that extracts bright pixels, blurs them through a resolution pyramid, and composites the glow back additively. Creates soft HDR-style glowing highlights that respond to audio modulation.

## Current State

Transform effect infrastructure exists with 29 effects following the same pattern:
- `src/config/effect_config.h:42-73` - TransformEffectType enum, config members
- `src/render/post_effect.h:16-324` - Shader members, uniform locations
- `src/render/shader_setup.cpp` - GetTransformEffect dispatch, setup functions
- `src/render/render_pipeline.cpp:381-388` - Transform loop using ping-pong buffers

Research documentation with complete algorithm: `docs/research/bloom.md`

## Technical Implementation

**Source**: [frost.kiwi - Dual Kawase](https://blog.frost.kiwi/dual-kawase/), [Catlike Coding - Bloom](https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/)

### Mip Chain Structure

Allocate 5 render textures at decreasing resolutions:
- mip[0]: screenWidth/2 × screenHeight/2 (prefilter output)
- mip[1]: screenWidth/4 × screenHeight/4
- mip[2]: screenWidth/8 × screenHeight/8
- mip[3]: screenWidth/16 × screenHeight/16
- mip[4]: screenWidth/32 × screenHeight/32

### Prefilter Shader (bloom_prefilter.fs)

Soft threshold extraction with knee smoothing:

```glsl
uniform float threshold;
uniform float knee;

vec3 SoftThreshold(vec3 color)
{
    float brightness = max(color.r, max(color.g, color.b));
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);
    return color * contribution;
}

void main()
{
    vec3 color = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(SoftThreshold(color), 1.0);
}
```

### Downsample Shader (bloom_downsample.fs)

Dual Kawase pattern - center (4x weight) + four diagonals (1x each):

```glsl
uniform vec2 halfpixel;

void main()
{
    vec2 uv = fragTexCoord;
    vec3 sum = texture(texture0, uv).rgb * 4.0;
    sum += texture(texture0, uv + vec2(-halfpixel.x, -halfpixel.y)).rgb;
    sum += texture(texture0, uv + vec2( halfpixel.x, -halfpixel.y)).rgb;
    sum += texture(texture0, uv + vec2(-halfpixel.x,  halfpixel.y)).rgb;
    sum += texture(texture0, uv + vec2( halfpixel.x,  halfpixel.y)).rgb;
    finalColor = vec4(sum / 8.0, 1.0);
}
```

### Upsample Shader (bloom_upsample.fs)

Four edge samples (1x) + four diagonal corners (2x):

```glsl
uniform vec2 halfpixel;

void main()
{
    vec2 uv = fragTexCoord;
    vec3 sum = vec3(0.0);

    sum += texture(texture0, uv + vec2(-halfpixel.x * 2.0, 0.0)).rgb;
    sum += texture(texture0, uv + vec2( halfpixel.x * 2.0, 0.0)).rgb;
    sum += texture(texture0, uv + vec2(0.0, -halfpixel.y * 2.0)).rgb;
    sum += texture(texture0, uv + vec2(0.0,  halfpixel.y * 2.0)).rgb;

    sum += texture(texture0, uv + vec2(-halfpixel.x,  halfpixel.y)).rgb * 2.0;
    sum += texture(texture0, uv + vec2( halfpixel.x,  halfpixel.y)).rgb * 2.0;
    sum += texture(texture0, uv + vec2(-halfpixel.x, -halfpixel.y)).rgb * 2.0;
    sum += texture(texture0, uv + vec2( halfpixel.x, -halfpixel.y)).rgb * 2.0;

    finalColor = vec4(sum / 12.0, 1.0);
}
```

### Composite Shader (bloom_composite.fs)

Additive blend with intensity control:

```glsl
uniform sampler2D bloomTexture;
uniform float intensity;

void main()
{
    vec3 original = texture(texture0, fragTexCoord).rgb;
    vec3 bloom = texture(bloomTexture, fragTexCoord).rgb;
    finalColor = vec4(original + bloom * intensity, 1.0);
}
```

### Pass Sequence

1. Render prefilter shader from source → mip[0] (half res)
2. Downsample: mip[0] → mip[1] → mip[2] → mip[3] → mip[4]
3. Upsample: mip[4] → mip[3] → mip[2] → mip[1] → mip[0] (additively blend at each level)
4. Composite: original + mip[0] * intensity → output

### Parameters

| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| threshold | float | 0.0 - 2.0 | 0.8 |
| knee | float | 0.0 - 1.0 | 0.5 |
| intensity | float | 0.0 - 2.0 | 0.5 |
| iterations | int | 3 - 5 | 5 |

---

## Phase 1: Config and Shaders

**Goal**: Create bloom config struct and all four shaders.

**Build**:
- Create `src/config/bloom_config.h` with BloomConfig struct (enabled, threshold, knee, intensity, iterations)
- Create `shaders/bloom_prefilter.fs` with soft threshold algorithm
- Create `shaders/bloom_downsample.fs` with dual Kawase downsample
- Create `shaders/bloom_upsample.fs` with dual Kawase upsample
- Create `shaders/bloom_composite.fs` with additive blend

**Done when**: Config header compiles, shaders have correct GLSL syntax.

---

## Phase 2: PostEffect Integration

**Goal**: Add bloom textures, shaders, and uniform locations to PostEffect.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `RenderTexture2D bloomMips[5]` for mip chain
  - Add four shader members: bloomPrefilterShader, bloomDownsampleShader, bloomUpsampleShader, bloomCompositeShader
  - Add uniform locations: bloomThresholdLoc, bloomKneeLoc, bloomHalfpixelLoc, bloomIntensityLoc, bloomBloomTexLoc
- Modify `src/render/post_effect.cpp`:
  - In PostEffectInit: allocate bloomMips at half/quarter/etc resolutions, load four shaders, get uniform locations
  - In PostEffectResize: reallocate bloomMips at new resolutions
  - In PostEffectUninit: unload shaders, unload bloomMips textures

**Done when**: PostEffect initializes without errors, bloom textures allocated at correct sizes.

---

## Phase 3: Effect Registration

**Goal**: Register TRANSFORM_BLOOM in the effect system.

**Build**:
- Modify `src/config/effect_config.h`:
  - Add `#include "bloom_config.h"`
  - Add `TRANSFORM_BLOOM` to TransformEffectType enum (before TRANSFORM_EFFECT_COUNT)
  - Add `case TRANSFORM_BLOOM: return "Bloom";` in TransformEffectName
  - Add `TRANSFORM_BLOOM` to TransformOrderConfig::order array
  - Add `BloomConfig bloom;` to EffectConfig struct
  - Add `case TRANSFORM_BLOOM: return e->bloom.enabled;` in IsTransformEnabled

**Done when**: TRANSFORM_BLOOM appears in effect count, builds without errors.

---

## Phase 4: Multi-Pass Processing

**Goal**: Implement bloom multi-pass logic in shader_setup.

**Build**:
- Modify `src/render/shader_setup.h`:
  - Declare `void SetupBloom(PostEffect* pe);`
  - Declare `void ApplyBloomPasses(PostEffect* pe, RenderTexture2D* source, int* writeIdx);`
- Modify `src/render/shader_setup.cpp`:
  - Add dispatch case in GetTransformEffect for TRANSFORM_BLOOM returning bloomCompositeShader
  - Implement ApplyBloomPasses:
    1. Run prefilter from source to bloomMips[0]
    2. Loop downsample passes: mips[i-1] → mips[i]
    3. Loop upsample passes: mips[i] → mips[i-1] (blend at each level)
  - Implement SetupBloom to bind bloomMips[0] as bloomTexture uniform + intensity
- Modify `src/render/render_pipeline.cpp`:
  - In transform loop, detect TRANSFORM_BLOOM and call ApplyBloomPasses before RenderPass

**Done when**: Bloom effect visually works with threshold/knee/intensity controls.

---

## Phase 5: UI Panel

**Goal**: Add bloom controls to the Style category in Effects panel.

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add `case TRANSFORM_BLOOM: return CATEGORY_STYLE;` in GetTransformCategory
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionBloom = false;` at top
  - Add DrawStyleBloom helper function with:
    - Enabled checkbox with MoveTransformToEnd on enable
    - ModulatableSlider for threshold (0.0-2.0)
    - Slider for knee (0.0-1.0)
    - ModulatableSlider for intensity (0.0-2.0)
    - SliderInt for iterations (3-5)
  - Add `DrawStyleBloom(e, modSources, categoryGlow);` call in DrawStyleCategory

**Done when**: Bloom section appears in Style category, sliders control effect.

---

## Phase 6: Preset Serialization and Modulation

**Goal**: Save/load bloom settings and enable audio modulation.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BloomConfig, enabled, threshold, knee, intensity, iterations)`
  - Add `if (e.bloom.enabled) { j["bloom"] = e.bloom; }` in to_json
  - Add `e.bloom = j.value("bloom", e.bloom);` in from_json
- Modify `src/automation/param_registry.cpp`:
  - Add `{"bloom.threshold", {0.0f, 2.0f}}` to PARAM_TABLE
  - Add `{"bloom.intensity", {0.0f, 2.0f}}` to PARAM_TABLE
  - Add `&effects->bloom.threshold` to targets array (matching index)
  - Add `&effects->bloom.intensity` to targets array (matching index)

**Done when**: Presets save/load bloom settings, intensity/threshold respond to modulation.

---

## Phase 7: Testing and Polish

**Goal**: Verify full integration and visual quality.

**Build**:
- Test bloom with various input scenes (bright waveforms, dark backgrounds, simulations)
- Verify transform order drag-drop works
- Verify preset round-trip preserves all bloom settings
- Verify modulation routes to threshold and intensity
- Check performance with profiler (bloom should be fast due to downsampled processing)
- Adjust default values if needed for best visual results

**Done when**: Bloom works correctly in all scenarios, no visual artifacts.
