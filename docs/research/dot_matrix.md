# Dot Matrix

Tiles the scene into a uniform grid and renders each cell as a soft-glow dot colored by the input texture at that cell's center. Bright pixels produce bright glowing dots; dark pixels fade to nothing. The result resembles an LED billboard or neon dot display — colored circles floating on black, each one a pixel-scale summary of the underlying image.

## Classification

- **Category**: TRANSFORMS > Graphic (alongside Halftone, Toon, Neon Glow)
- **Pipeline Position**: Transform pass, user-ordered with other transforms
- **Chosen Approach**: Balanced — single-pass fragment shader with inverse-cube glow kernel, no neighbor bleed. Cheap enough for real-time, visually rich glow falloff without the 25x cost of a neighbor loop.

## References

- [LED/Dot Matrix Shader — Godot Shaders](https://godotshaders.com/shader/led-dot-matrix-shader/) — Grid quantization, aspect correction, SDF circle mask, `mix()` compositing against background
- [Dot Matrix Shader (LED Screen Effect) — gameidea.org](https://gameidea.org/2024/02/15/dot-matrix-shader-led-screen-effect/) — Same technique with extracted helper functions, clearer breakdown of the `fract()` → `length()` → invert pipeline
- [Glow Shader in Shadertoy — inspirnathan](https://inspirnathan.com/posts/65-glow-shader-in-shadertoy/) — Inverse-distance glow kernel applied to SDF shapes, clamping strategy, intensity scaling
- User-provided Shadertoy reference — `GlowKern3(x, s) = s / (1 + s*x)^3`, per-cell SDF glyph with anti-aliased edge

## Algorithm

### Grid Setup

1. Compute aspect ratio to keep dots circular:
   ```
   ratio = vec2(1.0, resolution.x / resolution.y)
   ```
2. Scale UV into grid space:
   ```
   gridUV = fragTexCoord * dotScale * ratio
   ```
3. Apply rotation (speed-accumulated + static angle offset):
   ```
   rotatedUV = rotationMatrix(rotation) * gridUV
   ```
4. Snap to cell and find cell center:
   ```
   cellID   = floor(rotatedUV)
   cellFrac = fract(rotatedUV)
   ```
5. Compute the unrotated cell center for texture sampling:
   ```
   cellCenter = inverse(rotationMatrix) * (cellID + 0.5) / (dotScale * ratio)
   ```

### Texture Sampling

Sample input at cell center to get the dot's color:
```
texColor = texture(texture0, cellCenter).rgb
luma     = dot(texColor, vec3(0.299, 0.587, 0.114))
```

### Dot Shape — Soft Glow

Remap cell-local coordinates to [-1, 1]:
```
localUV = cellFrac * 2.0 - 1.0
dist    = length(localUV)
```

Apply inverse-cube glow kernel (from reference `GlowKern3`):
```
glow = softness / pow(1.0 + softness * dist, 3.0)
glow = clamp(glow, 0.0, 1.0)
```

`softness` controls how quickly the glow falls off. Higher values = tighter dot, lower values = wider bloom. The cubic denominator produces a natural light-like falloff that reads as "glowing" rather than the harsh circle of a step function.

### Compositing

Combine dot color, luminance intensity, glow mask, and brightness:
```
result = texColor * luma * glow * brightness
```

Output on black background:
```
finalColor = vec4(result, 1.0)
```

Dark scene regions produce near-zero luma, so those dots vanish naturally. Bright regions produce vivid glowing circles.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| dotScale | float | 4.0–80.0 | 32.0 | Grid resolution — low values = large dots, high = fine dot matrix |
| softness | float | 0.2–4.0 | 1.2 | Glow falloff tightness — low = wide soft bloom, high = compact bright dot |
| brightness | float | 0.5–8.0 | 3.0 | Overall dot intensity multiplier |
| rotationSpeed | float | -3.14–3.14 | 0.0 | Grid rotation rate (radians/second) |
| rotationAngle | float | -3.14–3.14 | 0.0 | Static grid rotation offset (radians) |

## Modulation Candidates

- **dotScale**: Scales the grid resolution — pumping this with audio produces a zoom-like pulse
- **softness**: Sweeps between tight sharp dots and diffuse glowing blobs
- **brightness**: Directly controls dot intensity — natural mapping to overall energy
- **rotationSpeed**: Spinning the dot grid

## Notes

- No neighbor bleed keeps this at one texture sample per pixel. If the user later wants glow bleeding across cells, a second pass or the 5x5 neighbor loop from the reference can be added as an enhancement.
- The rotation unproject (step 5) must use the inverse rotation to sample the correct texture location. The halftone shader already demonstrates this pattern with `transpose(m) * cell`.
- `dotScale` means "number of cells across the shorter dimension." Aspect ratio correction ensures circles, not ellipses.
- The glow kernel `s / (1+sx)^3` naturally clamps near 1.0 at the center and falls off smoothly — no branch or `if` needed, just a final `clamp` for safety.
