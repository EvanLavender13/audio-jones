# Triskelion

Neon circles blooming into a fractal tiled grid through iterated space-folding. Each iteration rotates and scales the coordinate space, folds it into a tiling cell (hex or square), then draws interfering concentric circles. Four independent phase accumulators (rotation, scale, color, circle) with different speeds create dissonant animation that never repeats. Bass drives the large-scale structure and treble sparkles in the fine detail.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator (blend compositor)
- **Section Index**: 10

## Attribution

- **Based on**: "Triskelion" by rrrola
- **Source**: https://www.shadertoy.com/view/dl3SRr
- **License**: CC BY-NC-SA 3.0

## Reference Code

The sole reference is Triskelion. It contains both hex fold (default) and square fold (`#ifdef TETRASKELION`). The adaptation is derived entirely from this code.

```glsl
// Triskelion by rrrola
// https://www.shadertoy.com/view/dl3SRr
// Neon circles blooming into a fractal hexagonal grid.
// Inspiration:
// - https://www.shadertoy.com/view/mly3Dd - Glowing concentric circles + square grid folding
// - https://www.shadertoy.com/view/NtBSRV - Hexagonal cells

//#define TETRASKELION  // if hexagons offend you

vec3 pal(float a) { return 0.5 + cos(3.0*a + vec3(2,1,0)); }  // Biased rainbow color map. Will be squared later.

vec2 fold(vec2 p) {  // Shift and fold into a vertex-centered grid.
#ifdef TETRASKELION
  return fract(p) - 0.5;
#else
  vec4 m = vec4(2,-1, 0,sqrt(3.));
  p.y += m.w/3.0;      // center at vertex
  vec2 t = mat2(m)*p;  // triangular coordinates (x, y, x+y)
  return p - 0.5*mat2(m.xzyw) * round((floor(t) + ceil(t.x+t.y)) / 3.0);  // fold into hexagonal cells
#endif
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  float t = iTime / 4.0, t2 = t * 0.618034, t3 = t * 1.4142135;  // dissonant timers
  mat2 M = mat2(cos(t),sin(t), -sin(t),cos(t)) * (1.0 - 0.1*cos(t2));  // rotation and scale: 0.9 [smooth] .. 1.1 [fractal]

  vec2 p = (2.0*fragCoord - iResolution.xy) / iResolution.y;  // y: -1 .. 1
  float d = 0.5*length(p);  // animation phase is based on distance to center

  vec3 sum = vec3(0);
  for (float i = 0.0; i < 24.0; i++) {
    p = fold(M * p);                                            // rotate and scale, fold
    sum += pal(0.01*i - d + t2) / cos(d - t3 + 5.0*length(p));  // interfering concentric circles
    // Use pal(...)/abs(cos(...)) for additive circles. I like the interference effect without the abs.
  }

  fragColor = vec4(0.0002*sum*sum, 1);  // square the sum for better contrast
}
```

## Algorithm

Adapted shader derived mechanically from the reference above. Each substitution is annotated with the reference line it replaces.

```glsl
// Based on "Triskelion" by rrrola
// https://www.shadertoy.com/view/dl3SRr
// License: CC BY-NC-SA 3.0 Unported
// Modified: AudioJones generator with gradient LUT, FFT reactivity, fold mode uniform
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform int foldMode;          // 0=square, 1=hex
uniform int layers;
uniform float circleFreq;
uniform float rotationAngle;   // CPU-accumulated rotation phase
uniform float scaleAngle;      // CPU-accumulated scale oscillation phase
uniform float scaleAmount;
uniform float colorPhase;      // CPU-accumulated color cycling phase
uniform float circlePhase;     // CPU-accumulated circle interference phase
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const int BAND_SAMPLES = 4;

// Fold space into a vertex-centered grid.
// Both paths verbatim from reference fold().
vec2 fold(vec2 p) {
  if (foldMode == 0) {
    // Square cells (reference: #ifdef TETRASKELION path)
    return fract(p) - 0.5;
  } else {
    // Hexagonal cells (reference: #else path)
    vec4 m = vec4(2.0, -1.0, 0.0, sqrt(3.0));
    p.y += m.w / 3.0;
    vec2 t = mat2(m) * p;
    return p - 0.5 * mat2(m.xzyw) * round((floor(t) + ceil(t.x + t.y)) / 3.0);
  }
}

void main() {
  // Centered coordinates (reference: (2.0*fragCoord - iResolution.xy) / iResolution.y)
  vec2 p = fragTexCoord * 2.0 - 1.0;
  p.x *= resolution.x / resolution.y;

  // Reference: float d = 0.5*length(p)
  float d = 0.5 * length(p);

  // Reference: mat2(cos(t),sin(t),-sin(t),cos(t)) * (1.0 - 0.1*cos(t2))
  // Replace: t -> rotationAngle, 0.1 -> scaleAmount, t2 -> scaleAngle
  float ca = cos(rotationAngle), sa = sin(rotationAngle);
  mat2 M = mat2(ca, sa, -sa, ca) * (1.0 - scaleAmount * cos(scaleAngle));

  float fLayers = float(layers);
  float logRatio = log(maxFreq / baseFreq);

  // Reference: for (float i = 0.0; i < 24.0; i++)
  // Replace: 24.0 -> layers
  vec3 sum = vec3(0.0);
  for (int i = 0; i < layers; i++) {
    float fi = float(i);

    // Reference: p = fold(M * p)  (verbatim)
    p = fold(M * p);

    // FFT: map iteration i to a frequency band in log space (standard generator pattern)
    float t0 = fi / fLayers;
    float t1 = (fi + 1.0) / fLayers;
    float freqLo = baseFreq * exp(t0 * logRatio);
    float freqHi = baseFreq * exp(t1 * logRatio);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    for (int b = 0; b < BAND_SAMPLES; b++) {
      float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
      if (bin <= 1.0)
        energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    float bright = baseBright +
        pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

    // Reference: pal(0.01*i - d + t2)
    // Replace: pal() -> gradientLUT, 0.01*i -> fi/fLayers (spread across full gradient), t2 -> colorPhase
    vec3 col = texture(gradientLUT, vec2(fract(fi / fLayers - d + colorPhase), 0.5)).rgb;

    // Reference: sum += pal(...) / cos(d - t3 + 5.0*length(p))
    // Replace: t3 -> circlePhase, 5.0 -> circleFreq
    // bright multiplier adds FFT reactivity per iteration
    sum += col * bright / cos(d - circlePhase + circleFreq * length(p));
  }

  // Reference: fragColor = vec4(0.0002*sum*sum, 1)  (tuned for 24 iterations)
  // Normalize sum to 24-iteration equivalent so 0.0002 constant stays valid at any layer count
  sum *= 24.0 / fLayers;
  finalColor = vec4(0.0002 * sum * sum, 1.0);
}
```

### Substitution table

| Reference | Adapted | Reason |
|-----------|---------|--------|
| `#ifdef TETRASKELION` / `#else` | `if (foldMode == 0)` / `else` | Runtime mode switch |
| `iTime / 4.0` (rotation) | `rotationAngle` uniform | CPU-accumulated, separate speed |
| `t * 0.618034` (scale + color timer) | `scaleAngle` and `colorPhase` uniforms | Split into two independent accumulators |
| `t * 1.4142135` (circle timer) | `circlePhase` uniform | CPU-accumulated, separate speed |
| `0.1` (scale deviation) | `scaleAmount` uniform | User-controllable |
| `24.0` (iteration count) | `layers` uniform | User-controllable |
| `5.0` (circle frequency) | `circleFreq` uniform | User-controllable |
| `pal(0.01*i - d + t2)` | `texture(gradientLUT, vec2(fract(fi/fLayers - d + colorPhase), 0.5)).rgb` | Gradient LUT replaces hardcoded rainbow |
| `0.0002*sum*sum` | `sum *= 24.0/fLayers; 0.0002*sum*sum` | Normalization scales with layer count |

### Timer architecture

The reference uses three coupled timers derived from one base (`t = iTime/4.0`):
- `t` = rotation (1.0x base)
- `t2 = t * 0.618034` = scale oscillation + color cycling (golden ratio)
- `t3 = t * 1.4142135` = circle interference phase (sqrt(2))

The adaptation splits these into **four independent CPU-accumulated phases**, each with its own speed uniform:
- `rotationAngle += rotationSpeed * deltaTime` (reference: `t`)
- `scaleAngle += scaleSpeed * deltaTime` (reference: `t2` in scale term)
- `colorPhase += colorSpeed * deltaTime` (reference: `t2` in pal() term)
- `circlePhase += circleSpeed * deltaTime` (reference: `t3`)

The reference's `t2` served double duty for both scale and color. Splitting them into separate accumulators gives more modulation control while preserving the dissonant-timer character (four independent speeds that never sync).

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| foldMode | int | 0-1 | 1 | Tiling type: 0=square, 1=hex |
| layers | int | 4-32 | 16 | Iteration depth / fractal detail |
| circleFreq | float | 1.0-20.0 | 5.0 | Concentric circle ring density |
| rotationSpeed | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.25 | Fold matrix rotation rate (rad/s) |
| scaleSpeed | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.15 | Scale oscillation rate (rad/s) |
| scaleAmount | float | 0.01-0.3 | 0.1 | Scale deviation from 1.0 (fractal tightness) |
| colorSpeed | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.15 | Color cycling rate through gradient (rad/s) |
| circleSpeed | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.35 | Circle interference phase rate (rad/s) |
| baseFreq | float | 27.5-440 | 55.0 | FFT low frequency bound (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | FFT high frequency bound (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT magnitude power curve (contrast) |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness per iteration |

Default speeds derived from reference: `t = iTime/4.0` gives rotation at 0.25 rad/s, `t2 = t*0.618` gives scale/color at ~0.15 rad/s, `t3 = t*1.414` gives circles at ~0.35 rad/s.

## Modulation Candidates

- **scaleAmount**: controls the fractal/smooth boundary — small values = smooth circles, large values = dense fractal grid
- **circleFreq**: ring density, changes the interference pattern character
- **rotationSpeed**: animation pace, higher = faster morphing
- **circleSpeed**: circle phase rate, creates pulsing/breathing when modulated
- **gain**: overall audio reactivity intensity

### Interaction Patterns

**Cascading threshold (scaleAmount x gain)**: When `scaleAmount` is near 0, the pattern is smooth concentric circles regardless of audio. As `scaleAmount` increases past ~0.05, the fractal grid structure emerges and FFT energy starts to visibly modulate distinct grid cells. Audio reactivity becomes meaningfully visible only when the fractal structure is present to carry it — creating a gate where quiet passages show smooth circles and loud passages bloom into reactive fractal grids.

**Competing forces (rotationSpeed x scaleAmount)**: Fast rotation with low scale = smooth spinning vortex. Slow rotation with high scale = static crystalline fractal. The visual character is the tension between these two — neither dominates, and modulating both creates dynamic transitions between fluid and crystalline states.

## Notes

- The hex fold involves a matrix multiply + round operation per iteration — at 32 iterations this is still very cheap (no texture lookups in the fold, just ALU)
- The `1/cos()` term can produce very large values near cos=0, but squaring the sum and dividing by a large constant normalizes the output. The `sum *= 24.0/fLayers` normalization keeps brightness consistent across different layer counts.
- The dissonant timer approach (four independent accumulation speeds) means the animation has extremely long cycle times before repeating, even with constant speed values
- Triangle fold was considered but dropped — only hex and square have reference implementations. Could be added later as an extension.
