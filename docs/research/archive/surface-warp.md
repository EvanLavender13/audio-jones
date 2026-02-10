# Surface Warp

Creates the illusion of a 3D undulating surface by stretching UV coordinates based on layered sine waves. Peaks appear closer, valleys recede. Optional depth shading darkens the valleys to enhance the effect.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Output stage, transforms (user order)

## References

- Shadertoy examples by plento and Cole Peterson (user-provided) - multiplicative UV remapping technique
- [Lode's CG Tutorial - Image Filtering](https://lodev.org/cgtutor/filtering.html) - convolution and image transformation fundamentals

## Algorithm

### Step 1: Rotate Into Warp Space

Apply rotation so the warp always operates along a consistent axis:

```glsl
float effectiveAngle = angle + rotateSpeed * time;
mat2 r = mat2(cos(effectiveAngle), -sin(effectiveAngle),
              sin(effectiveAngle), cos(effectiveAngle));
vec2 ruv = (uv - 0.5) * r;  // rotate around center
```

### Step 2: Compute Wave Height

Sum multiple sine waves at irrational frequency ratios for organic, non-repeating motion:

```glsl
float t = scrollSpeed * time;
float wave = sin(ruv.x * 2.0 + t * 0.7) * 0.5
           + sin(ruv.x * 3.7 + t * 1.1) * 0.3
           + sin(ruv.x * 5.3 + t * 0.4) * 0.2;
```

### Step 3: Remap Perpendicular Axis

Multiply the perpendicular axis by an exponential of the wave. This stretches "peaks" and compresses "valleys":

```glsl
float m = exp(abs(wave) * intensity);
ruv.y *= m;
```

### Step 4: Rotate Back and Sample

```glsl
vec2 warpedUV = ruv * mat2(cos(-effectiveAngle), -sin(-effectiveAngle),
                           sin(-effectiveAngle), cos(-effectiveAngle)) + 0.5;
vec4 color = texture(texture0, warpedUV);
```

### Step 5: Apply Depth Shading (Optional)

Darken valleys based on the inverse of the stretch factor:

```glsl
float shade = mix(1.0, smoothstep(0.0, 2.0, m), depthShade);
color.rgb *= shade;
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| intensity | float | 0.0 - 2.0 | 0.5 | Hill steepness (0 = flat, 2 = dramatic) |
| angle | float | 0 - 2π | 0 | Base warp direction |
| rotateSpeed | float | -π to π | 0 | Direction rotation rate (rad/s) |
| scrollSpeed | float | -2.0 to 2.0 | 0.5 | Wave drift speed |
| depthShade | float | 0.0 - 1.0 | 0.3 | Valley darkening amount |

## Modulation Candidates

- **intensity**: Bass energy creates dramatic peaks on hits
- **rotateSpeed**: Slowly rotating hills, or beat-synced direction shifts
- **scrollSpeed**: Tempo-linked drift

## Notes

- The layered sine approach (2.0, 3.7, 5.3 frequencies) creates organic motion without noise sampling overhead.
- `exp(abs(wave))` ensures the multiplier is always >= 1, preventing UV inversion artifacts.
- Depth shading is subtle but sells the 3D illusion - without it the effect reads as "stretchy" rather than "hilly."
- Consider clamping warped UVs or using mirror/repeat addressing to handle edge sampling.
