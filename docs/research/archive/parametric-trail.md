# Parametric Trail

A cursor traces mathematical curves, depositing strokes that accumulate via feedback. Time-gating toggles between smooth continuous lines and scattered chaotic fragments. Path formulas are interchangeable: Lissajous figures, circles, or spirals.

## Classification

- **Type**: Drawable
- **Pipeline Position**: Draws to accumulation buffer, feeds into feedback stage

## References

- [Lissajous Curve - Wolfram MathWorld](https://mathworld.wolfram.com/LissajousCurve.html) - Parametric equations and parameter effects
- [Lissajous Figures - IntMath](https://www.intmath.com/trigonometric-graphs/7-lissajous-figures.php) - Frequency ratio effects on figure shape
- [Drawing Lines is Hard - Matt DesLauriers](https://mattdesl.svbtle.com/drawing-lines-is-hard) - GPU line rendering techniques
- [Smooth Mouse Drawing - fad (Shadertoy)](https://www.shadertoy.com/view/dldXR7) - Time-gated drawing concept (user-provided)

## Algorithm

### Path Types

**Lissajous** (default):
```
x(t) = centerX + amplitude * (sin(freqX1 * phase) + sin(freqX2 * phase + offsetX))
y(t) = centerY + amplitude * (sin(freqY1 * phase) + sin(freqY2 * phase + offsetY))
```

Configuration produces different behaviors:
- `freqX2 = freqY2 = 0`: Classic single-harmonic Lissajous
- `freqX1 = freqY1, offset = π/2`: Circle
- Rational frequency ratios: Closed repeating figures (figure-8, pretzel)
- Irrational ratios: Space-filling paths that never repeat, appear chaotic
- Non-zero secondary frequencies: Complex multi-harmonic paths (reference shader style)

**Spiral** (expanding/contracting):
```
r(t) = baseRadius + spiralGrowth * phase
x(t) = centerX + r(t) * cos(phase * spiralTurns)
y(t) = centerY + r(t) * sin(phase * spiralTurns)
```

### Phase Accumulation

CPU accumulates phase each frame:
```cpp
phase += deltaTime * speed;
```

### Time-Gating (Draw Gate)

Controls whether the cursor draws or skips each frame:
```cpp
bool shouldDraw = true;
if (drawGateEnabled) {
    float gatePhase = fmodf(time * gateFreq, 1.0f);
    shouldDraw = gatePhase < gateDuty;  // duty cycle 0-1
}
```

- `gateFreq = 0` or `gateDuty = 1.0`: continuous drawing
- `gateFreq > 0`, `gateDuty < 1.0`: intermittent drawing, creates scattered strokes
- High `gateFreq` + low `gateDuty`: rapid dashes
- Low `gateFreq` + low `gateDuty`: long gaps between stroke bursts

### Stroke Rendering

Each frame (when draw gate allows):
1. Compute current cursor position from path equation
2. Draw line segment from previous position to current using `ThickLineBegin/Vertex/End`
3. Optionally draw circles at both endpoints for rounded caps
4. Store current position as previous for next frame

```cpp
if (shouldDraw && hasValidPrevPos) {
    // Line segment
    ThickLineBegin(thickness);
    ThickLineVertex(prevPos, color);
    ThickLineVertex(currPos, color);
    ThickLineEnd(false);

    // Rounded caps (optional)
    if (roundedCaps) {
        DrawCircleV(prevPos, thickness * 0.5f, color);
        DrawCircleV(currPos, thickness * 0.5f, color);
    }
}
prevPos = currPos;
```

When draw gate disables drawing, `prevPos` still updates - this creates gaps in the trail.

### Color

HSV cycling over time (like reference shader):
```cpp
float hue = fmodf(time * hueSpeed, 1.0f);
Color color = ColorFromHSV(hue * 360.0f, saturation, value);
```

Or use the existing `ColorConfig` system for gradient/solid/audio-reactive modes.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| `pathType` | enum | Lissajous/Spiral | Lissajous | Curve shape |
| `speed` | float | 0.1-10.0 | 1.0 | How fast cursor moves along path |
| `amplitude` | float | 0.05-0.5 | 0.25 | Path size (fraction of screen) |
| `thickness` | float | 1.0-50.0 | 4.0 | Stroke width in pixels |
| `roundedCaps` | bool | - | true | Circle caps vs flat rectangle ends |
| `gateEnabled` | bool | - | false | Enable time-gating |
| `gateFreq` | float | 0.1-20.0 | 5.0 | Gate oscillation frequency (Hz) |
| `gateDuty` | float | 0.0-1.0 | 0.5 | Duty cycle (fraction of time drawing) |
| `hueSpeed` | float | 0.0-1.0 | 0.2 | Color cycling rate |

### Lissajous Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `freqX1` | float | 0.1-5.0 | 3.14159 | Primary X frequency |
| `freqY1` | float | 0.1-5.0 | 1.0 | Primary Y frequency |
| `freqX2` | float | 0.0-5.0 | 0.72834 | Secondary X frequency (0 = disabled) |
| `freqY2` | float | 0.0-5.0 | 2.781374 | Secondary Y frequency (0 = disabled) |
| `offsetX` | float | 0-2π | 0.3 | Phase offset for secondary X |
| `offsetY` | float | 0-2π | 3.47912 | Phase offset for secondary Y |

### Spiral Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `spiralGrowth` | float | -0.1-0.1 | 0.01 | Radius change per radian |
| `spiralTurns` | float | 1.0-10.0 | 3.0 | Rotations per phase cycle |

## Modulation Candidates

- **speed**: cursor velocity along path
- **amplitude**: path size grows/shrinks
- **freqX1/freqY1**: primary path shape morphs
- **freqX2/freqY2**: secondary harmonic intensity (0 = simple, non-zero = complex)
- **thickness**: stroke weight pulses
- **gateDuty**: controls how much drawing occurs (low = sparse, high = dense)
- **gateFreq**: rhythm of stroke fragments
- **hueSpeed**: color cycling rate

## Notes

- Phase accumulator persists across frames - cursor picks up where it left off
- When switching path types, reset phase to avoid position discontinuity
- Feedback decay rate controls how long strokes persist before fading
- Multiple parametric trail drawables with different settings create layered complexity
- DrawableBase already provides position (x, y) for screen placement
