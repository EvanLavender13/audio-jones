# Solarize

Psychedelic tone inversion where colors flip past a threshold - dark becomes light, light becomes dark, creating surreal banding and color shifts like an overexposed darkroom print.

## Classification

- **Category**: TRANSFORMS > Color
- **Pipeline Position**: Transform chain (user-ordered), same slot as Color Grade / False Color

## References

- [Suricrasia Online - Interpolatable Colour Inversion](https://suricrasia.online/blog/interpolatable-colour-inversion/) - Animatable solar inversion with continuous control parameter
- [GPUImage Solarize (BradLarson)](https://github.com/BradLarson/GPUImage/blob/master/framework/Source/GPUImageSolarizeFilter.m) - Luminance-based threshold with hard step
- [Pillow ImageOps.solarize](https://github.com/python-pillow/Pillow/blob/main/src/PIL/ImageOps.py) - Per-channel threshold reference
- [Processing Forum - Solarization shader](https://discourse.processing.org/t/solarization-shader/21731) - Per-channel with independent thresholds

## Reference Code

GPUImage solarize (luminance-based, hard step):

```glsl
uniform sampler2D inputImageTexture;
uniform highp float threshold;
const highp vec3 W = vec3(0.2125, 0.7154, 0.0721);

void main() {
    highp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
    highp float luminance = dot(textureColor.rgb, W);
    highp float thresholdResult = step(luminance, threshold);
    highp vec3 finalColor = abs(thresholdResult - textureColor.rgb);
    gl_FragColor = vec4(finalColor, textureColor.w);
}
```

Suricrasia interpolatable solar inversion (V-type):

```glsl
vec3 solar_invert(vec3 color, float x) {
    float st = 1.-step(.5, x);
    return abs((color-st)*(2.*x+4.*st-3.)+1.);
}
// x=0.0: identity, x=0.5: full solarize (tent), x=1.0: full inversion
FragColor.xyz = solar_invert(FragColor.xyz, invertSlider);
```

Suricrasia A-type (inverted tent):

```glsl
FragColor.xyz = 1.-solar_invert(1.-FragColor.xyz, invertSlider);
```

Photoshop classic tent function (no threshold, fixed):

```glsl
vec3 solarized = 1.0 - abs(2.0 * color.rgb - 1.0);
```

Pillow per-channel threshold (reference algorithm):

```python
def solarize(image, threshold=128):
    lut = []
    for i in range(256):
        if i < threshold:
            lut.append(i)
        else:
            lut.append(255 - i)
    return _lut(image, lut)
```

## Algorithm

Use the Suricrasia `solar_invert` as the core - it is the only approach that interpolates smoothly between identity, solarize, and full inversion, making it ideal for modulation.

- Keep `solar_invert()` verbatim
- Add a threshold parameter that shifts the tent peak position: offset `color` by `(threshold - 0.5) * 2.0` before passing to `solar_invert`, then clamp output
- Replace `texture2D` with `texture()`
- Use `fragTexCoord` for UV
- Input from `inputTexture` uniform
- The `x` parameter becomes the `amount` uniform (0 = no effect, 0.5 = full solarize, 1.0 = full inversion)

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| amount | float | 0.0-1.0 | 0.5 | Inversion strength - 0=identity, 0.5=full solarize, 1.0=full inversion |
| threshold | float | 0.0-1.0 | 0.5 | Shifts where the tone inversion peaks - 0.5 is symmetric |

## Modulation Candidates

- **amount**: sweeps from normal through solarized to fully inverted - the full range creates dramatic tonal shifts on every beat
- **threshold**: shifts which tones get inverted - low values invert shadows, high values invert highlights, creating rolling color waves across the image

### Interaction Patterns

- **Resonance (amount + threshold)**: When amount is near 0.5 (full solarize), threshold shifts create maximum visual change - the entire color palette rotates. When amount is near 0.0 or 1.0, threshold barely matters because the effect is either off or fully inverted. Audio driving amount to 0.5 on beats while threshold drifts with a slow LFO creates moments where the color palette suddenly unlocks and swirls.

## Notes

- Extremely cheap - no texture lookups beyond the input, no loops. Pure per-pixel math.
- The Suricrasia approach is superior to the GPUImage hard-step version because it produces useful intermediate states when animated. The hard step creates a jarring binary flip.
- Per-channel threshold splitting (different threshold per R/G/B) could be a future extension but adds complexity. Start with uniform threshold.
