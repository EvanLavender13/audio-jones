# Waveform Draw Styles

Add dots and bars rendering modes to the waveform drawable. The existing line renderer is unchanged. New modes average samples into `pointCount` groups and draw discrete dots or rectangular bars, reusing the same position math as the line renderer for both linear and circular paths.

**Research**: `docs/research/waveform_draw_styles.md`

## Design

### Types

**Enum** (in `drawable_config.h`):

```c
typedef enum {
  WAVEFORM_STYLE_LINE = 0,
  WAVEFORM_STYLE_DOTS = 1,
  WAVEFORM_STYLE_BARS = 2,
} WaveformStyle;
```

**WaveformData additions** (in `drawable_config.h`):

```c
WaveformStyle style = WAVEFORM_STYLE_LINE;
float pointCount = 128.0f; // Discrete element count for dots/bars (16-512)
```

<!-- Intentional deviation: research specifies pointCount as int, changed to float for modulation compatibility per project conventions (ModulatableSlider requires float) -->
`pointCount` is `float` for modulation compatibility, cast to `int` at draw time. Line mode ignores it.

### Algorithm

**Note**: The `smoothness` parameter still applies before averaging — the draw functions receive already-smoothed sample data from `ProcessWaveformSmooth`, so the waveform shape is smoothed before being quantized into discrete dots/bars.

#### Sample Averaging (shared by dots and bars)

Both modes subdivide the sample buffer into groups before drawing:

```c
int pc = (int)pointCount;
int groupSize = count / pc;
float averaged[pc];
for (int i = 0; i < pc; i++) {
    float sum = 0.0f;
    for (int j = 0; j < groupSize; j++) {
        sum += samples[i * groupSize + j];
    }
    averaged[i] = sum / groupSize;
}
```

#### Dots — Linear

For each averaged sample at index `i`:

```c
float t_pos = (float)i / (pc - 1);
float dist = -halfLen + t_pos * lineLength;
float amp = averaged[i] * amplitude;
float localX = dist * cosA - amp * sinA;
float localY = dist * sinA + amp * cosA;
Vector2 pos = {centerX + localX, centerY + localY};
// Color t
float t = t_pos + colorOffset;
if (t >= 1.0f) t -= 1.0f;
Color color = ColorFromConfig(&d->base.color, t, opacity);
DrawCircleV(pos, thickness * 0.5f, color);
```

Same position math as `DrawWaveformLinear` line renderer, substituting `t_pos` for `(float)i / (count - 1)`.

#### Dots — Circular

For each averaged sample at index `i`:

```c
float t_pos = (float)i / pc;
float angle = t_pos * 2.0f * PI + effectiveRotation - PI / 2;
float radius = baseRadius + averaged[i] * (amplitude * 0.5f);
if (radius < 10.0f) radius = 10.0f;
Vector2 pos = {centerX + cosf(angle) * radius, centerY + sinf(angle) * radius};
float t = t_pos + colorOffset;
if (t >= 1.0f) t -= 1.0f;
Color color = ColorFromConfig(&d->base.color, t, opacity);
DrawCircleV(pos, thickness * 0.5f, color);
```

Same position math as `DrawWaveformCircular` line renderer.

#### Bars — Linear

Uses the same setup variables as the linear line/dots code: `angle = d->base.rotationAngle + d->rotationAccum`, `cosA = cosf(angle)`, `sinA = sinf(angle)`, plus `centerX`, `centerY`, `amplitude`, `halfLen`, `lineLength`, `colorOffset`.

For each averaged sample, draw a rectangle from the baseline (amplitude=0) to the tip:

```c
float t_pos = (float)i / (pc - 1);
float dist = -halfLen + t_pos * lineLength;
// Baseline point (on the rotated axis, amplitude = 0)
float baseX = centerX + dist * cosA;
float baseY = centerY + dist * sinA;
// Bar height = perpendicular extent
float barHeight = averaged[i] * amplitude;
// Bar center is offset from baseline by half the bar height
float halfBar = barHeight * 0.5f;
float barCenterX = baseX - halfBar * sinA;
float barCenterY = baseY + halfBar * cosA;
// Rectangle: width = thickness, height = |barHeight|
float angleDegs = angle * RAD2DEG;
Rectangle rect = {barCenterX, barCenterY, thickness, fabsf(barHeight)};
Vector2 origin = {thickness * 0.5f, fabsf(barHeight) * 0.5f};
float t = t_pos + colorOffset;
if (t >= 1.0f) t -= 1.0f;
Color color = ColorFromConfig(&d->base.color, t, opacity);
DrawRectanglePro(rect, origin, angleDegs, color);
```

The bar extends perpendicular to the rotated axis, true to signal polarity (positive samples on one side, negative on the other).

#### Bars — Circular

For each averaged sample, draw a radial bar from `baseRadius` outward/inward:

```c
float t_pos = (float)i / pc;
float angle = t_pos * 2.0f * PI + effectiveRotation - PI / 2;
float barHeight = averaged[i] * amplitude * 0.5f;
float barRadius = baseRadius + barHeight * 0.5f;
float barCenterX = centerX + cosf(angle) * barRadius;
float barCenterY = centerY + sinf(angle) * barRadius;
// Rectangle rotated to point radially
float angleDegs = angle * RAD2DEG + 90.0f;
Rectangle rect = {barCenterX, barCenterY, thickness, fabsf(barHeight)};
Vector2 origin = {thickness * 0.5f, fabsf(barHeight) * 0.5f};
float t = t_pos + colorOffset;
if (t >= 1.0f) t -= 1.0f;
Color color = ColorFromConfig(&d->base.color, t, opacity);
DrawRectanglePro(rect, origin, angleDegs, color);
```

The bar extends radially: positive samples outward, negative inward from `baseRadius`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| style | WaveformStyle | LINE/DOTS/BARS | LINE | No | "Style" |
| pointCount | float | 16-512 | 128 | Yes | "Points" |

Existing `thickness` controls dot diameter and bar width. All other existing `WaveformData` fields apply to all three styles.

---

## Tasks

### Wave 1: Config

#### Task 1.1: Add WaveformStyle enum and fields

**Files**: `src/config/drawable_config.h`
**Creates**: `WaveformStyle` enum, `style` and `pointCount` fields on `WaveformData`

**Do**:
- Add `WaveformStyle` typedef enum before `WaveformData` struct (same pattern as `TrailShapeType` / `TrailMotionType`)
- Add `WaveformStyle style = WAVEFORM_STYLE_LINE;` and `float pointCount = 128.0f; // Discrete element count (16-512)` to `WaveformData`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Drawing logic

**Files**: `src/render/waveform.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add a `static void AverageSamples(const float *samples, int count, float *averaged, int pointCount)` helper that computes group averages as described in the Algorithm section.
- Add `static void DrawDotsLinear(...)` and `static void DrawDotsCircular(...)` functions implementing the Dots algorithm. Same parameter signature as existing `DrawWaveformLinear`/`DrawWaveformCircular`.
- Add `static void DrawBarsLinear(...)` and `static void DrawBarsCircular(...)` functions implementing the Bars algorithm.
- In `DrawWaveformLinear`: if `d->waveform.style != WAVEFORM_STYLE_LINE`, call the averaged-sample static helper then dispatch to `DrawDotsLinear` or `DrawBarsLinear` and return early. Existing line code is untouched.
- In `DrawWaveformCircular`: same dispatch pattern to `DrawDotsCircular` or `DrawBarsCircular`.
- Include `<stdlib.h>` for `fabsf` if not already included (already has `<math.h>` which provides it).

**Verify**: Compiles. Line mode unchanged.

---

#### Task 2.2: Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add `style, pointCount` to the `WaveformData` serialization macro call (line ~19-22). Append after `colorShiftSpeed`.

**Verify**: Compiles. Existing presets load without error (new fields get defaults).

---

#### Task 2.3: Param registration

**Files**: `src/automation/drawable_params.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In the `DRAWABLE_WAVEFORM` section of `DrawableParamsRegister`, add registration for `pointCount` with range 16.0f-512.0f. Follow the same `snprintf` + `ModEngineRegisterParam` pattern as adjacent params.

**Verify**: Compiles.

---

#### Task 2.4: UI controls

**Files**: `src/ui/drawable_type_controls.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `DrawWaveformControls`, add a Style combo and conditional Points slider inside the Geometry section, after the existing Thickness slider and before the Smooth slider.
- Style combo: `const char *styleLabels[] = {"Line", "Dots", "Bars"};` with `ImGui::Combo("Style", ...)` casting to/from `int`. Same pattern as `TrailMotionType` combo in `DrawParametricTrailControls`.
- Points slider: `ModulatableDrawableSlider("Points", &d->waveform.pointCount, d->id, "pointCount", "%.0f", sources)` — only shown when style is not LINE (wrap in `if (d->waveform.style != WAVEFORM_STYLE_LINE)`).

**Verify**: Compiles. Style combo visible. Points slider hidden for Line mode, visible for Dots/Bars.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Line mode renders identically to before (no regression)
- [ ] Dots mode draws filled circles at averaged sample positions
- [ ] Bars mode draws rectangles from baseline to tip, true to polarity
- [ ] Both modes work with linear and circular paths
- [ ] pointCount slider hidden in Line mode, visible in Dots/Bars
- [ ] Preset save/load round-trips style and pointCount
- [ ] pointCount is modulatable via LFO routing
