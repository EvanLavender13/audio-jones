# Color Capabilities Research

Research into advanced color modes for waveform visualization: gradients, rainbow effects, and audio-reactive coloring.

## Current Implementation

Each `WaveformConfig` stores a single raylib `Color` (RGBA, 4 bytes). The draw functions (`DrawWaveformLinear`, `DrawWaveformCircular`) pass this color to `DrawLineEx()` per segment. Colors flow through the blur pipeline unchanged.

Reference: `src/waveform.h:24`, `src/waveform.c:116-178`

## Color Mode Categories

### Static Modes

| Mode | Description | Inputs |
|------|-------------|--------|
| Solid | Single color (current behavior) | 1 RGBA value |
| Two-color gradient | Linear interpolation between endpoints | 2 colors + interpolation method |
| Multi-stop gradient | N colors at specified positions | N color-position pairs |
| Preset gradients | Named presets (fire, ocean, neon) | Preset ID |

### Dynamic Modes

| Mode | Description | Inputs |
|------|-------------|--------|
| Rainbow sweep | Hue varies by position along waveform | Hue offset, range, direction |
| Time-animated | Hue rotates over time | Speed, range |
| Amplitude-reactive | Color maps to sample amplitude | Low/high colors, mapping curve |
| Frequency-reactive | Color maps to frequency content | Frequency bands, per-band colors |
| Beat-sync | Color pulses on detected beats | Base color, flash color, decay rate |

## Color Space Considerations

### RGB Interpolation

Linear RGB interpolation produces muddy greys between complementary colors. Example: cyan (0,255,255) to red (255,0,0) passes through grey (127,127,127) at midpoint.

### HSV/HSL Interpolation

Interpolating in HSV maintains saturation through the transition. Hue wrapping requires care: interpolating from hue 350° to 10° should go through 0° (20° arc), not through 180° (340° arc). Always use the shorter arc unless explicitly requesting the long path.

raylib provides `ColorFromHSV(hue, saturation, value)` and `ColorToHSV()`.

### Perceptually Uniform Spaces (LCH, OKLCH)

These spaces ensure equal numeric distances produce equal perceived differences. Prevents banding artifacts in gradients. More complex to implement; requires conversion functions not built into raylib.

Source: [Alan Zucconi - Colour Interpolation](https://www.alanzucconi.com/2016/01/06/colour-interpolation/)

## Rainbow Generation

### CPU-side HSV sweep

```c
for (int i = 0; i < segmentCount; i++) {
    float t = (float)i / (segmentCount - 1);
    float hue = fmodf(baseHue + t * hueRange, 360.0f);
    Color c = ColorFromHSV(hue, saturation, value);
    DrawLineEx(start, end, thickness, c);
}
```

Cost: One `ColorFromHSV` call per segment. At 256 segments, negligible overhead.

### GPU-side (GLSL)

```glsl
vec3 HueToRGB(float hue) {
    vec3 h = vec3(hue, hue + 1.0/3.0, hue + 2.0/3.0);
    return clamp(6.0 * abs(h - floor(h) - 0.5) - 1.0, 0.0, 1.0);
}
```

Requires passing position data to fragment shader. More complex integration with current architecture.

Source: [Stack Overflow - HSV Rainbow](https://stackoverflow.com/questions/5162458/fade-through-more-more-natural-rainbow-spectrum-in-hsv-hsb)

## Audio-Reactive Color Techniques

### Amplitude Mapping

Map sample amplitude to color gradient position. Louder samples appear as one color, quieter as another.

```c
float amplitude = fabsf(sample);  // 0.0 to 1.0 normalized
Color c = LerpColorHSV(quietColor, loudColor, amplitude);  // HSV interpolation
```

Perceptual consideration: Human loudness perception is logarithmic. Apply `log10()` or power curve for more natural feel.

### Frequency Band Mapping

Divide spectrum into bands, assign colors per band:

| Band | Frequency Range | Common Color Association |
|------|-----------------|-------------------------|
| Sub-bass | 20-60 Hz | Deep purple, dark blue |
| Bass | 60-250 Hz | Blue, red |
| Low-mid | 250-500 Hz | Green, orange |
| Mid | 500-2000 Hz | Yellow, green |
| High-mid | 2000-4000 Hz | Orange, cyan |
| Treble | 4000-20000 Hz | White, bright cyan |

Requires FFT analysis of audio buffer. Current implementation captures raw waveform, not frequency data.

Source: [ciphrd - Audio Analysis for Music Visualization](https://ciphrd.com/2019/09/01/audio-analysis-for-advanced-music-visualization-pt-1/)

### Beat Detection

Energy-based algorithm: compare current energy to recent average. Beat triggers when current exceeds average by threshold (typically 1.5x).

```c
float currentEnergy = computeEnergy(samples, count);
float avgEnergy = getRecentAverageEnergy();
bool isBeat = currentEnergy > avgEnergy * 1.5f;
```

On beat: flash to peak color, then decay exponentially to base color.

Complexity: Requires energy history buffer (1-2 seconds). Simple energy detection works for steady beats; complex rhythms need more sophisticated approaches (onset detection, spectral flux).

Source: [Parallelcube - Beat Detection Algorithm](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/)

## UI Patterns

### Mode Selector

Dropdown or radio buttons: Solid | Rainbow | Gradient | Reactive

### Gradient Editor (Multi-stop)

Visual bar showing current gradient. Click to add color stops. Drag stops to reposition. Per-stop color picker appears on selection.

raygui limitation: No built-in gradient editor. Implementation requires custom drawing + hit testing.

### Simplified Alternative

Two-color gradient: show two color pickers side-by-side with preview bar between them. Covers 80% of use cases with minimal UI complexity.

Source: [CSS Gradient Generator](https://cssgradient.io/), [Learn UI Design Gradient Tool](https://www.learnui.design/tools/gradient-generator.html)

## Proposed Data Structures

```c
typedef enum {
    COLOR_MODE_SOLID,
    COLOR_MODE_RAINBOW,
    COLOR_MODE_GRADIENT_2,      // Two-color gradient
    COLOR_MODE_GRADIENT_MULTI,  // N-stop gradient
    COLOR_MODE_AMPLITUDE,
    COLOR_MODE_BEAT
} ColorMode;

typedef struct {
    Color color;
    float position;  // 0.0 to 1.0
} ColorStop;

#define MAX_COLOR_STOPS 8

typedef struct {
    ColorMode mode;

    // Solid mode
    Color solidColor;

    // Rainbow mode
    float rainbowHueOffset;    // Starting hue (0-360)
    float rainbowHueRange;     // Degrees of hue to span
    float rainbowSpeed;        // Degrees per second (0 = static)

    // Gradient mode
    ColorStop stops[MAX_COLOR_STOPS];
    int stopCount;

    // Amplitude mode
    Color amplitudeLow;
    Color amplitudeHigh;
    float amplitudeExponent;   // 1.0 = linear, 2.0 = quadratic

    // Beat mode
    Color beatBase;
    Color beatFlash;
    float beatDecayRate;       // Half-life in seconds
} WaveformColorConfig;
```

## Implementation Complexity Estimates

| Feature | Files Modified | New Code (approx) |
|---------|---------------|-------------------|
| Rainbow mode | waveform.c, ui.c, waveform.h | ~100 lines |
| Two-color gradient | waveform.c, ui.c, waveform.h | ~80 lines |
| Multi-stop gradient | waveform.c, ui.c, waveform.h | ~200 lines |
| Amplitude-reactive | waveform.c, ui.c | ~60 lines |
| Beat detection | New beat.c, waveform.c, ui.c | ~300 lines |
| Preset serialization | preset.cpp | ~50 lines per mode |

## Recommended Implementation Order

1. **Rainbow mode**: Highest visual impact, lowest complexity. Uses existing `ColorFromHSV()`.
2. **Two-color gradient**: Simple extension, covers common use case.
3. **Amplitude-reactive**: Data already available (samples array), straightforward mapping.
4. **Multi-stop gradient**: More complex UI, diminishing returns over two-color.
5. **Beat detection**: Requires new audio analysis subsystem; most complex.

## Design Decisions

- **Gradient interpolation**: HSV. Produces smoother, more vibrant transitions than RGB.
- **Per-waveform configs**: Each waveform stores its own `WaveformColorConfig`. Maximum customization.
- **Preset compatibility**: No existing presets to maintain. New serialization format starts fresh.

## Open Questions

- **Performance**: At 8 waveforms x 256 segments, per-segment color computation adds ~2048 `ColorFromHSV` calls per frame. Likely negligible but needs measurement.

## References

- [Alan Zucconi - Colour Interpolation](https://www.alanzucconi.com/2016/01/06/colour-interpolation/)
- [Stack Overflow - HSV Rainbow](https://stackoverflow.com/questions/5162458/fade-through-more-more-natural-rainbow-spectrum-in-hsv-hsb)
- [ciphrd - Audio Analysis for Music Visualization](https://ciphrd.com/2019/09/01/audio-analysis-for-advanced-music-visualization-pt-1/)
- [Parallelcube - Beat Detection Algorithm](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/)
- [AudioMotion Analyzer](https://www.npmjs.com/package/audiomotion-analyzer/v/2.4.0)
- [CSS Gradient Generator](https://cssgradient.io/)
- [Learn UI Design Gradient Tool](https://www.learnui.design/tools/gradient-generator.html)
- [raylib Default Shader](https://github.com/raysan5/raylib/wiki/raylib-default-shader)
