# Lissajous Parameter Standardization

The shared `DualLissajousConfig` and the shared `DrawLissajousControls` widget are used by ~20 effects, but every callsite passes a different `freqMax` argument and there is no shared convention for which parameters get registered as modulatable. The result is that "the same shit" looks and feels different across effects: in some, the freq slider tops out at 0.2; in others at 10. Some effects modulate amplitude and motion speed; others omit them.

This document inventories the inconsistencies and proposes a single convention.

## Classification

- **Category**: General (cross-cutting infrastructure, no algorithm)
- **Pipeline Position**: N/A

## References

None - this is internal codebase consistency work.

## Reference Code

Existing surface area:

`src/config/dual_lissajous_config.h`:

```cpp
struct DualLissajousConfig {
  float amplitude = 0.2f;   // Motion amplitude (0.0-0.5)
  float motionSpeed = 1.0f; // Phase accumulation rate (0.0-5.0)

  float freqX1 = 0.05f;   // Primary X frequency (Hz)
  float freqY1 = 0.08f;   // Primary Y frequency (Hz)
  float freqX2 = 0.0f;    // Secondary X frequency (Hz, 0 = disabled)
  float freqY2 = 0.0f;    // Secondary Y frequency (Hz, 0 = disabled)
  float offsetX2 = 0.3f;  // Phase offset for secondary X (radians)
  float offsetY2 = 2.09f; // Phase offset for secondary Y (radians, ~120deg)

  float phase = 0.0f; // Internal state, not serialized
};
```

`src/ui/ui_units.h::DrawLissajousControls`:

```cpp
inline void DrawLissajousControls(DualLissajousConfig *cfg,
                                  const char *idSuffix, const char *paramPrefix,
                                  const ModSources *modSources,
                                  float freqMax = 5.0f);
```

The default `freqMax` in the function signature is 5.0, but most callsites override it.

## Inventory

### freqMax across callsites

| `freqMax` | Effects |
|---|---|
| 0.2 | Ripple Tank, Quadtree, Laser Dance |
| 0.5 | Spectral Rings |
| 1.0 | Protean Clouds |
| 2.0 | Light Medley, Voxel March, Cyber March, Isoflow |
| 5.0 (default) | Calligraph, Twist Cage, Escher Droste, Infinite Zoom (x2), Mobius (x2), Wave Ripple, Surface Depth |
| 10.0 | Arc Strobe, Dancing Lines |

There is no documented reason for any specific cap. The values appear to be set per-effect by feel during initial implementation and copy-pasted into siblings without thought (e.g., Quadtree got 0.2 from Ripple Tank).

### Modulation registrations

Each effect chooses independently which Lissajous params to register. Common patterns:

- Most effects register `<prefix>.lissajous.amplitude` (0.0-0.5) and `<prefix>.lissajous.motionSpeed` (0.0-5.0)
- Frequency fields (`freqX1`, `freqY1`, `freqX2`, `freqY2`) are NEVER registered as modulatable. The DualLissajousConfig comment says "Shape params (not modulatable - cause discontinuities)" - meaning real-time phase accumulation breaks if the freq jumps mid-cycle. This is a real constraint, not stylistic.
- Phase offsets (`offsetX2`, `offsetY2`) are NEVER registered. They are static angle adjustments.

The `motionSpeed` registration range is also inconsistent: most effects use 0.0-5.0, Physarum uses 0.0-10.0.

### Slider format strings and ranges

`DrawLissajousControls` itself uses `"%.2f Hz"` for freq sliders, `"%.2f"` for amplitude/motionSpeed/offsets, and a fixed amplitude range 0.0-0.5 and motionSpeed range 0.0-10.0 (note: this is the SLIDER range; the registered modulation range is usually 0.0-5.0 - another inconsistency).

### "Hz" labeling

The freq sliders display "%.2f Hz" but the math treats values as `phase * freqX1` where phase is in `seconds * motionSpeed`. The unit is effectively rad/sec, not Hz. To complete one sin/cos cycle, `phase * freqX1` must reach 2pi, which takes `2pi / (motionSpeed * freqX1)` seconds. With defaults `motionSpeed = 1.0`, `freqX1 = 0.05`, that is 126 seconds, not 20 (which would be 1/0.05 Hz).

The "Hz" suffix is incorrect.

## Algorithm

This is consistency work, not an algorithm. The proposed standardization:

### Standard 1: `freqMax` is fixed at 5.0 for all callers

Drop the per-callsite override. The default value in `DrawLissajousControls` is 5.0; remove the trailing `freqMax` argument from every call. Effects that genuinely need a different cap (none found) can use raw `ImGui::SliderFloat` instead of the shared widget.

5.0 is the most common existing value (8 callsites use it explicitly, plus the function default). At the largest sensible amplitude (0.5) and motionSpeed (5.0), the slowest cycle (freq = 0.01 ish) is a few minutes; the fastest (freq = 5.0) is `2pi / (5 * 5)` = 0.25 sec/cycle. That is the full usable range.

### Standard 2: Drop the "Hz" label

Change `"%.2f Hz"` -> `"%.2f"` in `DrawLissajousControls`. The unit is internal coupling between freq and motionSpeed; presenting it as Hz is a lie. A unitless slider is honest.

### Standard 3: Standard modulation registration helper

Add a helper `DualLissajousRegisterParams(DualLissajousConfig *cfg, const char *prefix)` that registers the standard subset (amplitude 0.0-0.5, motionSpeed 0.0-5.0) and is called by every effect that embeds the config. This eliminates the per-effect copy-paste and prevents the Physarum-style (0.0-10.0 motionSpeed) drift.

Effects that intentionally want a wider motionSpeed range can register an override after calling the helper, but the default is the standard.

### Standard 4: `DualLissajousUpdateMulti` (already done)

The old `DualLissajousUpdateCircular` was a wobble-around-static-circle layout, not a travel along a Lissajous path. The function was renamed to `DualLissajousUpdateMulti` and now uses `cfg->amplitude` for the position scale (matching `DualLissajousUpdate`). The redundant `baseRadius` parameter was removed from the function and from the three callers (Ripple Tank, Quadtree, Physarum).

This standard is documented here so the rename and removal are recorded; the implementation already happened in the same session that produced this doc.

### Standard 5: Default `motionSpeed` and `freqX1`/`freqY1`

The defaults in the config struct (`motionSpeed = 1.0`, `freqX1 = 0.05`, `freqY1 = 0.08`) produce ~125-second cycles. New effects that embed the config get this slow-by-default behavior. Either:

- Bump struct defaults to something that cycles in ~5-10 seconds (e.g., `motionSpeed = 1.0`, `freqX1 = 0.5`, `freqY1 = 0.7`)
- Leave struct defaults alone and require effects to override at the embed site (`DualLissajousConfig lissajous = {.freqX1 = 0.5, .freqY1 = 0.7};`)

Recommendation: bump struct defaults. The current defaults serve no one - every effect that embeds the config silently gets motion that takes 2 minutes to cycle. New defaults should produce visible motion at the slider center.

## Parameters

This document does not define new parameters. It standardizes existing parameter handling.

## Modulation Candidates

Only `amplitude` and `motionSpeed` are runtime-safe modulation targets. All other Lissajous fields (frequencies, phase offsets) cause discontinuities when changed mid-cycle and must not be modulated.

## Notes

- This work is a precondition for cleanly adding Lissajous-driven effects in the future. Without it, every new effect re-derives the freqMax-and-mod-registration choices independently.
- Backwards compatibility is not a concern: presets store actual numeric values, not defaults; format strings only affect UI display; freqMax only affects slider caps. Default-value bumps may surprise existing presets when a field rolls back to default, but that is a no-op for any preset that explicitly sets the field.
- The "Hz" label correction does not change preset serialization - it is a display-only string.
