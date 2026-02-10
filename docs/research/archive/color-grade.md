# Color Grade

Full-spectrum color manipulation effect: hue rotation, saturation, brightness/contrast, temperature, and shadow/midtone/highlight control via lift/gamma/gain. All parameters slider-based for audio modulation.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Sample input texture at UV, transform RGB through color space operations, output modified color
- **Pipeline Position**: Output stage with other transforms (user-reorderable)

## References

- [GPU Gems Ch.22: Color Controls](https://developer.nvidia.com/gpugems/gpugems/part-iv-image-processing/chapter-22-color-controls) - Levels correction, saturation via luminance, hue rotation around (1,1,1) diagonal
- [Filmic Worlds: Minimal Color Grading Tools](http://filmicworlds.com/blog/minimal-color-grading-tools/) - Lift/gamma/gain formulas, log-space contrast, saturation
- [Catlike Coding: Color Grading](https://catlikecoding.com/unity/tutorials/custom-srp/color-grading/) - Post-exposure, white balance, contrast in log space, saturation, channel mixer
- [trevorvanhoof/ColorGrading](https://github.com/trevorvanhoof/ColorGrading) - GLSL implementation of lift/gamma/gain, hue shift, saturation, contrast

## Algorithm

### RGB to HSV Conversion

```glsl
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
```

### Hue Shift

```glsl
vec3 hueShift(vec3 color, float shift) {
    vec3 hsv = rgb2hsv(color);
    hsv.x = fract(hsv.x + shift);  // shift is 0-1 (normalized from degrees)
    return hsv2rgb(hsv);
}
```

### Saturation

Lerp between grayscale and original color. Luminance weights from Filmic Worlds (perceptually balanced for games):

```glsl
vec3 applySaturation(vec3 color, float saturation) {
    vec3 lumaWeights = vec3(0.25, 0.50, 0.25);
    float grey = dot(lumaWeights, color);
    return vec3(grey) + saturation * (color - vec3(grey));
}
```

- `saturation = 0`: grayscale
- `saturation = 1`: original
- `saturation > 1`: oversaturated

### Brightness (Exposure)

Multiply by power of 2 for F-stop-based control:

```glsl
vec3 applyBrightness(vec3 color, float exposure) {
    return color * exp2(exposure);
}
```

### Log-Space Contrast

Preserves shadow detail by operating in log space (from Filmic Worlds):

```glsl
float logContrast(float x, float contrast) {
    float eps = 1e-6;
    float logMidpoint = log2(0.18);  // standard 18% grey
    float logX = log2(x + eps);
    float adjX = logMidpoint + (logX - logMidpoint) * contrast;
    return max(0.0, exp2(adjX) - eps);
}

vec3 applyContrast(vec3 color, float contrast) {
    return vec3(
        logContrast(color.r, contrast),
        logContrast(color.g, contrast),
        logContrast(color.b, contrast)
    );
}
```

### Temperature (White Balance)

Approximate warm/cool shift by scaling RGB channels:

```glsl
vec3 applyTemperature(vec3 color, float temp) {
    // temp: -1 (cool/blue) to +1 (warm/orange)
    // Approximation: boost R and reduce B for warm, inverse for cool
    float warmth = temp * 0.3;
    return color * vec3(1.0 + warmth, 1.0, 1.0 - warmth);
}
```

For more accurate results, convert to LMS color space, but the RGB approximation works for artistic purposes.

### Lift/Gamma/Gain (Shadows/Midtones/Highlights)

Core formula from Filmic Worlds:

```glsl
float liftGammaGain(float x, float lift, float gamma, float gain) {
    float lerpV = clamp(pow(x, gamma), 0.0, 1.0);
    return gain * lerpV + lift * (1.0 - lerpV);
}

vec3 applyShadowsMidtonesHighlights(vec3 color, vec3 shadows, vec3 midtones, vec3 highlights) {
    // shadows = lift offset (-0.5 to 0.5), midtones = gamma power, highlights = gain offset
    vec3 lift = shadows;
    vec3 gain = vec3(1.0) + highlights;
    vec3 gamma = vec3(1.0) / max(vec3(0.01), vec3(1.0) + midtones);

    return vec3(
        liftGammaGain(color.r, lift.r, gamma.r, gain.r),
        liftGammaGain(color.g, lift.g, gamma.g, gain.g),
        liftGammaGain(color.b, lift.b, gamma.b, gain.b)
    );
}
```

### Combined Pipeline

Apply operations in this order for best results:

```glsl
vec3 colorGrade(vec3 color, ColorGradeConfig cfg) {
    // 1. Brightness/Exposure first (linear operation)
    color = applyBrightness(color, cfg.brightness);

    // 2. Temperature shift
    color = applyTemperature(color, cfg.temperature);

    // 3. Contrast (in log space, preserves shadows)
    color = applyContrast(color, cfg.contrast);

    // 4. Saturation
    color = applySaturation(color, cfg.saturation);

    // 5. Lift/Gamma/Gain (shadows/midtones/highlights)
    color = applyShadowsMidtonesHighlights(color,
        vec3(cfg.shadowsOffset),
        vec3(cfg.midtonesOffset),
        vec3(cfg.highlightsOffset));

    // 6. Hue shift last (operates on final color relationships)
    color = hueShift(color, cfg.hueShift);

    return clamp(color, 0.0, 1.0);
}
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| hueShift | float | 0–1 (display 0–360°) | 0 | Rotate color spectrum |
| saturation | float | 0–2 | 1 | Grayscale↔oversaturated |
| brightness | float | -2 to +2 | 0 | Exposure in F-stops |
| contrast | float | 0.5–2 | 1 | Log-space contrast around 18% grey |
| temperature | float | -1 to +1 | 0 | Cool (blue) ↔ warm (orange) |
| shadowsOffset | float | -0.5 to +0.5 | 0 | Lift black point |
| midtonesOffset | float | -0.5 to +0.5 | 0 | Gamma/midtone adjustment |
| highlightsOffset | float | -0.5 to +0.5 | 0 | Gain/highlight adjustment |

## Audio Mapping Ideas

- **Hue Shift**: Map to bass energy for spectrum cycling on low-frequency hits
- **Saturation**: Pulse with beat detection — desaturate on quiet, saturate on loud
- **Brightness**: Subtle breathing tied to overall energy level
- **Contrast**: Push on transients for punch, relax between beats
- **Temperature**: Tie to frequency centroid — bass-heavy = warm, treble-heavy = cool
- **Shadows/Midtones/Highlights**: Independent modulation creates color "washing" effects

## Notes

- Order of operations matters: brightness before contrast, saturation before hue shift
- Log-space contrast avoids crushing blacks (standard linear contrast clips shadows)
- Simplified temperature uses RGB scaling; more accurate white balance requires LMS color space conversion
- All parameters are independent scalars — no color wheels needed for the slider-based interface
- Consider adding per-channel saturation (HSL per-hue shifting) as a future extension
