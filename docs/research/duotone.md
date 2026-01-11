# Duotone

Maps image luminance to a two-color gradient, creating a stylized high-contrast look. Shadow color fills dark regions, highlight color fills bright regions, with smooth blending between.

## Classification

- **Category**: TRANSFORMS → Color
- **Core Operation**: Luminance extraction via weighted RGB sum, linear interpolation between shadow and highlight colors
- **Pipeline Position**: Transform stage (user-ordered)

## References

- [Agate Dragon: Creating a Duotone Gradient Shader](https://agatedragon.blog/2024/01/06/duotone-gradient-shader/) - Complete GLSL tutorial with overlay blending variant

## Algorithm

### Luminance Extraction

Standard luminance weights based on human perception:

```glsl
const vec3 LUMINANCE_WEIGHTS = vec3(0.299, 0.587, 0.114);

float getLuminance(vec3 color) {
    return dot(color, LUMINANCE_WEIGHTS);
}
```

### Basic Duotone

Simple linear interpolation between two colors:

```glsl
uniform vec3 shadowColor;
uniform vec3 highlightColor;
uniform float intensity;

vec3 duotone(vec3 inputColor) {
    float lum = getLuminance(inputColor);
    vec3 toned = mix(shadowColor, highlightColor, lum);
    return mix(inputColor, toned, intensity);
}
```

### Overlay Blend Variant

For more contrast and punch, blend via overlay mode:

```glsl
float overlay(float a, float b) {
    if (a < 0.5) return 2.0 * a * b;
    return 1.0 - 2.0 * (1.0 - a) * (1.0 - b);
}

vec3 overlay(vec3 a, vec3 b) {
    return vec3(
        overlay(a.r, b.r),
        overlay(a.g, b.g),
        overlay(a.b, b.b)
    );
}

vec3 duotoneOverlay(vec3 inputColor) {
    float lum = getLuminance(inputColor);
    vec3 grey = vec3(lum);
    vec3 gradient = mix(shadowColor, highlightColor, lum);
    vec3 toned = overlay(grey, gradient);
    return mix(inputColor, toned, intensity);
}
```

### Gradient Direction (Optional)

For screen-space gradient instead of luminance-based:

```glsl
uniform float gradientAngle;

vec3 duotoneGradient(vec3 inputColor, vec2 uv) {
    vec2 dir = vec2(cos(gradientAngle), sin(gradientAngle));
    float t = dot(uv - 0.5, dir) + 0.5;
    vec3 gradient = mix(shadowColor, highlightColor, t);
    float lum = getLuminance(inputColor);
    vec3 grey = vec3(lum);
    vec3 toned = overlay(grey, gradient);
    return mix(inputColor, toned, intensity);
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| shadowColor | vec3 | 0–1 per channel | Color for dark regions |
| highlightColor | vec3 | 0–1 per channel | Color for bright regions |
| intensity | float | 0–1 | Blend between original and duotone |
| useOverlay | bool | — | Enable overlay blend mode for more contrast |

## Audio Mapping Ideas

- **shadowColor hue** ← Low frequency energy: Shift shadow toward warmer colors on bass
- **highlightColor hue** ← High frequency energy: Shift highlight toward cooler colors on treble
- **intensity** ← Beat detection: Pulse to full intensity on beats

## Notes

- Overlay blend mode produces more dramatic results but can crush blacks/whites
- Consider HSV color pickers in UI for intuitive shadow/highlight selection
- For animated color cycling, modulate hue while keeping saturation/value fixed
- Simple mode (no overlay) preserves more tonal detail in midtones
