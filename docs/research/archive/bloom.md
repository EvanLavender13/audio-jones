# Bloom

Soft glow effect that extracts bright regions, blurs them through a resolution pyramid, and composites back additively. Creates the "glowing highlights" look common in HDR rendering and music visualizers.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Threshold bright pixels, blur via downsample/upsample pyramid, add back to original
- **Pipeline Position**: Reorderable with other transforms

## References

- [frost.kiwi - Video Game Blurs](https://blog.frost.kiwi/dual-kawase/) - Complete dual Kawase downsample/upsample shader code
- [LearnOpenGL - Bloom](https://learnopengl.com/Advanced-Lighting/Bloom) - Brightness threshold and composite implementation
- [Catlike Coding - Bloom](https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/) - Soft threshold knee formula
- [ARM SIGGRAPH 2015](https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_notes.pdf) - Original dual filter algorithm, performance data

## Algorithm

### Brightness Threshold

Extract pixels above brightness threshold. Uses luminance weights for perceptual accuracy:

```glsl
float Luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 BrightPass(vec3 color, float threshold)
{
    float brightness = Luminance(color);
    return (brightness > threshold) ? color : vec3(0.0);
}
```

### Soft Threshold (Knee)

Smooth transition instead of hard cutoff. The knee parameter controls gradient width:

```glsl
vec3 SoftThreshold(vec3 color, float threshold, float knee)
{
    float brightness = max(color.r, max(color.g, color.b));
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);
    return color * contribution;
}
```

### Dual Kawase Downsample

Samples center (4x weight) plus four diagonal corners (1x each). Halves resolution per pass.

```glsl
vec3 Downsample(sampler2D tex, vec2 uv, vec2 halfpixel)
{
    vec3 sum = texture(tex, uv).rgb * 4.0;
    sum += texture(tex, uv + vec2(-halfpixel.x, -halfpixel.y)).rgb;
    sum += texture(tex, uv + vec2( halfpixel.x, -halfpixel.y)).rgb;
    sum += texture(tex, uv + vec2(-halfpixel.x,  halfpixel.y)).rgb;
    sum += texture(tex, uv + vec2( halfpixel.x,  halfpixel.y)).rgb;
    return sum / 8.0;
}
```

### Dual Kawase Upsample

Four edge samples (1x) plus four diagonal corners (2x). Doubles resolution per pass.

```glsl
vec3 Upsample(sampler2D tex, vec2 uv, vec2 halfpixel)
{
    vec3 sum = vec3(0.0);

    // Edge samples (1x weight)
    sum += texture(tex, uv + vec2(-halfpixel.x * 2.0, 0.0)).rgb;
    sum += texture(tex, uv + vec2( halfpixel.x * 2.0, 0.0)).rgb;
    sum += texture(tex, uv + vec2(0.0, -halfpixel.y * 2.0)).rgb;
    sum += texture(tex, uv + vec2(0.0,  halfpixel.y * 2.0)).rgb;

    // Diagonal samples (2x weight)
    sum += texture(tex, uv + vec2(-halfpixel.x,  halfpixel.y)).rgb * 2.0;
    sum += texture(tex, uv + vec2( halfpixel.x,  halfpixel.y)).rgb * 2.0;
    sum += texture(tex, uv + vec2(-halfpixel.x, -halfpixel.y)).rgb * 2.0;
    sum += texture(tex, uv + vec2( halfpixel.x, -halfpixel.y)).rgb * 2.0;

    return sum / 12.0;
}
```

### Pyramid Structure

1. **Prefilter**: Apply threshold to input, write to mip 0
2. **Downsample chain**: For each level 1..N, downsample from previous level (resolution halves)
3. **Upsample chain**: For each level N-1..0, upsample and add to that level's downsample result
4. **Composite**: Add final bloom texture to original input

Typical iteration count: 4-6 levels (16x-64x total blur spread).

### Composite

```glsl
vec3 result = original + bloom * intensity;
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| threshold | float | 0.0 - 2.0 | Brightness cutoff for bloom extraction |
| knee | float | 0.0 - 1.0 | Soft threshold gradient (0 = hard, 1 = smooth) |
| intensity | float | 0.0 - 2.0 | Bloom brightness in final composite |
| iterations | int | 3 - 8 | Pyramid depth (blur spread = 2^iterations pixels) |

## Audio Mapping Ideas

- **Beat → Intensity**: Flash bloom brighter on kicks
- **Bass Energy → Threshold**: Lower threshold on bass hits (more pixels bloom)
- **RMS → Knee**: Louder = softer threshold transition
- **Spectral Centroid → Iterations**: Brighter sounds = wider blur spread

## Implementation Notes

### Render Target Requirements

Dual Kawase requires a mipmap chain or separate render textures at each resolution level. For N iterations:
- 1 prefilter texture (full res)
- N downsample textures (1/2, 1/4, 1/8... resolution)
- Can reuse downsample textures for upsample if ping-ponging carefully

### Performance

ARM's testing shows dual Kawase uses ~2% memory bandwidth vs 98% for linear sampling at equivalent blur radius. The technique scales to arbitrary blur sizes with minimal cost increase.

### Synergy with Lens Flare and Bokeh

All three effects enhance bright regions differently:
- **Bloom**: Soft glow bleeding outward
- **Lens Flare**: Ghost artifacts mirrored across frame
- **Bokeh**: Circular highlights from point sources

Order matters for artistic intent:
- Bloom before lens flare: Flare ghosts appear on bloomed glow
- Lens flare before bloom: Bloom softens the flare artifacts
- Bokeh typically last: Creates final highlight circles on everything
