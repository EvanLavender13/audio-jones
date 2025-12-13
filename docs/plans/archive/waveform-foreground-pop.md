# Waveform Foreground Visibility

Make waveforms stand out more against feedback trails, which currently blend together.

## Goal

Fresh waveforms should read as foreground elements while trails recede into the background. The feedback effect creates visual depth, but without contrast the layers flatten.

## Current State

- `shaders/feedback.fs:15-16` applies zoom (0.99) and rotation (0.001 rad) with no brightness reduction
- `shaders/blur_v.fs:38-48` handles exponential decay via `halfLife` uniform
- Waveforms draw with configurable thickness via `WaveformConfig.lineThickness`
- Trail and waveform colors share the same palette

## Options

### Option A: Feedback Brightness Decay

Add brightness multiplier in feedback shader. Compounds each frame, making older content fade faster.

```glsl
// feedback.fs - add after finalColor assignment
const float brightness = 0.97;  // 3% darkening per frame
finalColor.rgb *= brightness;
```

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Brightness | 0.97 | 3% per frame reaches 50% after 23 frames (~0.4s at 60fps) |

**Pros:** Single line change, immediate effect, works with any color scheme
**Cons:** May need tuning alongside existing halfLife decay

### Option B: Feedback Hue Shift

Rotate trail colors toward blue/purple so fresh waveforms contrast by hue.

```glsl
// feedback.fs - shift hue toward blue
vec3 hsv = rgb2hsv(finalColor.rgb);
hsv.x = mod(hsv.x + 0.02, 1.0);  // Shift 7 degrees per frame
finalColor.rgb = hsv2rgb(hsv);
```

**Pros:** Creates color depth even with white waveforms
**Cons:** Requires HSV conversion functions, may clash with rainbow mode

### Option C: Feedback Desaturation

Reduce saturation in trails while waveforms stay vivid.

```glsl
// feedback.fs - desaturate trails
float gray = dot(finalColor.rgb, vec3(0.299, 0.587, 0.114));
finalColor.rgb = mix(finalColor.rgb, vec3(gray), 0.05);  // 5% toward gray
```

**Pros:** Works with any color palette, subtle depth effect
**Cons:** May look washed out with already-muted colors

### Option D: Waveform Line Thickness

Increase `WaveformConfig.lineThickness` default or range.

| Current | Proposed | Rationale |
|---------|----------|-----------|
| Default 2.0 | Default 3.0 | Thicker lines more visible against busy background |
| Max 10.0 | Max 15.0 | Allow user to push further if needed |

**Pros:** No shader changes, user-controllable
**Cons:** Doesn't address color/brightness blending

### Option E: Waveform Glow Pass

Draw waveforms twice: first with blur for glow, then sharp on top.

**Pros:** Creates halo effect that separates from background
**Cons:** Doubles waveform draw calls, requires render pipeline changes

## Recommendations

Start with Option A (brightness decay) as it directly addresses the blending issue with minimal code. If insufficient, layer with Option C (desaturation) or Option D (line thickness).

Option B (hue shift) offers the most dramatic separation but risks color palette conflicts.

## File Changes

| File | Change |
|------|--------|
| `shaders/feedback.fs` | Add brightness multiplier (Option A) |
| `config/waveform_config.h` | Increase lineThickness default/max (Option D) |

## Validation

- [ ] Fresh waveforms visually distinct from 1-second-old trails
- [ ] Effect works with both solid and rainbow color modes
- [ ] No visible banding or artifacts from accumulated multiplications
- [ ] User can still see trail motion (not too aggressive)
