# Attractor Lines

Glowing line traces of chaotic attractors drawn via per-pixel distance-field evaluation. Each frame advances the integration state and renders new trajectory segments as soft luminous strokes, blending onto accumulated trails that slowly fade. The result is a continuously-growing neon wireframe of the attractor's shape — Lorenz butterflies, Rossler spirals, Thomas knots — built from thousands of overlapping line segments.

## Classification

- **Category**: GENERATORS > Filament
- **Pipeline Position**: Output stage (same slot as other generators)
- **Self-Feedback**: Yes — owns ping-pong render texture pair for trail persistence and state storage
- **Chosen Approach**: Full — all 5 attractor types, 3D rotation, gradient LUT, complete parameter set matching particle version where applicable

## References

- User-provided Shadertoy code: Lorenz attractor line tracer (96 steps, xz projection, HSL velocity coloring)
- User-provided Shadertoy code: Dadras attractor line tracer (100 steps, xy projection, HSL velocity coloring)
- Existing codebase: `src/simulation/attractor_flow.h` — particle-based attractor config, attractor type enum, system parameters

## Algorithm

### Integration

Euler integration of a 3D ODE system. Each frame advances N steps from the stored state:

```
pos[0] = stored state from pixel (0,0) of previous frame
for i in 0..STEPS:
    derivative = attractorDerivative(pos[i], systemParams)
    pos[i+1] = pos[i] + derivative * dt * speed
```

The final position stores back to pixel (0,0) for the next frame. On first frame, uses the attractor's canonical starting point.

### Attractor Systems

Each system defines a `vec3 derivative(vec3 p)` function:

**Lorenz** (sigma=10, rho=28, beta=8/3):
- dx = sigma * (y - x)
- dy = x * (rho - z) - y
- dz = x * y - beta * z

**Rossler** (a=0.2, b=0.2, c=5.7):
- dx = -y - z
- dy = x + a * y
- dz = b + z * (x - c)

**Aizawa** (a=0.95, b=0.7, c=0.6, d=3.5, e=0.25, f=0.1):
- dx = (z - b) * x - d * y
- dy = d * x + (z - b) * y
- dz = c + a * z - z^3/3 - (x^2 + y^2) * (1 + e * z) + f * z * x^3

**Thomas** (b=0.208186):
- dx = sin(y) - b * x
- dy = sin(z) - b * y
- dz = sin(x) - b * z

**Dadras** (a=3, b=2.7, c=1.7, d=2, e=9):
- dx = y - a * x + b * y * z
- dy = c * y - x * z + z
- dz = d * x * y - e * z

### 3D Rotation and Projection

Before distance evaluation, project each 3D point to 2D:
1. Subtract hardcoded per-attractor center offset (e.g., Lorenz orbits around (0, 0, 27), Dadras around origin)
2. Apply 3x3 rotation matrix (accumulated from rotationSpeed * deltaTime)
3. Scale by viewScale to map attractor coordinates to screen UV space
4. Offset by screen position (x, y)

### Distance-Field Line Evaluation

For each pixel, find distance to the nearest line segment among all N segments:

```
d = 1e6
for each segment (a, b):
    ab = b - a
    t = clamp(dot(p - a, ab) / dot(ab, ab), 0, 1)
    segD = distance(a + ab * t, p)
    if segD < d:
        d = segD
        bestSpeed = length(derivative at b)
```

### Brightness

Two-term brightness combining a soft glow (smoothstep falloff) and a sharp core (Gaussian):

```
c = intensity * smoothstep(focus / resolution.y, 0.0, d)
c += (intensity / 8.5) * exp(-1000.0 * d * d)
```

### Coloring

Velocity of the nearest segment maps to gradient LUT coordinate:
```
speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0)
color = texture(gradientLUT, vec2(speedNorm, 0.5)).rgb
```

### Trail Persistence

Each frame blends new contribution onto previous frame:
```
result = lineColor * brightness + previousFrame * fade
```

The `fade` parameter (0.95–0.999) controls trail half-life. Lower values give short sharp trails; higher values build dense accumulated shapes.

## Parameters

### Attractor System

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| attractorType | enum | 0–4 | LORENZ | Selects ODE system (Lorenz/Rossler/Aizawa/Thomas/Dadras) |
| sigma | float | 1–30 | 10.0 | Lorenz: coupling strength between x/y |
| rho | float | 10–50 | 28.0 | Lorenz: drives z-axis folding, controls wing separation |
| beta | float | 0.5–5 | 2.667 | Lorenz: z damping rate |
| rosslerC | float | 2–12 | 5.7 | Rossler: controls spiral-to-chaos transition |
| thomasB | float | 0.1–0.3 | 0.208 | Thomas: damping, lower = more chaotic |
| dadrasA | float | 1–5 | 3.0 | Dadras param a |
| dadrasB | float | 1–5 | 2.7 | Dadras param b |
| dadrasC | float | 0.5–3 | 1.7 | Dadras param c |
| dadrasD | float | 0.5–4 | 2.0 | Dadras param d |
| dadrasE | float | 4–15 | 9.0 | Dadras param e |

### Line Tracing

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| steps | int | 32–256 | 96 | Integration steps per frame — more steps = longer segment per frame, fills shape faster |
| timeScale | float | 0.001–0.1 | 0.01 | Euler step size — smaller = more accurate curves, larger = faster traversal |
| viewScale | float | 0.005–0.1 | 0.025 | Attractor-to-screen scale factor |

Per-attractor center offsets are hardcoded internally (each system orbits a known region of 3D space).

### Appearance

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| intensity | float | 0.01–1.0 | 0.18 | Line brightness |
| fade | float | 0.9–0.999 | 0.985 | Trail persistence per frame — higher = denser accumulated shape |
| focus | float | 0.5–5.0 | 2.0 | Line sharpness — larger = wider soft glow, smaller = thinner crisp lines |
| maxSpeed | float | 5–200 | 50.0 | Velocity normalization ceiling for LUT mapping |

### Transform

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| x | float | 0–1 | 0.5 | Screen X position |
| y | float | 0–1 | 0.5 | Screen Y position |
| rotationAngleX | float | -PI–PI | 0.0 | Static rotation around X axis |
| rotationAngleY | float | -PI–PI | 0.0 | Static rotation around Y axis |
| rotationAngleZ | float | -PI–PI | 0.0 | Static rotation around Z axis |
| rotationSpeedX | float | -2–2 | 0.0 | Rotation rate around X (rad/s) |
| rotationSpeedY | float | -2–2 | 0.0 | Rotation rate around Y (rad/s) |
| rotationSpeedZ | float | -2–2 | 0.0 | Rotation rate around Z (rad/s) |

### Standard Generator

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| blendMode | enum | — | SCREEN | How generator composites onto scene |
| color | ColorConfig | — | — | Gradient LUT palette selection |

## Modulation Candidates

- **timeScale**: faster/slower traversal creates denser or sparser segment traces
- **intensity**: overall line brightness
- **fade**: trail length — low values make the shape dissolve, high values build solid forms
- **focus**: line width pulsing between thin crisp traces and wide glowing strokes
- **viewScale**: zoom in/out on the attractor shape
- **sigma/rho/beta**: deforming the Lorenz attractor's wing geometry in real time
- **rosslerC**: driving Rossler between periodic orbits and chaos
- **thomasB**: Thomas damping — lower values explode into chaos
- **rotationSpeedX/Y/Z**: tumbling the 3D projection

### Interaction Patterns

- **Competing Forces (timeScale vs fade)**: Higher timeScale fills the shape faster with longer segments per frame, while lower fade erases trails faster. Their balance determines whether the visualization shows a sparse moving tracer or a dense solid shape. Modulating both creates breathing density cycles.
- **Cascading Threshold (intensity vs fade)**: At very low fade values, lines disappear almost immediately. Intensity must exceed a brightness floor to remain visible against the rapid decay. Boosting intensity while fade drops creates brief bright flashes that vanish instantly.

## Notes

- **Performance**: The distance-field loop evaluates `steps` line segments per pixel per frame. At 96 steps and 1080p, that's ~200M segment-distance evaluations. This is GPU-bound but straightforward ALU work — no texture fetches in the inner loop. If performance is tight, reduce steps.
- **Self-feedback architecture**: The generator owns a ping-pong render texture pair. Each frame: read from texture A (previous trails + state), render to texture B (new trails), swap. The final output composites texture B onto the scene via blendMode.
- **State storage in pixel (0,0)**: The attractor's current 3D position stores in the RGB channels of pixel (0,0) of the feedback texture. This avoids CPU↔GPU readback. On resize, the state resets (acceptable — the shape rebuilds within seconds).
- **Center offset per attractor**: Each attractor type orbits a different region of 3D space. Per-type center offsets are hardcoded constants (e.g., Lorenz orbits around (0, 0, 27), Dadras around the origin). The shader subtracts these before rotation/projection so the attractor appears centered.
- **Dadras addition**: Dadras (ATTRACTOR_DADRAS) extends the existing AttractorType enum. The 5 Dadras params (a–e) only display in UI when Dadras is selected, matching how Lorenz/Rossler/Thomas params are conditionally shown.
