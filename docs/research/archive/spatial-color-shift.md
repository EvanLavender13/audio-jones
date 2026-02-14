# Hue Remap

Replaces the standard HSV hue wheel with a custom color wheel defined by ColorConfig. A "shift" slider rotates through the custom palette exactly like Color Grade's hue shift rotates through the rainbow — one slider, everything changes. Radial blending controls how much the remap applies based on distance from a configurable center.

## Classification

- **Category**: TRANSFORMS > Color
- **Pipeline Position**: Output stage, user-ordered transform chain (alongside Color Grade, False Color, Palette Quantization)
- **Technique**: Single-pass fragment shader with LUT texture

## References

- Existing codebase: `shaders/color_grade.fs` — HSV conversion functions (rgb2hsv, hsv2rgb)
- Existing codebase: `src/render/color_lut.h` — ColorLUT for getting ColorConfig into a shader as a 1D texture
- Existing codebase: `src/effects/false_color.h` — proven pattern for ColorConfig + LUT in a transform effect
- Existing codebase: `shaders/feedback.fs` — radial field math (aspect-corrected distance from center)

## Algorithm

1. Sample input pixel, convert to HSV
2. Use hue (0-1) as `t`, add shift offset, sample ColorConfig LUT to get the remapped color
3. Preserve original saturation and value — only the hue mapping changes
4. Blend between original pixel and remapped pixel using the radial field

```
pixelHSV = rgb2hsv(pixel.rgb)
t = fract(pixelHSV.x + shift)
remappedRGB = texture(gradientLUT, vec2(t, 0.5)).rgb
remappedHSV = rgb2hsv(remappedRGB)

// Keep original saturation and value, take remapped hue
result = hsv2rgb(vec3(remappedHSV.x, pixelHSV.y, pixelHSV.z))

// Radial blend: mix between original and remapped
rad = length(aspect-corrected(fragTexCoord - center)) * 2.0
blend = clamp(intensity + radial * rad, 0.0, 1.0)  // future: + angular + luminance terms
finalColor = mix(pixel.rgb, result, blend)
```

When ColorConfig is in RAINBOW mode, this behaves identically to standard hue shift.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| gradient | ColorConfig | — | Rainbow | Custom color wheel |
| shift | float | 0.0-1.0 | 0.0 | Rotates through the custom palette (like hue shift) |
| intensity | float | 0.0-1.0 | 1.0 | Global blend strength (0 = no remap, 1 = full remap) |
| radial | float | -1.0 to 1.0 | 0.0 | Blend varies with distance from center |
| cx | float | 0.0-1.0 | 0.5 | Center X |
| cy | float | 0.0-1.0 | 0.5 | Center Y |

## Modulation Candidates

- **shift**: The primary target. Modulate to continuously rotate through the custom palette — same feel as modulating Color Grade hue shift but with your own color wheel.
- **intensity**: Sweep the whole effect on/off.
- **radial**: Expand/contract how much the edges differ from center.
- **cx/cy**: Drift the radial center around.

### Interaction Patterns

**Competing forces**: `intensity` at 0.5 with `radial` at -0.5 means center gets full remap while edges get none. Modulating `intensity` shifts the boundary — at 1.0 everything is remapped, at 0.0 only the very center is. This creates a spatial "wash" that expands and contracts with the music.

## Notes

- Reuses existing ColorLUT system — no new texture infrastructure needed
- When gradient is set to RAINBOW mode, the effect is identical to standard hue shift — no functional difference
- Preserving original saturation and value means low-saturation and dark pixels naturally stay unaffected — no content gate needed
- The radial field is optional (intensity defaults to 1.0, radial to 0.0) so it works as a simple global remap out of the box
