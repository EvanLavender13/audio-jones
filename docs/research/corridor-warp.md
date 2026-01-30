# Corridor Warp

Projects the input texture onto an infinite floor, ceiling, or both (corridor/tunnel), creating a perspective illusion of depth stretching to a vanishing point. Animated rotation and scrolling simulate walking or spinning through an endless hallway.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Reorderable transforms stage (after feedback, before output)

## References

- [Fake Perspective UVs](https://halisavakis.com/shader-bits-world-space-reconstruction-orthographic-camera-texture-projection-fake-perspective-uvs/) - Core UV division technique
- [Mode 7 Shader (GitHub)](https://github.com/edwardOpich3/Mode-7-Shader/blob/master/Mode7%20Pixel%20Shader.glsl) - SNES-style floor projection with camera position and horizon
- [Infinite Plane Shader](https://gist.github.com/cmbruns/3c184d303e665ee2e987e4c1c2fe4b56) - Ray-plane intersection approach with tiling

## Algorithm

### Core Perspective Projection

Screen coordinates map to an infinite plane via division by vertical distance from horizon:

```glsl
// Normalize screen coords centered at origin
vec2 screen = (fragCoord - 0.5 * resolution) / resolution.y;

// Apply view rotation (spins whole scene)
screen *= rotate2d(viewRotation);

// Perspective division: project onto infinite plane
// horizon controls vanishing point position
float depth = screen.y - (horizon - 0.5);
vec2 planeUV = vec2(screen.x / depth, perspectiveStrength / depth);

// Apply floor rotation (spins just the texture)
planeUV *= rotate2d(planeRotation);

// Scale and scroll
planeUV *= scale;
planeUV += vec2(strafe, scroll);

// Sample input texture with projected UVs
vec4 color = texture(inputTex, fract(planeUV));

// Apply fog based on depth
float fog = pow(abs(depth), fogStrength);
color.rgb *= fog;
```

### Mode Handling

```glsl
// FLOOR mode: only render below horizon
if (mode == FLOOR && screen.y > horizon - 0.5) discard;

// CEILING mode: only render above horizon
if (mode == CEILING && screen.y < horizon - 0.5) discard;

// CORRIDOR mode: render both, flip UVs for ceiling
if (screen.y > horizon - 0.5) {
    depth = -depth;  // mirror projection
}
```

### Rotation Helper

```glsl
mat2 rotate2d(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| `horizon` | float | 0.0-1.0 | 0.5 | Vertical vanishing point position |
| `perspectiveStrength` | float | 0.5-2.0 | 1.0 | Depth convergence aggressiveness |
| `mode` | enum | FLOOR/CEILING/CORRIDOR | CORRIDOR | Which surfaces to project |
| `viewRotationSpeed` | float | -PI to PI | 0.0 | Scene rotation rate (rad/s) |
| `planeRotationSpeed` | float | -PI to PI | 0.0 | Floor texture rotation rate (rad/s) |
| `scale` | float | 0.5-10.0 | 2.0 | Texture tiling density |
| `scrollSpeed` | float | -2.0 to 2.0 | 0.0 | Forward/backward motion (units/s) |
| `strafeSpeed` | float | -2.0 to 2.0 | 0.0 | Side-to-side drift (units/s) |
| `fogStrength` | float | 0.0-4.0 | 1.0 | Distance fade intensity |

## Modulation Candidates

- **scrollSpeed**: Forward motion synced to beat creates rhythmic "walking" pulses
- **viewRotationSpeed**: Rotation intensity tied to bass energy for spinning sensation
- **fogStrength**: Depth fade pulsing with audio creates breathing depth effect
- **planeRotationSpeed**: Floor spin rate modulated by treble for hypnotic spirals
- **horizon**: Subtle oscillation creates head-bob or vertigo effect

## Notes

- When `depth` approaches zero (at horizon), UVs explode to infinity. Clamp or fade to avoid artifacts.
- Corridor mode requires mirroring the projection for ceiling; ensure UV continuity at the seam.
- High `scale` values with low `fogStrength` can look flat; fog sells the depth illusion.
- Consider blend mode to mix warped result with original at horizon edges.
