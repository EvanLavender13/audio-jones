# Cosine Palette

Procedural color generation using a cosine function with four vec3 parameters. Maps any float input to a smooth, cyclic RGB color without discrete gradient stops.

## Classification

- **Type**: General (ColorConfig mode extension)
- **Integration**: New `COLOR_MODE_PALETTE` alongside existing Solid, Rainbow, Gradient modes

## References

- [Inigo Quilez - Palettes](https://iquilezles.org/articles/palettes/) - Original technique, parameter guidance, examples

## Algorithm

The formula generates RGB from a single float input:

```
color(t) = a + b * cos(2π * (c * t + d))
```

Where:
- **t**: Input value (0-1), typically position along waveform or agent index ratio
- **a**: Bias (vec3) — controls overall brightness center
- **b**: Amplitude (vec3) — controls contrast/range of each channel
- **c**: Frequency (vec3) — how many times the palette cycles per unit t
- **d**: Phase (vec3) — shifts each channel's starting point

### GLSL Implementation

```glsl
vec3 cosinePalette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {
    return a + b * cos(6.283185 * (c * t + d));
}
```

### Parameter Behavior

| Parameter | Effect |
|-----------|--------|
| a = (0.5, 0.5, 0.5) | Mid-gray center point |
| b = (0.5, 0.5, 0.5) | Full range (0-1) oscillation |
| c = (1, 1, 1) | One complete cycle over t=0-1 |
| d = (0, 0.33, 0.67) | RGB phases offset by 1/3 each (rainbow) |

### Example Palettes

| Name | a | b | c | d |
|------|---|---|---|---|
| Rainbow | (0.5, 0.5, 0.5) | (0.5, 0.5, 0.5) | (1, 1, 1) | (0, 0.33, 0.67) |
| Sunset | (0.5, 0.5, 0.5) | (0.5, 0.5, 0.5) | (1, 1, 1) | (0, 0.1, 0.2) |
| Fire | (0.5, 0.5, 0.5) | (0.5, 0.5, 0.5) | (1, 1, 0.5) | (0.8, 0.9, 0.3) |
| Ocean | (0.5, 0.5, 0.5) | (0.5, 0.5, 0.5) | (1, 0.7, 0.4) | (0, 0.15, 0.2) |
| Neon | (0.5, 0.5, 0.5) | (0.5, 0.5, 0.5) | (2, 1, 0) | (0.5, 0.2, 0.25) |
| Earth | (0.8, 0.5, 0.4) | (0.2, 0.4, 0.2) | (2, 1, 1) | (0, 0.25, 0.25) |

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| a (bias) | vec3 | 0-1 each | (0.5, 0.5, 0.5) | Shifts brightness center |
| b (amplitude) | vec3 | 0-1 each | (0.5, 0.5, 0.5) | Controls color intensity range |
| c (frequency) | vec3 | 0-4 each | (1, 1, 1) | Number of color cycles |
| d (phase) | vec3 | 0-1 each | (0, 0.33, 0.67) | Starting color offset per channel |

## Modulation Candidates

- **d (phase)**: Animates color shift along the palette
- **c (frequency)**: Changes color density/complexity
- **a (bias)**: Pulses overall brightness

## Notes

- For seamless cycling (t wraps from 1 to 0), use integer values for c
- c values of 0.5 multiples create half-cycle palettes (useful for gradient-like appearance)
- Lower b values produce more pastel/muted colors; higher values produce more saturated colors
- The formula naturally clamps to valid RGB since cos outputs -1 to 1, and a=0.5, b=0.5 maps this to 0-1
