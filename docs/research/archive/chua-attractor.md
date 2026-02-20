# Chua Attractor

Double-scroll chaotic attractor driven by a piecewise-linear nonlinearity (Chua diode). Two spiral lobes connected by chaotic transitions — particles wind tightly in one lobe then jump unpredictably to the other. The defining visual is the dual-vortex structure where neither lobe dominates for long.

## Classification

- **Category**: Extension to existing SIMULATIONS > Attractor Flow and GENERATORS > Filament > Attractor Lines
- **Pipeline Position**: Same as existing attractors (no new pipeline stage)
- **Compute Model**: Same as existing attractors (compute shader + trail texture for Flow; fragment shader ping-pong for Lines)

## References

- User-provided Shadertoy shader (see Reference Code below)
- Chua circuit canonical parameters: alpha=15.6, gamma=25.58, m0=-2.0, m1=0.0

## Reference Code

```glsl
// Piecewise-linear Chua diode characteristic
float chuaDiode(float x) {
    return M1 * x + 0.5 * (M0 - M1) * (abs(x + 1.0) - abs(x - 1.0));
}

vec3 integrate(vec3 cur, float dt) {
    float g = chuaDiode(cur.x);
    return cur + vec3(
        ALPHA * (cur.y - cur.x - g),
        cur.x - cur.y + cur.z,
        -GAMMA * cur.y
    ) * dt;
}

// Canonical parameters
#define ALPHA 15.6
#define GAMMA 25.58
#define M0 -2.0
#define M1 0.0

// Seeding: half into each scroll lobe (x ≈ ±1.5)
float sign = (pid < NUM_PARTICLES / 2) ? 1.0 : -1.0;
float angle = float(pid) * 6.28318 / float(NUM_PARTICLES);
float r = 0.1;
pos = vec3(sign * 1.5 + r * cos(angle), r * sin(angle), r * sin(angle * 0.7 + 1.0));

// Respawn into alternating lobes
float sign = hash(float(pid) * 7.3 + time * 31.0) < 0.5 ? 1.0 : -1.0;
pos = vec3(sign * 1.5 + r * cos(angle), r * sin(angle), r * sin(angle * 0.7));

// Divergence threshold from reference
length(pos) > 12.0

// Reference view scale
#define VIEW_SCALE 0.15
```

## Algorithm

Adaptation to existing attractor system — follows the established pattern exactly:

### Derivative Function (both shaders)
- Add `chuaDiode(float x)` helper: piecewise-linear `m1*x + 0.5*(m0-m1)*(abs(x+1) - abs(x-1))`
- Add `chuaDerivative(vec3 p)`: `vec3(alpha*(p.y - p.x - chuaDiode(p.x)), p.x - p.y + p.z, -gamma*p.y)`
- Wire into `attractorDerivative()` dispatch (both shaders)
- Uses existing RK4 integration — Euler from reference code is replaced by the codebase's RK4

### Tuning Constants (attractor_lines.fs)
- `STEP_CHUA`: 0.008 (similar to Lorenz — moderately stiff system)
- `SCALE_CHUA`: 3.0 (attractor spans roughly ±3 in x, ±0.5 in y/z — needs scaling up)
- `DIVERGE_CHUA`: 50.0 (double-scroll basin is compact, ~12 units radius)
- Projection plane: XZ (like Lorenz — shows both scroll lobes side by side)

### Seeding (both systems)
- Split agents 50/50 into each scroll lobe at x ≈ ±1.5
- Small random offset (r ≈ 0.1) around lobe centers
- Respawn: random lobe selection on divergence

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| alpha | float | 5.0-30.0 | 15.6 | Primary chaos control — lower values produce periodic orbits, higher values increase inter-lobe transitions |
| gamma | float | 10.0-40.0 | 25.58 | Y-axis coupling strength — controls how tightly particles spiral within each lobe |
| m0 | float | -3.0-0.0 | -2.0 | Inner diode slope — determines the shape of each scroll lobe |
| m1 | float | -1.0-1.0 | 0.0 | Outer diode slope — affects how particles transition between lobes |

## Modulation Candidates

- **alpha**: Drives chaos level — low alpha locks particles into one lobe (periodic), high alpha creates frequent lobe-hopping (chaotic). Most impactful single parameter.
- **gamma**: Changes spiral tightness. Higher values compress the scrolls vertically, lower values stretch them.
- **m0**: Morphs lobe geometry. Moving toward 0 collapses the double-scroll toward a single attractor.
- **m1**: Subtle — shifts the outer region behavior. Combined with m0, can transition between double-scroll, single-scroll, and divergent regimes.

### Interaction Patterns

- **alpha × m0 (cascading threshold)**: When m0 approaches 0, the double-scroll structure degrades. But alpha must also be in a specific range for chaos to exist. Modulating both creates windows where the attractor suddenly "snaps" between organized spirals and chaotic scatter — verse/chorus dynamics.
- **alpha × gamma (competing forces)**: Alpha pushes particles between lobes while gamma pulls them into tighter spirals. High alpha + low gamma = loose chaotic wander. Low alpha + high gamma = tight trapped spirals. The tension between these two defines the visual character.

## Notes

- The piecewise-linear Chua diode is numerically cheaper than smooth attractors (no trig, no exp) — just abs() operations
- The double-scroll is compact (roughly ±3 units) compared to Lorenz (±20 units), so needs a larger scale factor
- m0 and m1 have narrow "interesting" ranges — outside canonical values the system either converges or diverges quickly. The ranges above are tuned to keep the system chaotic.
- With all four params exposed, users can discover non-canonical parameter regimes (single-scroll Chua, etc.)
