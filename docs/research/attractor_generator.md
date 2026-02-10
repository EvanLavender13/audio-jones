# Attractor Generator

A single chaotic trajectory drawn as a glowing distance-field line that accumulates into luminous trails. Velocity along the curve maps to hue — fast turns burn warm, slow arcs cool — while active semitones selectively brighten matching hue bands, making the attractor breathe with the music.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage, generator blend path (two-pass: scratch → blend)
- **Chosen Approach**: Full — supports all five attractor types, velocity-hue coloring, FFT-driven semitone flare, feedback accumulation with tone mapping

## References

- **Lorenz attractor (Shadertoy, user-pasted)** — distance-field line drawing, additive accumulation with temporal fade, velocity→HSL coloring, filmic tone mapping. Two-buffer architecture (Buffer A accumulates, Image tone-maps).
- **Dadras attractor (Shadertoy, user-pasted)** — same rendering framework applied to five-parameter Dadras system. Demonstrates the technique generalizes across attractor types with only the derivative function and view constants changing.
- **Existing codebase `shaders/attractor_agents.glsl`** — Lorenz, Rossler, Aizawa, Thomas derivative functions and per-attractor projection centers/scales. Source of truth for ODE formulas and parameter ranges.

## Algorithm

### Core Loop (Fragment Shader, Per Pixel)

The attractor state (one `vec3` position) persists in pixel (0,0) of a feedback texture. Each frame:

1. Read state from pixel (0,0)
2. Integrate forward N steps (Euler), producing N line segments
3. For each pixel: find distance to nearest segment via `dfLine()`, record velocity magnitude at that segment
4. Convert distance → brightness via `smoothstep` + Gaussian core
5. Map velocity → hue (HSL), sample FFT for semitone-matched intensity boost
6. Composite: `newColor * brightness + previousFrame * fade`
7. Write updated state back to pixel (0,0)

### Distance-Field Line Drawing

For each integration step, `dfLine(a, b, p)` computes the closest distance from pixel `p` to line segment `a→b`. The minimum distance across all segments drives brightness:

```
brightness = (intensity / speed) * smoothstep(focus / resolution.y, 0.0, dist)
           + (intensity / 8.5) * exp(-1000.0 * dist * dist)
```

The `smoothstep` term creates the soft body; the `exp` term adds a bright Gaussian core along the exact line.

### Velocity-to-Hue Color Mapping

At the closest segment, compute the attractor derivative magnitude as the speed scalar. Normalize against a max speed constant, then map to a hue range:

```
speedNorm = clamp(speed / maxSpeed, 0.0, 1.0)
hue = hueMax - speedNorm * (hueMax - hueMin) + time * hueShiftSpeed
```

Convert via HSL→RGB. Slow regions get `hueMax` (cool), fast regions get `hueMin` (warm). The `hueShiftSpeed` term cycles the entire palette over time.

### Semitone Flare (Audio Reactivity)

The velocity→hue mapping divides the hue range into regions. Each region corresponds roughly to a pitch class. When a semitone is active in the FFT:

- Sample `fftTexture` at the bin for that semitone's frequency
- If the hue at the current pixel falls within that semitone's hue band, boost intensity and saturation
- Active notes flare bright along their hue region of the trajectory; quiet notes remain at base intensity

This replaces the Shadertoy's random "blink" with musically-driven pulses. The natural velocity variation along the attractor creates spatial hue bands that light up selectively when their corresponding notes play.

### Feedback Architecture

The generator owns a persistent `RenderTexture2D` (ping-pong pair or copy-back). Each frame:

1. **Accumulate pass**: Fragment shader reads previous feedback texture, draws new line segments additively, applies fade (`prev * fadeAmount`), writes state to pixel (0,0)
2. **Output pass**: Fragment shader reads feedback texture, applies filmic tone mapping `1.0 - exp(-col * exposure)`, writes to `generatorScratch`

The blend compositor then composites `generatorScratch` onto the scene via the standard generator blend path.

### Tone Mapping

Filmic exponential compression recovers dynamic range from accumulated HDR trails:

```
col = 1.0 - exp(-col * exposure)
```

Areas the attractor revisits accumulate brightness beyond 1.0. The tone map compresses these into saturated color without clipping. An `exposure` parameter controls the curve — higher values push more of the trail toward full brightness.

## Attractor Types

Five attractor types, each with its own derivative function, default projection plane, and view center:

### Lorenz
- **Parameters**: sigma (10.0), rho (28.0), beta (8/3)
- **Derivative**: `dx = sigma*(y-x), dy = x*(rho-z)-y, dz = x*y - beta*z`
- **Default projection**: XZ, center (0, 27), scale 0.025

### Rossler
- **Parameters**: a (0.2), b (0.2), c (5.7)
- **Derivative**: `dx = -y-z, dy = x + a*y, dz = b + z*(x-c)`
- **Default projection**: XY, center (0, 0), scale 0.05

### Aizawa
- **Parameters**: a (0.95), b (0.7), c (0.6), d (3.5), e (0.25), f (0.1)
- **Derivative**: `dx = (z-b)*x - d*y, dy = d*x + (z-b)*y, dz = c + a*z - z^3/3 - (x^2+y^2)*(1+e*z) + f*z*x^3`
- **Default projection**: XZ, center (0, 0), scale 0.4

### Thomas
- **Parameters**: b (0.208186)
- **Derivative**: `dx = sin(y) - b*x, dy = sin(z) - b*y, dz = sin(x) - b*z`
- **Default projection**: XY, center (0, 0), scale 0.2

### Dadras (new)
- **Parameters**: a (3.0), b (2.7), c (1.7), d (2.0), e (9.0)
- **Derivative**: `dx = y - a*x + b*y*z, dy = c*y - x*z + z, dz = d*x*y - e*z`
- **Default projection**: XY, center (0, 0), scale 0.05

User-selectable projection plane (XY, XZ, YZ) plus 3D rotation angles override the defaults.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| attractorType | enum | 0-4 | 0 (Lorenz) | Selects ODE system |
| projectionPlane | enum | XY/XZ/YZ | per-type | Which 2D slice to view |
| steps | int | 32-256 | 96 | Integration steps per frame — more = longer visible trail segment |
| speed | float | 0.05-2.0 | 0.25 | Integration timestep multiplier — faster traversal |
| viewScale | float | 0.005-0.2 | 0.025 | World-to-screen scale |
| centerX, centerY | float | -50 to 50 | per-type | View center offset in attractor space |
| intensity | float | 0.01-1.0 | 0.18 | Line brightness |
| focus | float | 0.5-8.0 | 2.0 | Line sharpness (higher = thinner) |
| fade | float | 0.95-0.999 | 0.985 | Trail persistence per frame |
| exposure | float | 0.5-5.0 | 2.5 | Tone map exposure |
| hueMin | float | 0-360 | 30 | Warm end of velocity hue range (degrees) |
| hueMax | float | 0-360 | 200 | Cool end of velocity hue range (degrees) |
| hueShiftSpeed | float | 0-60 | 15 | Palette rotation speed (degrees/sec) |
| saturation | float | 0.0-1.0 | 0.85 | Base color saturation |
| lightness | float | 0.0-1.0 | 0.55 | Base color lightness |
| maxSpeed | float | 5-200 | 50 | Velocity normalization ceiling |
| rotationAngleX/Y/Z | float | +/- pi | 0 | Static 3D rotation (radians) |
| rotationSpeedX/Y/Z | float | +/- 2 | 0 | Rotation rate (rad/sec) |
| flareGain | float | 0.0-3.0 | 1.0 | Semitone flare intensity multiplier |
| flareCurve | float | 0.5-4.0 | 2.0 | FFT magnitude response curve (power) |
| baseFreq | float | 20-200 | 55.0 | A1 reference frequency for semitone mapping |
| numOctaves | int | 1-8 | 4 | Octave range for semitone sampling |
| blendMode | enum | 16 modes | Screen | Compositing blend mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Blend strength |
| *Per-attractor params* | float | per-type | per-type | ODE constants (sigma, rho, beta, etc.) |

## Modulation Candidates

- **speed**: faster/slower traversal changes trail density and velocity distribution
- **intensity**: overall brightness scaling
- **focus**: line width — thicker lines at low values, hair-thin at high
- **fade**: trail length — low values = short afterglow, high values = long persistent trails
- **exposure**: tone map aggressiveness — low = dim moody, high = blown-out bright
- **hueMin / hueMax**: shifts the velocity color endpoints — changes the palette
- **hueShiftSpeed**: faster/slower palette cycling
- **saturation / lightness**: color vibrancy
- **viewScale**: zoom level
- **rotationSpeedX/Y/Z**: tumbling speed of the 3D view
- **rotationAngleX/Y/Z**: static view angle offsets
- **flareGain**: audio reactivity strength
- **blendIntensity**: scene presence
- **Per-attractor params** (sigma, rho, beta, etc.): deforms the attractor shape — chaotic systems produce dramatically different trajectories from small parameter changes

### Interaction Patterns

- **Cascading threshold — flareGain gates semitone visibility**: At low `flareGain`, the attractor shows pure velocity coloring. As `flareGain` rises, semitone-matched hue bands begin to pulse. Musical dynamics (verse vs chorus) determine which bands light up and how strongly — quiet passages show the bare attractor, loud passages make it flare across multiple hue bands simultaneously.

- **Competing forces — fade vs speed**: `fade` controls how long trails persist; `speed` controls how fast new trail is drawn. High fade + low speed = dense, slowly-built luminous cloud. Low fade + high speed = a sharp racing line with minimal trail. Modulating both from different audio sources creates tension — bass sustains trails while treble drives traversal, producing verse/chorus texture shifts.

- **Resonance — attractor params + rotationSpeed**: Small changes to ODE constants (e.g., Lorenz `rho`) push the attractor between single-wing and butterfly orbits. When parameter shifts coincide with rotation changes, the visual transitions between radically different trajectory shapes seen from different angles — rare moments of alignment create dramatic structural shifts.

## Notes

- **First generator with feedback**: Requires a persistent `RenderTexture2D` pair for accumulation. The pipeline dispatch needs modification to handle the accumulate→output two-sub-pass before the standard blend pass.
- **Resize handling**: The feedback texture must reallocate on resolution change. Trail state resets on resize (acceptable — it rebuilds within seconds).
- **State divergence**: Chaotic systems can diverge to infinity from numerical instability. Include an explosion guard — if the state exceeds a threshold, reset to the attractor's start position.
- **Per-attractor defaults**: Each attractor type needs its own default center, scale, projection, and speed. Switching types should load these defaults.
- **Existing attractor flow sim**: This generator coexists alongside the particle simulation. They serve different aesthetics — sharp coherent lines vs diffuse particle clouds.
- **Performance**: One fullscreen fragment pass with N integration steps per pixel. At 96 steps and 1080p, that's ~200M distance evaluations per frame. The Shadertoy references run smoothly at this count, but `steps` should be tunable for lower-end GPUs.
