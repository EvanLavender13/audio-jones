# Halvorsen Attractor

Three-fold symmetric chaotic attractor where each axis equation has the same structure, cyclically permuting (x, y, z). Produces a triangular-lobed shape when viewed along the (1,1,1) diagonal — visually distinct from the butterfly (Lorenz), spiral (Rossler), or toroidal (Aizawa) forms already in the system. Added as `ATTRACTOR_HALVORSEN`, the 6th entry in both Attractor Lines and Attractor Flow.

## Classification

- **Category**: Extension of existing GENERATORS > Filament (Attractor Lines) and SIMULATIONS (Attractor Flow)
- **Pipeline Position**: Same as existing attractors — no change
- **Scope**: New enum value + derivative function + one config parameter + UI/serialization wiring

## References

- User-provided Shadertoy code (guinetik, "Attractor Study #05: Halvorsen") — complete implementation with equations, integration, and rendering
- Halvorsen system documented in Sprott's "Elegant Chaos" — A=1.89 is the canonical chaotic value

## Reference Code

```glsl
// Halvorsen attractor equations:
//   dx/dt = -A*x - 4*y - 4*z - y^2
//   dy/dt = -A*y - 4*z - 4*x - z^2
//   dz/dt = -A*z - 4*x - 4*y - x^2
// Parameter: A = 1.89

#define A 1.89

vec3 integrate(vec3 cur, float dt) {
    return cur + vec3(
        -A * cur.x - 4.0 * cur.y - 4.0 * cur.z - cur.y * cur.y,
        -A * cur.y - 4.0 * cur.z - 4.0 * cur.x - cur.z * cur.z,
        -A * cur.z - 4.0 * cur.x - 4.0 * cur.y - cur.x * cur.x
    ) * dt;
}

// Starting point: radius 0.5, evenly spaced angles
// Escape radius: 20.0
// Respawn chance: 0.025 per frame
```

## Algorithm

**Adaptation from reference to codebase conventions:**

- Keep the derivative equations verbatim — the cyclic permutation structure is exact
- Replace `#define A` with `uniform float halvorsenA` (fragment shader) / `uniform float halvorsenA` (compute shader), sourced from `AttractorLinesConfig::halvorsenA` / `AttractorFlowConfig::halvorsenA`
- The coupling constant `4.0` stays hardcoded in the shader (not exposed as a parameter)
- Add `derivativeHalvorsen(vec3 p)` to both shaders following the existing per-type derivative function pattern
- Add case 5 to `attractorDerivative()` dispatch in both shaders
- Starting point: `base = vec3(0.5, 0.5, 0.5)` with `r = 1.5` spread (similar to Thomas which also has 3-fold structure)
- Escape radius: `DIVERGE_HALVORSEN = 20.0` in fragment shader; respawn check `length(pos) > 20.0` in compute shader
- Per-attractor tuning constants in fragment shader: `STEP_HALVORSEN`, `SCALE_HALVORSEN` — tune to match the attractor's natural scale (it ranges roughly ±6 on each axis)

**RK4 vs Euler**: The fragment shader already uses RK4 integration for all attractors. The reference uses Euler but that's a Shadertoy simplification — keep RK4.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| halvorsenA | float | 1.0 - 3.0 | 1.89 | Dissipation — lower expands the attractor, higher collapses it. Chaotic at 1.89. |

All other parameters (steps, speed, viewScale, intensity, decayHalfLife, focus, rotation, gradient) are shared across all attractor types and already exist.

## Modulation Candidates

- **halvorsenA**: Modulating around 1.89 makes the triangular lobes breathe — expanding (lower A) and tightening (higher A). Outside ~1.5-2.5 the system loses chaos, so moderate modulation depth works best.

### Interaction Patterns

- **halvorsenA × speed**: At high speed the system traverses the attractor faster, so A modulation causes more dramatic structural shifts per frame. At low speed, A changes produce gentle morphing. Speed acts as an amplifier for A's visual impact.

## Notes

- The three-fold symmetry means the attractor looks best when slowly rotated around the (1,1,1) diagonal — consider default rotationSpeedY ≈ 0.3 for Halvorsen specifically in the UI
- A values below ~1.5 cause the system to diverge (escape radius hit frequently) — respawn keeps it alive but trails become sparse
- A values above ~2.5 cause rapid collapse to a fixed point — visually static
- The sweet spot 1.7-2.1 maintains chaotic motion with visible three-lobed structure
