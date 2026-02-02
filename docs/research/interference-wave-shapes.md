# Interference Wave Shapes (Future Idea)

Alternative wave functions for the Interference generator. The core interference math remains unchanged (sum waves from sources), but the ring character varies with the function.

## Wave Functions

| Function | Formula | Visual Character |
|----------|---------|------------------|
| Sine | `sin(x)` | Smooth continuous waves, classic interference |
| Absolute Sine | `abs(sin(x))` | Double the rings, always positive, sharper nodes |
| Triangle | `2*abs(fract(x)-0.5)-0.5` | Linear ramps, angular/geometric patterns |
| Square | `sign(sin(x))` | Hard binary rings, no gradient, digital/harsh |
| Sawtooth | `fract(x)*2-1` | Asymmetric - gradual fade one way, sharp edge the other |
| Soft Sine | `sin(x)^2` | Wider bright bands, narrower dark transitions |
| Sharp Sine | `sign(sin(x))*pow(abs(sin(x)),8)` | Needle-thin bright rings, wide dark gaps |
| Pulse | `exp(-pow(fract(x)-0.5,2)*20)` | Isolated rings, "droplet ripple" feel |
| Sinc | `sin(x)/max(x,0.001)` | Diffraction pattern - central peak with diminishing side lobes |

## Visual Comparisons

**Sine vs Square:** Square waves create checkerboard-like patterns at source intersections instead of smooth moiré.

**Sine vs Sawtooth:** Sawtooth creates directional "flow" feeling - rings appear to move outward with a leading edge.

**Sine vs Sharp Sine:** Sharp sine looks like thin neon rings floating in darkness rather than continuous gradients.

**Sine vs Pulse:** Pulse creates discrete ring events rather than continuous wave structure.

## Implementation

Would add a `waveShape` enum to config:

```cpp
typedef enum {
    WAVE_SHAPE_SINE,
    WAVE_SHAPE_ABSOLUTE,
    WAVE_SHAPE_TRIANGLE,
    WAVE_SHAPE_SQUARE,
    WAVE_SHAPE_SAWTOOTH,
    WAVE_SHAPE_SOFT,
    WAVE_SHAPE_SHARP,
    WAVE_SHAPE_PULSE,
    WAVE_SHAPE_SINC
} WaveShape;
```

Shader switch:

```glsl
float wave(float x, int shape) {
    if (shape == 0) return sin(x);
    if (shape == 1) return abs(sin(x));
    if (shape == 2) return abs(fract(x / 6.283) - 0.5) * 4.0 - 1.0;
    if (shape == 3) return sign(sin(x));
    if (shape == 4) return fract(x / 6.283) * 2.0 - 1.0;
    if (shape == 5) return sin(x) * sin(x) * sign(sin(x));
    if (shape == 6) return sign(sin(x)) * pow(abs(sin(x)), 8.0);
    if (shape == 7) return exp(-pow(fract(x / 6.283) - 0.5, 2.0) * 20.0) * 2.0 - 1.0;
    if (shape == 8) return sin(x) / max(abs(x), 0.1);
    return sin(x);
}
```

## Status

Not planned for v1. Document preserved for future consideration.
