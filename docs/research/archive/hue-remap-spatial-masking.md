# Hue Remap Spatial Masking

Expand Hue Remap's spatial control from a single radial blend to five independent masking modes — radial, angular, linear gradient, luminance, and noise — applied independently to both blend intensity and hue shift. Each pixel's final blend and shift are computed from the sum of active spatial contributions, giving full control over where and how the color remapping applies across the frame.

## Classification

- **Category**: TRANSFORMS > Color (existing effect enhancement)
- **Pipeline Position**: Same as current Hue Remap — color transform stage
- **Scope**: Shader + config + UI expansion only. No new files, no new pipeline stages.

## References

- `shaders/feedback.fs` lines 60-80 — radial + angular spatial field computation (aspect-corrected polar coords, `base + rad * radial + sin(ang * freq) * angular` pattern)
- `shaders/hue_remap.fs` — current implementation with single radial mask
- `src/config/effect_config.h` FlowFieldConfig — parameter structure for per-target radial/angular coefficients with int frequency

## Algorithm

### Spatial Coordinate Setup (shared, computed once)

Same polar coordinate system as feedback.fs:
- Aspect-corrected UV relative to `(cx, cy)` center
- `rad` = distance from center (0 at center, ~1 at shorter edge)
- `ang` = polar angle via `atan(y, x)`
- `luma` = luminance of source pixel (for luminance mode)
- `noise` = 2D simplex/value noise at `fragTexCoord * noiseScale + time * noiseSpeed`

### Per-Target Spatial Field

Both blend and shift compute their own spatial field from 5 modes. Each mode's coefficient is signed: positive = normal, negative = inverted.

**Radial**: Varies with distance from center. Positive = stronger at edges, negative = stronger at center. Matches current behavior.

**Angular**: Sine wave around polar angle. `sin(ang * freq) * amplitude` produces petal/spoke patterns. Frequency controls how many lobes (1-8).

**Linear**: Directional gradient. Projects UV onto a direction vector controlled by an angle parameter. Creates a wipe/fade across one axis.

**Luminance**: Content-aware. Positive = more effect on bright areas, negative = more on dark areas. Reads from source texture.

**Noise**: Procedural organic variation. Shared noise field (scale + speed params) with per-target amplitude. Creates drifting patchy application.

### Field Composition

When all spatial coefficients are zero, the field equals 1.0 (uniform application — current default behavior). Active modes introduce spatial variation. The composition must keep blend in 0-1 range (clamp) and shift in 0-1 range (fract/wrap).

The current radial mask approach (`intensity * mix(1.0, field, abs(radial))`) should be generalized to handle multiple additive spatial contributions while preserving the "zero coefficients = uniform" invariant.

### Preset Compatibility

Current `radial` field maps directly to the new `blendRadial`. No other existing fields change meaning. Old presets with `radial` set will behave identically.

## Parameters

### Existing (unchanged)

| Parameter | Type | Range | Default | Purpose |
|-----------|------|-------|---------|--------|
| `enabled` | bool | — | false | Master enable |
| `gradient` | ColorConfig | — | rainbow | Color palette |
| `shift` | float | 0.0-1.0 | 0.0 | Base hue rotation through palette |
| `intensity` | float | 0.0-1.0 | 1.0 | Base blend strength |
| `cx` | float | 0.0-1.0 | 0.5 | Center X (shared by all spatial modes) |
| `cy` | float | 0.0-1.0 | 0.5 | Center Y (shared by all spatial modes) |

### Existing (renamed)

| Old | New | Notes |
|-----|-----|-------|
| `radial` | `blendRadial` | Same behavior, just namespaced under blend |

### New: Blend Spatial Coefficients

| Parameter | Type | Range | Default | Purpose |
|-----------|------|-------|---------|--------|
| `blendAngular` | float | -1.0 to 1.0 | 0.0 | Angular sine wave amplitude |
| `blendAngularFreq` | int | 1-8 | 2 | Angular lobe count |
| `blendLinear` | float | -1.0 to 1.0 | 0.0 | Linear gradient strength |
| `blendLinearAngle` | float | 0-2π | 0.0 | Linear gradient direction (radians, displayed as degrees) |
| `blendLuminance` | float | -1.0 to 1.0 | 0.0 | Luminance-based masking |
| `blendNoise` | float | -1.0 to 1.0 | 0.0 | Noise field amplitude |

### New: Shift Spatial Coefficients

| Parameter | Type | Range | Default | Purpose |
|-----------|------|-------|---------|--------|
| `shiftRadial` | float | -1.0 to 1.0 | 0.0 | Shift varies with distance |
| `shiftAngular` | float | -1.0 to 1.0 | 0.0 | Shift varies with angle |
| `shiftAngularFreq` | int | 1-8 | 2 | Shift angular lobe count |
| `shiftLinear` | float | -1.0 to 1.0 | 0.0 | Shift linear gradient strength |
| `shiftLinearAngle` | float | 0-2π | 0.0 | Shift linear gradient direction |
| `shiftLuminance` | float | -1.0 to 1.0 | 0.0 | Shift varies with brightness |
| `shiftNoise` | float | -1.0 to 1.0 | 0.0 | Shift noise amplitude |

### New: Shared Noise Field

| Parameter | Type | Range | Default | Purpose |
|-----------|------|-------|---------|--------|
| `noiseScale` | float | 1.0-20.0 | 5.0 | Noise spatial frequency |
| `noiseSpeed` | float | 0.0-2.0 | 0.5 | Noise drift speed |

## Modulation Candidates

All float parameters are modulatable. Integer frequency params (`blendAngularFreq`, `shiftAngularFreq`) are non-modulatable (SliderInt, matching feedback convention).

- **blendRadial / blendAngular**: Modulating these with different audio bands creates verse-vs-chorus spatial variation — quiet passages apply uniformly, loud sections concentrate the effect
- **shiftAngular**: Modulating shift's angular coefficient rotates the hue distribution around the center — creates spinning color wheels
- **shiftLuminance**: Audio-reactive luminance gating — bright areas shift hue on beats while dark areas stay stable
- **noiseSpeed**: Modulating drift speed ties the organic variation to musical energy

### Interaction Patterns

**Competing forces (blend)**: `blendRadial` and `blendLuminance` can oppose each other — radial pulls effect to edges while luminance pulls to bright areas. The visual result depends on where bright content sits relative to center.

**Cascading threshold (shift)**: `shiftAngular` only produces visible change where `blend` is non-zero. Angular shift variation is invisible in regions where blend spatial modes have suppressed the effect — blend gates shift visibility.

## Notes

- Simplex noise preferred over value noise for organic look (no grid artifacts). A simple 2D hash-based noise is sufficient — doesn't need to be true simplex, just smooth.
- The noise function can be inlined in the shader (~15 lines). No external texture needed.
- Angular frequency as int (1-8) matches feedback convention and avoids non-integer lobe counts that produce discontinuities.
- Linear gradient angle uses `SliderAngleDeg` / `ModulatableSliderAngleDeg` per angular field conventions.
- Total new uniforms: ~17 floats + 2 ints. Well within uniform budget.
