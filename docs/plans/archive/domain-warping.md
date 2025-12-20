# Domain Warping for Feedback Effect

Animated fBM-based UV distortion in the feedback shader, creating flowing organic patterns that physarum agents can sense and follow.

## Goal

Add domain warping to the feedback pass so trails swirl and flow organically over time. Unlike static reaction-diffusion patterns, domain warping continuously evolves, creating dynamic marble-vein-like distortions that compound with each feedback frame.

The effect applies before zoom/rotation, so physarum agents naturally sense and follow the warped accumulation texture without any changes to physarum code.

Reference: [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/)

## Current State

The feedback shader applies zoom + rotation to create MilkDrop-style infinite tunnel effects:

- `shaders/feedback.fs:19-30` - UV transform: center offset, rotation matrix, zoom scale
- `src/render/post_effect.cpp:93-106` - `ApplyFeedbackPass()` sets uniforms and renders
- `src/render/post_effect.cpp:154` - Feedback called in `ApplyAccumulationPipeline()` before blur

Physarum agents sample the accumulation texture for sensing:

- `shaders/physarum_agents.glsl:97-102` - `sampleAccumAffinity()` reads accumTexture
- Agents already sense whatever is in accumTexture, so warped patterns work automatically

Configuration and UI follow established patterns:

- `src/config/effect_config.h:12-14` - Feedback params: zoom, rotation, desaturate
- `src/ui/ui_panel_effects.cpp:26-28` - Feedback sliders
- `src/config/preset.cpp:20-24` - JSON serialization via nlohmann macro

## Algorithm

### Domain Warping Transform

Single-layer warp using fractional Brownian motion (fBM):

```glsl
vec2 warpOffset = fbm(fragTexCoord * warpScale + vec2(warpTime, 0.0));
uv += warpOffset * warpStrength;
```

The warp applies to UV coordinates before rotation/zoom, so distortion compounds over frames as feedback accumulates.

### fBM Implementation

Value noise with cubic interpolation, summed across octaves:

```glsl
float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Cubic Hermite interpolation

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

vec2 fbm(vec2 p) {
    vec2 offset = vec2(0.0);
    float amplitude = 1.0;
    float frequency = 1.0;

    for (int i = 0; i < warpOctaves; i++) {
        offset.x += noise2D(p * frequency + warpTime) * amplitude;
        offset.y += noise2D(p * frequency + warpTime + vec2(5.2, 1.3)) * amplitude;
        amplitude *= 0.5;
        frequency *= warpLacunarity;
    }
    return offset;
}
```

The offset `vec2(5.2, 1.3)` breaks correlation between x/y components. Any arbitrary values work.

### Parameter Ranges

| Parameter | Range | Default | Rationale |
|-----------|-------|---------|-----------|
| warpStrength | 0.0-0.2 | 0.0 | 0.05 subtle, 0.15 strong swirls, 0.2 chaotic |
| warpScale | 1.0-20.0 | 5.0 | Lower = larger swirls, higher = finer detail |
| warpOctaves | 1-5 | 3 | More = finer detail, higher GPU cost |
| warpLacunarity | 1.5-3.0 | 2.0 | Standard fBM doubles frequency per octave |
| warpSpeed | 0.1-2.0 | 0.5 | Animation rate, 0 would freeze pattern |

## Architecture Decision

**Pragmatic Balance: Inline noise in feedback.fs**

Rationale:
1. Single warp layer is sufficient for flowing visuals
2. Inline code avoids premature abstraction (no shared noise library yet)
3. Clear upgrade path to double-warp or shared library if needed later
4. Follows existing voronoi/physarum parameter patterns

Trade-offs considered:
- **Shared noise library:** Would benefit reaction-diffusion and future effects, but adds complexity now. Extract later when 3+ shaders need noise.
- **Separate warp pass:** Extra fullscreen render, unnecessary since warp is just UV math.
- **Double warp:** More complex folding patterns, but higher GPU cost. Add as separate feature if desired.

## Integration

### 1. Shader Modification

Modify `shaders/feedback.fs`:
- Add 5 uniforms: warpStrength, warpScale, warpOctaves, warpLacunarity, warpTime
- Add hash/noise/fbm functions before main()
- Apply warp to UV before rotation/zoom (after line 20, before line 22)

### 2. Configuration

Add to `src/config/effect_config.h` after feedbackDesaturate (line 14):
```cpp
float warpStrength = 0.0f;
float warpScale = 5.0f;
int warpOctaves = 3;
float warpLacunarity = 2.0f;
float warpSpeed = 0.5f;
```

### 3. Uniform Wiring

Modify `src/render/post_effect.h`:
- Add 5 uniform location fields after feedbackDesaturateLoc (line 40)
- Add `float warpTime` state after voronoiTime (line 48)

Modify `src/render/post_effect.cpp`:
- Cache locations in `GetShaderUniformLocations()` (after line 198)
- Initialize `warpTime = 0.0f` in `PostEffectInit()` (after line 232)
- Accumulate `warpTime += deltaTime * warpSpeed` in `PostEffectBeginAccum()` (after line 302)
- Set 5 uniforms in `ApplyFeedbackPass()` (after line 102)

### 4. UI Controls

**Fix feedback rotation slider** in `src/ui/ui_panel_effects.cpp:27`:
- Change range from `0.0f, 0.02f` to `-0.02f, 0.02f` to allow counterclockwise rotation

**Add warp sliders** after kaleidoscope (after line 29):
```cpp
DrawLabeledSlider(l, "Warp", &effects->warpStrength, 0.0f, 0.2f, NULL);
if (effects->warpStrength > 0.0f) {
    DrawLabeledSlider(l, "W.Scale", &effects->warpScale, 1.0f, 20.0f, NULL);
    DrawIntSlider(l, "W.Octaves", &effects->warpOctaves, 1, 5, NULL);
    DrawLabeledSlider(l, "W.Lacun", &effects->warpLacunarity, 1.5f, 3.0f, NULL);
    DrawLabeledSlider(l, "W.Speed", &effects->warpSpeed, 0.1f, 2.0f, NULL);
}
```

### 5. Preset Serialization

Modify `src/config/preset.cpp:20-24` - append warp fields to EffectConfig macro:
```cpp
warpStrength, warpScale, warpOctaves, warpLacunarity, warpSpeed
```

## File Changes

| File | Change |
|------|--------|
| `shaders/feedback.fs` | Modify - Add uniforms, noise functions, warp logic |
| `src/config/effect_config.h` | Modify - Add 5 warp parameter fields |
| `src/render/post_effect.h` | Modify - Add 5 uniform locations + warpTime |
| `src/render/post_effect.cpp` | Modify - Cache locations, init/update time, set uniforms |
| `src/ui/ui_panel_effects.cpp` | Modify - Fix rotation range, add warp slider section |
| `src/config/preset.cpp` | Modify - Add warp fields to serialization |

## Validation

- [ ] Feedback rotation slider allows negative values (counterclockwise rotation works)
- [ ] Warp disabled (strength=0) produces identical output to current feedback
- [ ] Warp strength 0.1, scale 5, octaves 3, speed 0.5 creates visible flowing distortion
- [ ] Animation flows smoothly (no temporal aliasing or jitter)
- [ ] Physarum agents follow warped trail patterns when accumSenseBlend > 0
- [ ] Warp parameters save/load correctly in presets
- [ ] UI shows conditional sliders only when warpStrength > 0
- [ ] No performance regression when warp disabled
- [ ] Extreme values (strength=0.2, octaves=5) don't crash or produce NaN

## Implementation Notes

Changes made during implementation:

- **Warp strength range reduced:** 0.0-0.2 → 0.0-0.05. Original range was too intense; even 0.05 produces strong visible distortion due to feedback compounding.
- **noise2D output fixed:** Changed from [0, 1] to [-1, 1] range so warp flows in all directions instead of biasing toward one corner.
- **Physarum boost range increased:** 0.0-2.0 → 0.0-5.0 (unrelated quality-of-life tweak).

## References

- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/) - Original technique
- [Book of Shaders - fBM](https://thebookofshaders.com/13/) - Noise layering explanation
- `docs/research/advanced-shader-effects.md` - Background research for this feature
