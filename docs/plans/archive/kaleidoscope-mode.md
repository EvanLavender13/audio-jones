# Kaleidoscope Mode

Post-process fragment shader that mirrors the visual output into radial segments around the screen center.

## Goal

Add a kaleidoscope effect that creates mandala-like symmetric patterns from existing waveform/bloom/trail output. The effect transforms UV coordinates to polar space, divides into segments, and mirrors content within each segment.

Prior art: [Daniel Ilett's Kaleidoscope Tutorial](https://danielilett.com/2020-02-19-tut3-8-crazy-kaleidoscopes/), [LYGIA Shader Library](https://lygia.xyz/space/kaleidoscope), [Shadertoy Kaleidoscope Visualizer](https://www.shadertoy.com/view/ldsXWn).

## Current State

- Post-effect pipeline uses ping-pong render textures (`accumTexture` ↔ `tempTexture`)
- Final composite in `PostEffectToScreen()` applies chromatic aberration before drawing to screen
- Existing shaders: `blur_h.fs`, `blur_v.fs`, `chromatic.fs`, `feedback.fs`
- `EffectConfig` struct holds all effect parameters with JSON serialization
- UI controls in `src/ui/ui_panel_effects.cpp`

## Algorithm

Kaleidoscope transforms UV coordinates through four steps:

### 1. Center UV Coordinates
Shift origin from corner to screen center:
```glsl
vec2 uv = fragTexCoord - 0.5;  // Range: -0.5 to 0.5
```

### 2. Convert to Polar Coordinates
```glsl
float radius = length(uv);
float angle = atan(uv.y, uv.x);  // Range: -PI to PI
```

Use `atan(y, x)` not `atan(y/x)` to avoid discontinuity at x=0.

### 3. Segment and Mirror
```glsl
float segmentAngle = TWO_PI / float(segments);
angle = mod(angle, segmentAngle);              // Wrap to first segment
angle = min(angle, segmentAngle - angle);      // Mirror at segment center
```

| Segments | segmentAngle | Visual Result |
|----------|--------------|---------------|
| 4 | 1.571 rad (90°) | Quad symmetry |
| 6 | 1.047 rad (60°) | Hexagonal |
| 8 | 0.785 rad (45°) | Octagonal |
| 12 | 0.524 rad (30°) | Dodecagonal |

### 4. Convert Back to Cartesian
```glsl
vec2 kaleidoUV = vec2(cos(angle), sin(angle)) * radius + 0.5;
kaleidoUV = clamp(kaleidoUV, 0.0, 1.0);  // Prevent edge wrapping
vec4 color = texture(texture0, kaleidoUV);
```

### Optional: Rotation Offset
Add static or animated rotation before segment division:
```glsl
angle += rotation;  // Spins the kaleidoscope pattern
```

## Integration

Insert kaleidoscope as final post-process step in `PostEffectToScreen()`, before chromatic aberration:

```
Current Pipeline:              New Pipeline:
accumTexture                   accumTexture
    ↓                              ↓
[chromatic.fs] → screen        [kaleidoscope.fs] → tempTexture
                                   ↓
                               [chromatic.fs] → screen
```

Kaleidoscope applies when `segments > 1`. Segments = 1 bypasses the effect (no modulo/mirror).

## File Changes

| File | Change |
|------|--------|
| `shaders/kaleidoscope.fs` | Create - fragment shader with polar mirroring |
| `src/render/post_effect.h` | Add `kaleidoShader`, uniform location fields |
| `src/render/post_effect.cpp` | Load shader, add pass before chromatic |
| `src/config/effect_config.h` | Add `kaleidoSegments`, `kaleidoRotation` |
| `src/config/preset.cpp` | Add fields to serialization macro |
| `src/ui/ui_panel_effects.cpp` | Add segment count slider, rotation slider |

## Validation

- [ ] Segments = 1 produces identical output to disabled state
- [ ] Segments = 6 creates hexagonal symmetry with 6 mirrored wedges
- [ ] Changing segment count updates pattern in real-time
- [ ] Rotation slider spins pattern smoothly
- [ ] Preset save/load preserves kaleidoscope settings
- [ ] Combined with feedback zoom/rotation creates spiraling mandalas
- [ ] No visible seams at segment boundaries
- [ ] Performance: <0.5ms additional frame time at 1080p

## References

- [Daniel Ilett - Crazy Kaleidoscopes](https://danielilett.com/2020-02-19-tut3-8-crazy-kaleidoscopes/) - Polar coordinate math explanation
- [LYGIA kaleidoscope](https://lygia.xyz/space/kaleidoscope) - Production GLSL implementation
- [KinoMirror](https://github.com/keijiro/KinoMirror) - Unity reference implementation
- [Synesthesia](https://synesthesia.live/) - Commercial music visualizer with kaleidoscope
