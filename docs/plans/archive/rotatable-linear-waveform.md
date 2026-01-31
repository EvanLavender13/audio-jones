# Rotatable Linear Waveform

Add geometric rotation to linear waveforms and independent color cycling to both linear and circular waveforms. Brings parity between path types: Spin/Angle controls geometry, Color Shift/Color Spin controls color cycling.

## Specification

### Types

Add to `WaveformData` in `src/config/drawable_config.h`:

```cpp
struct WaveformData {
  float amplitudeScale = 0.35f;
  float thickness = 2.0f;
  float smoothness = 5.0f;
  float radius = 0.25f;
  float waveformMotionScale = 1.0f;
  float colorShift = 0.0f;      // Static color offset (radians)
  float colorShiftSpeed = 0.0f; // Color cycling rate (radians/sec)
};
```

Add runtime accumulator to `Drawable` in `src/config/drawable_config.h`:

```cpp
struct Drawable {
  // ... existing fields ...
  float rotationAccum = 0.0f;      // Existing
  float colorShiftAccum = 0.0f;    // New: color shift accumulator
  // ...
};
```

### Algorithm

**Linear waveform rotation** (`DrawWaveformLinear` in `src/render/waveform.cpp`):

```cpp
void DrawWaveformLinear(const float *samples, int count, RenderContext *ctx,
                        const Drawable *d, uint64_t globalTick, float opacity) {
  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float amplitude = ctx->minDim * d->waveform.amplitudeScale;

  // Geometric rotation
  const float angle = d->base.rotationAngle + d->rotationAccum;
  const float cosA = cosf(angle);
  const float sinA = sinf(angle);

  // Calculate line length to span viewport at this angle
  // At 0°: screenW, at 90°: screenH, at 45°: diagonal
  const float abscos = fabsf(cosA);
  const float abssin = fabsf(sinA);
  float lineLength;
  if (abscos < 0.001f) {
    lineLength = ctx->screenH;
  } else if (abssin < 0.001f) {
    lineLength = ctx->screenW;
  } else {
    // Length needed to reach viewport edge in both dimensions
    const float lenX = ctx->screenW / abscos;
    const float lenY = ctx->screenH / abssin;
    lineLength = fminf(lenX, lenY);
  }

  const float halfLen = lineLength * 0.5f;
  const float step = lineLength / (count - 1);

  // Color offset from color shift (independent of geometry)
  const float effectiveColorShift = d->waveform.colorShift + d->colorShiftAccum;
  float colorOffset = fmodf(-effectiveColorShift / (2.0f * PI), 1.0f);
  if (colorOffset < 0.0f) {
    colorOffset += 1.0f;
  }

  ThickLineBegin(d->waveform.thickness);
  for (int i = 0; i < count; i++) {
    // Position along rotated axis
    const float dist = -halfLen + i * step;

    // Perpendicular offset for amplitude
    const float amp = samples[i] * amplitude;

    // Rotated position: dist along axis, amp perpendicular
    const float localX = dist * cosA - amp * sinA;
    const float localY = dist * sinA + amp * cosA;

    const Vector2 pos = {centerX + localX, centerY + localY};

    // Color from position + offset
    float t = (float)i / (count - 1) + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color vertColor = ColorFromConfig(&d->base.color, t, opacity);
    ThickLineVertex(pos, vertColor);
  }
  ThickLineEnd(false);
}
```

**Circular waveform color shift** (`DrawWaveformCircular` in `src/render/waveform.cpp`):

```cpp
// Add after effectiveRotation calculation:
const float effectiveColorShift = d->waveform.colorShift + d->colorShiftAccum;
float colorOffset = fmodf(-effectiveColorShift / (2.0f * PI), 1.0f);
if (colorOffset < 0.0f) {
  colorOffset += 1.0f;
}

// Change color calculation in loop:
float t = (float)i / count + colorOffset;
if (t >= 1.0f) {
  t -= 1.0f;
}
const Color vertColor = ColorFromConfig(&d->base.color, t, opacity);
```

**Accumulator tick** (`DrawableTickRotations` in `src/render/drawable.cpp`):

```cpp
void DrawableTickRotations(Drawable *drawables, int count, float deltaTime) {
  for (int i = 0; i < count; i++) {
    drawables[i].rotationAccum += drawables[i].base.rotationSpeed * deltaTime;
    if (drawables[i].type == DRAWABLE_WAVEFORM) {
      drawables[i].colorShiftAccum +=
          drawables[i].waveform.colorShiftSpeed * deltaTime;
    }
    if (drawables[i].type == DRAWABLE_PARAMETRIC_TRAIL) {
      drawables[i].parametricTrail.phase +=
          drawables[i].parametricTrail.speed * deltaTime;
    }
  }
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| colorShift | float | 0 - 2π | 0.0 | Yes | Color Angle |
| colorShiftSpeed | float | -2π - 2π | 0.0 | Yes | Color Spin |

### Constants

No new enums or transform types required. Uses existing drawable infrastructure.

---

## Tasks

### Wave 1: Config Changes

#### Task 1.1: Add WaveformData fields and accumulator

**Files**: `src/config/drawable_config.h`

**Build**:
1. Add `float colorShift = 0.0f;` to `WaveformData` struct after `waveformMotionScale`
2. Add `float colorShiftSpeed = 0.0f;` to `WaveformData` struct after `colorShift`
3. Add `float colorShiftAccum = 0.0f;` to `Drawable` struct after `rotationAccum`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Core Implementation

#### Task 2.1: Update DrawWaveformLinear

**Files**: `src/render/waveform.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Replace `DrawWaveformLinear` function body with the algorithm from the spec
2. Include `<math.h>` for `fminf`, `fabsf` (already included)

**Verify**: Compiles. Linear waveform rotates with Spin/Angle slider.

#### Task 2.2: Update DrawWaveformCircular

**Files**: `src/render/waveform.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. After `effectiveRotation` calculation, add color shift calculation from spec
2. Update the color `t` calculation in the loop to add `colorOffset`
3. Add wrap logic for `t >= 1.0f`

**Verify**: Compiles. Circular waveform color cycles with new sliders.

#### Task 2.3: Update DrawableTickRotations

**Files**: `src/render/drawable.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add colorShiftAccum tick for DRAWABLE_WAVEFORM type per spec algorithm

**Verify**: Compiles.

---

### Wave 3: Integration

#### Task 3.1: Add UI controls

**Files**: `src/ui/drawable_type_controls.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. In `DrawWaveformControls`, inside the Animation section (after `DrawBaseAnimationControls`), add:
```cpp
ModulatableDrawableSliderSpeedDeg("Color Spin", &d->waveform.colorShiftSpeed,
                                  d->id, "colorShiftSpeed", sources);
ModulatableDrawableSliderAngleDeg("Color Angle", &d->waveform.colorShift,
                                  d->id, "colorShift", sources);
```

**Verify**: Compiles. UI shows Color Spin and Color Angle sliders.

#### Task 3.2: Add param registry entries

**Files**: `src/automation/drawable_params.cpp`, `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `param_registry.cpp`, add to `DRAWABLE_FIELD_TABLE`:
```cpp
{"colorShift", 0.0f, 2.0f * PI, 0.0f},
{"colorShiftSpeed", -2.0f * PI, 2.0f * PI, 0.0f},
```
2. In `drawable_params.cpp`, in the waveform section of `DrawableParamsRegister`, add pointer registrations for `colorShift` and `colorShiftSpeed`

**Verify**: Compiles. Parameters appear in modulation dropdown.

#### Task 3.3: Add preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Find waveform serialization section (search for `waveformMotionScale`)
2. Add `colorShift` and `colorShiftSpeed` to JSON write
3. Add `colorShift` and `colorShiftSpeed` to JSON read with defaults

**Verify**: Compiles. Save/load preset preserves color shift values.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Linear waveform rotates geometrically with Spin/Angle
- [ ] Linear waveform centered on X/Y position
- [ ] Linear waveform scales to fill viewport at all angles
- [ ] Color Spin/Angle cycles colors on linear waveform
- [ ] Color Spin/Angle cycles colors on circular waveform (independent of geometry rotation)
- [ ] Modulation works for new parameters
- [ ] Presets save/load new parameters
