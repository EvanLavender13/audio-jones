# Feedback Motion Scale

A global time-dilation control for the feedback pipeline. Reduces per-frame displacement of all feedback transforms and waveform shape updates, letting intricate patterns unfold in slow motion while preserving trail length through auto-coupled decay compensation.

## Classification

- **Type**: General (cross-cutting behavior modification)
- **Affects**: Feedback stage (flow field transforms) + Drawable stage (waveform shape)

## References

- [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html) - Per-frame motion variables (zoom, rot, dx, dy, warp) and `*_att` damping concept
- [MilkDrop Feedback Architecture](docs/research/milkdrop-feedback-architecture.md) - Existing pipeline documentation

## Algorithm

### Feedback Transform Scaling

Each per-frame motion parameter creates movement by accumulating small displacements. A motion scale `s ∈ [0, 1]` reduces displacement proportionally.

**Speed/displacement values** — direct multiplication:

```cpp
float rotBase = rotationSpeed * deltaTime * motionScale;
float rotRadial = rotationSpeedRadial * deltaTime * motionScale;
warpTime += deltaTime * warpSpeed * motionScale;
float dxEff = dxBase * motionScale;
float dyEff = dyBase * motionScale;
```

**Identity-centered values** (zoom, stretch) — scale the deviation from 1.0:

```cpp
float zoomEff = 1.0f + (zoomBase - 1.0f) * motionScale;
float sxEff = 1.0f + (sx - 1.0f) * motionScale;
float syEff = 1.0f + (sy - 1.0f) * motionScale;
```

At `motionScale = 0`, all motion stops (zoom=1, rotation=0, translation=0). At `motionScale = 1`, behavior is unchanged.

**Radial and angular components** follow the same pattern as their base values (direct multiplication for radial speed offsets, deviation scaling for radial zoom offsets).

### Decay Compensation

Trail *distance* = motion speed x half-life. Halving motion without adjusting decay produces trails half as long — content fades before traveling far.

To preserve constant trail length regardless of motion scale, adjust per-frame decay:

```cpp
float effectiveDecay = powf(decay, motionScale);
```

**Why this works:** If per-frame decay is `d`, after `n` frames brightness = `d^n`. Slowing motion by factor `s` means content takes `n/s` frames to reach the same distance. Brightness there becomes `d^(n/s)`. Setting effective decay to `d^s` yields `(d^s)^(n/s) = d^n` — identical brightness at the same visual distance.

At `motionScale = 1`: `effectiveDecay = decay` (unchanged).
At `motionScale = 0.25`: decay per frame reduces (trails last 4x longer in frames, matching 4x slower motion).
At `motionScale = 0`: `effectiveDecay = 1.0` (no fade, since nothing moves).

### Waveform Temporal Smoothing

Replace instant-snap waveform updates with exponential moving average (EMA):

```cpp
for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
    smoothedWaveform[i] = lerp(smoothedWaveform[i], rawWaveform[i], waveformMotionScale);
}
```

- `waveformMotionScale = 1.0`: instant response (current behavior)
- `waveformMotionScale = 0.1`: waveform shape morphs slowly toward new audio data
- `waveformMotionScale = 0.0`: waveform freezes

This replaces the existing `drawInterval` frame-skipping with genuinely smooth interpolation. The two controls serve different purposes: `drawInterval` creates strobed/ghosted echoes, while temporal smoothing creates fluid slow-motion morphing.

### Luminance Flow

`feedbackFlowStrength` drives per-frame edge-smearing displacement. Scale directly:

```cpp
float flowEff = feedbackFlowStrength * motionScale;
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| motionScale | float | 0.0–1.0 | 1.0 | Global speed of all feedback transforms |
| waveformMotionScale | float | 0.0–1.0 | 1.0 | Waveform shape interpolation rate |

## Modulation Candidates

- **motionScale**: audio reactivity creates speed surges on beats, slow drift between — "breathing" motion
- **waveformMotionScale**: tie to RMS energy so quiet passages morph slowly, loud passages snap quickly

## Notes

- Both parameters default to 1.0 (identity), so existing presets are unaffected.
- The decay coupling (`decay^motionScale`) preserves visual density automatically. No manual half-life adjustment needed when tweaking motion scale.
- Waveform smoothing operates on raw samples before the existing spatial smoothing pass (`ProcessWaveformSmooth`), so both controls stack independently.
- At extreme low values (motionScale < 0.05), visuals effectively freeze. Consider a floor or exponential curve for the UI slider to give finer control in the 0.1–0.5 range.
- The existing `drawInterval` mechanism remains useful for intentional strobe/echo aesthetics and is unrelated to this feature.
