# Perspective Tilt

A general-purpose transform that projects the input texture onto a tilted 3D plane. Pitch and yaw sliders rotate the viewing angle while perspective division creates foreshortening — the far edge compresses, the near edge stretches. An auto-zoom compensates for tilt to keep the frame filled at any angle.

## Classification

- **Category**: TRANSFORMS > Optical
- **Pipeline Position**: Reorderable transform chain (between generators and output)
- **Chosen Approach**: Balanced — full pitch/yaw/roll control with FOV and auto-zoom, single-pass fragment shader

## References

- [2D Perspective Shader – Godot Shaders](https://godotshaders.com/shader/2d-perspective/) — Lifts UV to 3D, applies YXZ rotation matrix, perspective divides back to 2D. Includes FOV via tangent scaling and auto-zoom via inset parameter.
- [Planar Projection Mapping – WebGL Fundamentals](https://webglfundamentals.org/webgl/lessons/webgl-planar-projection-mapping.html) — Texture matrix approach: inverse world transform × projection matrix × bias/scale, with `xyz / w` perspective division in the fragment shader.

## Algorithm

### Projection Pipeline

1. **Center and aspect-correct**: `p = (UV - 0.5) * vec2(aspect, 1.0)`
2. **Lift to 3D ray**: `vec3 ray = vec3(p, 0.5 / tan(fov * PI / 360.0))` — the z-component encodes FOV. Low FOV → large z → mild foreshortening. High FOV → small z → dramatic perspective.
3. **Rotate by pitch** (X-axis): standard 2D rotation applied to `ray.yz`
4. **Rotate by yaw** (Y-axis): standard 2D rotation applied to `ray.xz`
5. **Rotate by roll** (Z-axis, optional): standard 2D rotation applied to `ray.xy`
6. **Perspective divide**: `vec2 tilted = ray.xy / ray.z`
7. **Auto-zoom**: scale `tilted` so the visible frame stays filled (see below)
8. **Map to texture UV**: `uv = tilted + 0.5`
9. **Bounds check**: if `ray.z <= 0.0` or `uv` outside `[0, 1]`, output black (back-face cull + no tiling)

### Rotation Matrix

Combined YXZ rotation (yaw then pitch then roll):

```
cos_y = cos(yaw),   sin_y = sin(yaw)
cos_p = cos(pitch), sin_p = sin(pitch)

R[0] = ( cos_y,              sin_y * sin_p,        sin_y * cos_p      )
R[1] = ( 0,                  cos_p,                -sin_p             )
R[2] = (-sin_y,              cos_y * sin_p,         cos_y * cos_p     )
```

Apply as: `ray = R * vec3(p.x, p.y, z_fov)`

### Auto-Zoom

Compute the projected position of all four screen corners through the same rotation + perspective divide. Find the bounding box of those four projected points. Scale `tilted` by the inverse of the half-extent so the warped image always covers the full screen:

```
zoom = max(abs(corner_projected.x), abs(corner_projected.y)) for all 4 corners
tilted /= zoom
```

At zero tilt, zoom = 1.0 (identity). As tilt increases, zoom grows and the shader samples a tighter crop of the texture to compensate for stretching.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| pitch | float | -90..90 deg | 0 | Forward/backward tilt — top compresses or bottom compresses |
| yaw | float | -90..90 deg | 0 | Left/right tilt — left side or right side compresses |
| roll | float | -180..180 deg | 0 | In-plane rotation of the tilted view |
| fov | float | 20..120 deg | 60 | Perspective intensity — low = subtle skew, high = dramatic fisheye-like foreshortening |
| autoZoom | bool | on/off | on | When on, auto-scales to fill frame. When off, black appears at stretched edges. |

## Modulation Candidates

- **pitch**: tilting forward/back creates a rocking or breathing motion
- **yaw**: left/right sway, pairs with pitch for orbital camera movement
- **fov**: pulsing FOV creates a dolly-zoom / vertigo effect — perspective shifts while framing stays similar
- **roll**: spinning the tilted plane, dramatic at high tilt angles

### Interaction Patterns

- **Competing Forces**: pitch and yaw oppose each other for screen coverage — high pitch compresses vertically while high yaw compresses horizontally. Modulating both creates a swaying, shifting perspective where the dominant tilt axis changes with the music.
- **Resonance**: fov and pitch/yaw amplify each other — a FOV spike at the same moment as a pitch spike produces an extreme perspective lurch that neither alone achieves. Rare musical peaks trigger dramatic camera moves.

## Notes

- At extreme tilt (approaching 90 degrees), `ray.z` approaches zero and the projection blows up. Clamp the effective angle to ~85 degrees to avoid division artifacts.
- Auto-zoom at high tilt crops aggressively. The user may want to toggle it off for the "floating plane in black" look.
- Single-pass effect, no render textures needed. Low GPU cost.
- Roll without pitch/yaw is just a flat rotation — only visually interesting when combined with tilt.
