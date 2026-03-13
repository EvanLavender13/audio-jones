# Waveform Draw Styles

Add alternative rendering styles to the waveform drawable. Currently waveforms render as a continuous thick line strip. New styles render the same audio data as discrete dots or rectangular bars, with a configurable point count that averages samples into groups.

## Classification

- **Category**: Drawable (waveform rendering mode)
- **Pipeline Position**: Accumulation buffer draw pass (same as existing waveform)

## Algorithm

Add a `WaveformStyle` enum (`WAVEFORM_STYLE_LINE`, `WAVEFORM_STYLE_DOTS`, `WAVEFORM_STYLE_BARS`) and a `pointCount` field to `WaveformData`. The existing line renderer is unchanged.

### Sample Averaging

Both dots and bars use `pointCount` to subdivide the sample buffer into groups. Each group averages its samples to produce one value. This is computed before drawing:

```
groupSize = count / pointCount
for each group i:
    avg = mean of samples[i*groupSize .. (i+1)*groupSize - 1]
```

### Dots Mode

For each averaged sample, compute the position using the same math as the line renderer (linear: along rotated axis with perpendicular amplitude offset; circular: radial at baseRadius + amplitude). Draw a filled circle at that position with diameter = `thickness` and color from `ColorFromConfig` at the same `t` value the line renderer uses.

Uses `DrawCircleV(pos, thickness * 0.5f, color)`.

### Bars Mode

For each averaged sample, compute the position of the **baseline point** (where amplitude = 0) and the **tip point** (where amplitude = sample value). The bar is a rectangle connecting these two points:

- **Linear path**: baseline is on the rotated axis at the sample's horizontal position. The bar extends perpendicular to the axis by `sample * amplitude` pixels. Width = `thickness`, height = distance from baseline to tip.
- **Circular path**: baseline is at `baseRadius` along the radial direction. The bar extends radially outward (positive samples) or inward (negative samples) by `sample * amplitude * 0.5f`. Width = `thickness`, height = radial extent.

Both cases use `DrawRectanglePro` with rotation matching the local direction (axis angle for linear, radial angle for circular). Color from `ColorFromConfig` at the positional `t` value.

True to signal polarity: positive samples produce bars on one side, negative on the other.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| style | WaveformStyle (enum) | LINE/DOTS/BARS | LINE | Rendering mode |
| pointCount | int | 16-512 | 128 | Number of discrete elements for dots/bars (averaged from samples) |

Existing `thickness` parameter controls dot diameter and bar width. All other existing `WaveformData` fields (amplitudeScale, smoothness, radius, colorShift, etc.) apply to all three styles.

## Modulation Candidates

- **pointCount**: Low counts give chunky retro look, high counts approach smooth curve. Modulating creates density shifts with the music.
- **thickness**: Combined with pointCount, controls the fill ratio. Thick + sparse = blocky, thin + dense = detailed.

### Interaction Patterns

**Competing forces (pointCount vs thickness)**: At high pointCount with low thickness, bars/dots are sparse and delicate. At low pointCount with high thickness, they're chunky and overlap. When both are modulated by different audio bands, the visual density shifts dynamically -- bass could drive thickness while treble drives count, creating tension between coarse and fine detail.

## Notes

- `pointCount` is only used by dots and bars modes; line mode ignores it and renders all samples as before.
- For circular bars, the bar's angular width is `thickness` pixels at the baseline radius. At very high pointCount the bars may overlap angularly -- this is fine and creates a dense look.
- The `smoothness` parameter still applies before averaging, so the waveform shape is smoothed before being quantized into bars/dots.
