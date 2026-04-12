# Film Grain

Photographic noise with luminance-dependent density - thick speckle in shadows, clean highlights - that shimmers temporally like analog film stock.

## Classification

- **Category**: TRANSFORMS > Retro
- **Pipeline Position**: Transform chain (user-ordered)

## References

- [CeeJay FilmGrain.fx (ReShade)](https://github.com/killashok/GShade-Shaders/blob/master/Shaders/FilmGrain.fx) - Box-Muller Gaussian noise with SNR power curve
- [Martins Upitis FilmGrain2.fx](https://github.com/Mortalitas/GShade/blob/master/Shaders/FilmGrain2.fx) - Per-channel Perlin noise with smoothstep luminance response (CC-BY 3.0)
- [prod80 PD80_06_Film_Grain.fx](https://github.com/prod80/prod80-ReShade-Repository/blob/master/Shaders/PD80_06_Film_Grain.fx) - Shadow/highlight split, hue-aware density (MIT)

## Reference Code

CeeJay FilmGrain (Box-Muller + SNR power curve):

```hlsl
uniform float Intensity = 0.50;  // 0.0 - 1.0
uniform float Variance = 0.40;   // 0.0 - 1.0
uniform float Mean = 0.5;        // 0.0 - 1.0
uniform int SignalToNoiseRatio = 6; // 0 - 16

float3 FilmGrainPass(float4 vpos, float2 texcoord)
{
    float3 color = tex2D(BackBuffer, texcoord).rgb;

    // Inverted luminance: 1.0 in shadows, 0.0 in highlights
    const float inv_luma = dot(color, float3(-1.0/3.0, -1.0/3.0, -1.0/3.0)) + 1.0;

    const float t = Timer * 0.0022337;

    // Pseudo-random seed from screen position
    const float seed = dot(texcoord, float2(12.9898, 78.233));
    const float sine = sin(seed);
    const float cosine = cos(seed);
    float uniform_noise1 = frac(sine * 43758.5453 + t);
    const float uniform_noise2 = frac(cosine * 53758.5453 - t);

    // Luminance-dependent variance via power curve
    float stn = (SignalToNoiseRatio != 0)
        ? pow(abs(inv_luma), float(SignalToNoiseRatio))
        : 1.0;
    const float variance = (Variance * Variance) * stn;

    // Box-Muller transform: uniform -> Gaussian
    if (uniform_noise1 < 0.0001) uniform_noise1 = 0.0001;
    float r = sqrt(-log(uniform_noise1));
    const float theta = (2.0 * 3.1415927) * uniform_noise2;
    const float gauss_noise1 = variance * r * cos(theta) + Mean;

    // Multiplicative grain blend
    const float grain = lerp(1.0 + Intensity, 1.0 - Intensity, gauss_noise1);
    color = color * grain;

    return color;
}
```

Upitis per-channel color noise (different rotation angles per channel):

```hlsl
float2 rotCoordsR = coordRot(texCoord, timer + 1.425);
float2 rotCoordsG = coordRot(texCoord, timer + 3.892);
float2 rotCoordsB = coordRot(texCoord, timer + 5.835);
noise.r = pnoise3D(float3(rotCoordsR * screenSize / grainsize, 0.0));
noise.g = lerp(noise.r, pnoise3D(float3(rotCoordsG * screenSize / grainsize, 1.0)), coloramount);
noise.b = lerp(noise.r, pnoise3D(float3(rotCoordsB * screenSize / grainsize, 2.0)), coloramount);
```

Upitis luminance response curve:

```hlsl
float luminance = dot(col, float3(0.299, 0.587, 0.114));
float lum = smoothstep(0.2, 0.0, luminance) + luminance;
noise = lerp(noise, 0.0, pow(lum, 4.0));
```

## Algorithm

Use the CeeJay approach - cheap Box-Muller Gaussian noise with the SNR power curve for luminance-dependent grain density.

- Keep the Box-Muller transform verbatim: `sqrt(-log(u1))`, `cos(2*PI*u2)`
- Keep the SNR power curve: `pow(inv_luma, snr)` - this is the key perceptual accuracy mechanism
- Keep the multiplicative blend: `lerp(1+intensity, 1-intensity, gaussNoise)` - more filmic than additive
- Replace `tex2D` with `texture()`, `lerp` with `mix`
- Replace `Timer` with `time` uniform
- Replace `BackBuffer` with `inputTexture`
- Use `fragTexCoord` for UV
- For per-channel color noise: generate 3 independent Gaussian samples using different seed offsets (cheaper than Upitis's 3x Perlin). Use `sin(seed)`, `sin(seed + 1.0)`, `sin(seed + 2.0)` with a `colorAmount` lerp between mono and per-channel.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| intensity | float | 0.0-1.0 | 0.35 | Grain visibility - how strong the noise multiplier swings |
| variance | float | 0.0-1.0 | 0.4 | Gaussian spread - lower is smoother, higher is punchier |
| snr | float | 0.0-16.0 | 6.0 | Signal-to-noise ratio power - higher suppresses grain in highlights more |
| colorAmount | float | 0.0-1.0 | 0.0 | 0 = monochrome grain, 1 = full per-channel color noise |

## Modulation Candidates

- **intensity**: grain fades in and out - subtle texture in quiet passages, heavy analog noise on loud sections
- **variance**: controls grain character from smooth/subtle to harsh/punchy
- **snr**: shifts where grain concentrates - low SNR puts grain everywhere, high SNR pushes it into shadows only
- **colorAmount**: mono grain feels like black-and-white film, color grain feels like cheap expired film stock

### Interaction Patterns

- **Competing forces (intensity vs snr)**: High intensity wants to cover everything in grain, but high SNR fights back by suppressing grain in highlights. The result depends on the image content - bright scenes get cleaned up while dark scenes stay noisy. Audio driving intensity up while SNR stays high creates grain that only appears in the dark areas of the visual, tracking shadow regions.

## Notes

- Very cheap - one sin/cos pair and a few exp/log per pixel. No loops, no texture lookups beyond input.
- The CeeJay multiplicative blend (`color * grain`) is superior to additive for film simulation because it scales with brightness - dark pixels get small absolute changes, bright pixels get large ones, matching real photographic grain response.
- `Mean` from the reference (default 0.5) centers the Gaussian output. This can stay hardcoded at 0.5 - there's no artistic reason to shift it.
