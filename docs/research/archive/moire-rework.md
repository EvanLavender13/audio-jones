# Moiré Rework

Bring the Moiré Interference transform up to feature parity with the Moiré Generator: per-layer controls, grating shape modes (stripes/circles/grid), and a shared 4-profile wave dropdown (sine/square/triangle/sawtooth) replacing the generator's bool toggle.

## Classification

- **Category**: TRANSFORMS > Warp (Interference), GENERATORS > Texture (Generator)
- **Scope**: Rework of two existing effects — no new pipeline slots

## Current State

### Moiré Generator (fully featured)
- 3 grating shapes: stripes, circles, grid
- 2 wave profiles: sine (smooth), square (sharp) via bool `sharpMode`
- 4 independent layers with: frequency, angle, rotationSpeed, warpAmount, scale, phase
- Gradient LUT color output

### Moiré Interference (prototype, mostly hardcoded)
- 1 grating shape: stripes (hardcoded)
- 1 wave profile: sine (hardcoded)
- Layer count works but all parameters are global or derived from hardcoded ratios
- `blendMode` uniform is leftover dead code from the old shader
- `scaleDiff` repurposed as crude global frequency multiplier

## Reference Code

Current prototype shader (wave interference displacement, working):

```glsl
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float rotationAngle;
uniform float scaleDiff;
uniform int layers;
uniform int blendMode;
uniform float centerX;
uniform float centerY;
uniform float rotationAccum;
uniform vec2 resolution;

out vec4 finalColor;

#define TAU 6.28318530718

void main()
{
    vec2 center = vec2(centerX, centerY);
    vec2 centered = fragTexCoord - center;
    float aspect = resolution.x / resolution.y;

    float freq = scaleDiff * 15.0;
    float amplitude = 0.05 / sqrt(float(layers));

    vec2 displacement = vec2(0.0);

    for (int i = 0; i < layers; i++) {
        float drift = rotationAccum * (1.0 + 0.4 * float(i));
        float angle = rotationAngle * float(i) + drift;
        vec2 dir = vec2(cos(angle), sin(angle));

        vec2 pos = centered;
        pos.x *= aspect;
        float wave = sin(dot(pos, dir) * freq * TAU);

        vec2 perp = vec2(-dir.y, dir.x);
        perp.x /= aspect;
        displacement += perp * wave * amplitude;
    }

    vec2 uv = fragTexCoord + displacement;
    uv = 1.0 - abs(mod(uv, 2.0) - 1.0);

    finalColor = texture(texture0, uv);
}
```

Moiré Generator grating functions (existing, working):

```glsl
float gratingSmooth(float coord, float freq, float phase) {
    return 0.5 + 0.5 * cos(2.0 * PI * coord * freq + phase);
}

float gratingSharp(float coord, float freq, float phase) {
    return step(0.5, fract(coord * freq + phase / (2.0 * PI)));
}
```

## Algorithm

### Wave Profile Functions (shared concept, both effects)

Replace the bool smooth/sharp toggle with a 4-mode `profileMode` int:

```glsl
// 0=sine, 1=square, 2=triangle, 3=sawtooth
float profile(float t, int mode) {
    if (mode == 1) return sign(sin(t));                          // square
    if (mode == 2) return asin(sin(t)) * (2.0 / PI);            // triangle
    if (mode == 3) return 2.0 * fract(t / TAU + 0.5) - 1.0;    // sawtooth
    return sin(t);                                                // sine
}
```

- Generator: replace `gratingSmooth`/`gratingSharp` routing with `profile()` call
- Interference: replace hardcoded `sin()` displacement with `profile()` call

### Grating Shape Modes (interference — new)

Add `patternMode` int matching the generator's vocabulary:

| Mode | Generator (color) | Interference (displacement) |
|------|-------------------|-----------------------------|
| 0 — Stripes | Linear grating along rotated axis | Planar wave displacement (current behavior) |
| 1 — Circles | Radial grating from center | Concentric ring displacement (push inward/outward) |
| 2 — Grid | X * Y grating product | Crossed X+Y wave displacement |

Interference displacement per mode:
- **Stripes**: project position onto wave direction, displace perpendicular (current)
- **Circles**: use distance from center as coordinate, displace radially
- **Grid**: sum two perpendicular planar waves per layer

### Per-Layer Parameters (interference — new)

Replace global uniforms with per-layer struct matching generator's `MoireLayer`:

| Parameter | Currently | After rework |
|-----------|-----------|--------------|
| frequency | Global `scaleDiff * 15.0` | Per-layer `layer[i].frequency` |
| angle | Derived `rotationAngle * i` | Per-layer `layer[i].angle` |
| rotationSpeed | Hardcoded `1.0 + 0.4*i` ratio | Per-layer `layer[i].rotationSpeed` |
| amplitude | Global `0.05 / sqrt(layers)` | Per-layer `layer[i].amplitude` |
| phase | None | Per-layer `layer[i].phase` |

### Config Struct Changes

**MoireInterferenceConfig** — replace flat globals with per-layer struct:
- Remove: `rotationAngle`, `scaleDiff`, `blendMode`, `animationSpeed`
- Add: `int patternMode`, `int profileMode`, `MoireInterferenceLayerConfig layers[4]`, `int layerCount`
- Keep: `enabled`, `centerX`, `centerY`

**MoireInterferenceLayerConfig** (new):
- `float frequency` — wave spatial frequency
- `float angle` — wave direction (radians)
- `float rotationSpeed` — drift rate (radians/second)
- `float amplitude` — displacement strength
- `float phase` — wave phase offset

**MoireGeneratorConfig** — profile mode swap:
- Remove: `bool sharpMode`
- Add: `int profileMode` (0=sine, 1=square, 2=triangle, 3=sawtooth)

### Files Touched

| File | Change |
|------|--------|
| `src/effects/moire_interference.h` | New layer config struct, new config fields, new effect uniform locations |
| `src/effects/moire_interference.cpp` | Per-layer uniform binding, profile/pattern mode setup, UI rework |
| `shaders/moire_interference.fs` | Profile function, pattern modes, per-layer uniforms |
| `src/effects/moire_generator.h` | `bool sharpMode` → `int profileMode` |
| `src/effects/moire_generator.cpp` | Profile mode uniform binding, UI combo instead of checkbox |
| `shaders/moire_generator.fs` | Profile function replacing smooth/sharp routing |
| `src/config/effect_serialization.cpp` | Update field macros for both configs |

## Parameters

### Moiré Interference (after rework)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| patternMode | int | 0-2 | 0 | Grating shape: stripes/circles/grid |
| profileMode | int | 0-3 | 0 | Wave profile: sine/square/triangle/sawtooth |
| layerCount | int | 2-4 | 2 | Number of active wave layers |
| centerX | float | 0-1 | 0.5 | Effect center X |
| centerY | float | 0-1 | 0.5 | Effect center Y |
| layer[i].frequency | float | 1-30 | 10 | Wave spatial frequency |
| layer[i].angle | float | -PI..PI | varies | Wave direction |
| layer[i].rotationSpeed | float | -PI..PI | 0.3 | Drift rate (rad/s) |
| layer[i].amplitude | float | 0-0.15 | 0.04 | Displacement strength |
| layer[i].phase | float | 0-TAU | 0 | Phase offset |

### Moiré Generator (profile change only)

| Parameter | Type | Range | Default | Change |
|-----------|------|-------|---------|--------|
| profileMode | int | 0-3 | 0 | Replaces `bool sharpMode` |

## Modulation Candidates

- **layer[i].frequency**: wave density shifts — low freq = broad warp bands, high = tight ripple
- **layer[i].amplitude**: displacement intensity — silent sections go calm, loud sections warp hard
- **layer[i].rotationSpeed**: drift rate changes shift the interference geometry
- **layer[i].angle**: direct angle control reshapes fringe pattern
- **profileMode**: not typically modulated but could snap between profiles

### Interaction Patterns

- **Frequency vs angle spread**: when layer frequencies are similar but angles differ, you get classic moire fringes. When frequencies also differ, the fringe pattern becomes more complex and shifts over time.
- **Amplitude gating**: low amplitude layers contribute subtle texture; high amplitude layers dominate. Modulating individual layer amplitudes creates a sense of layers appearing/disappearing.
- **Competing drift rates**: layers drifting at different speeds create constantly evolving interference — the fringe pattern never repeats the same way. Slow speed differences = gradual evolution, large differences = rapid reshuffling.

## Notes

- The generator's `warpAmount` and `scale` per-layer fields have no equivalent in the interference rework — the interference layers are pure wave displacement without secondary UV distortion. Could be added later.
- Sawtooth profile creates asymmetric displacement (shearing) which is visually distinct from the symmetric sine/triangle/square profiles.
- Default layer angles should be spread apart (e.g., 0, PI/3, 2*PI/3, PI) so the effect is immediately visible without tweaking.
- Serialization must handle the `sharpMode` → `profileMode` migration: `from_json` should accept both `bool` (legacy) and `int` (new) for `profileMode` in the generator config, mapping `true` → 1 (square), `false` → 0 (sine).
