# Polymorph

A glowing wireframe polyhedron that breathes and shape-shifts. Edges expand from collapsed vertices into clean platonic solid wireframes, then retract and re-emerge as a different shape. At low randomness it pulses on one solid; at high randomness it walks through tetrahedra, cubes, octahedra, dodecahedra, and icosahedra. A freeform slider warps vertex positions from mathematical perfection toward abstract random polyhedra.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator (same slot as Spin Cage)
- **Compute Model**: Fragment shader SDF ray marching (no compute pass)

## Attribution

- **Based on**: "Arknights：Polyhedrons" by yli110
- **Source**: https://www.shadertoy.com/view/sc2GzW
- **License**: CC BY-NC-SA 3.0 Unported

## References

- [hg_sdf library (Mercury)](https://github.com/danielscherzer/SHADER/blob/master/libs/hg_sdf.glsl) — GDF approach for platonic solid SDFs, GDFVectors constant array
- [IQ Distance Functions](https://iquilezles.org/articles/distfunctions/) — sdCapsule, sdOctahedron, general SDF primitives
- Existing codebase: `src/config/platonic_solids.h` — vertex/edge data for all 5 solids
- Existing codebase: `src/effects/spin_cage.cpp` — CPU-side rotation + uniform upload pattern for platonic wireframes

## Reference Code

```glsl
// "Arknights：Polyhedrons" by yli110
// https://www.shadertoy.com/view/sc2GzW
// License: CC BY-NC-SA 3.0 Unported

#define NOR(a) normalize(a)
#define R iResolution.xy
#define PIXEL 3.0/min(R.x,R.y)

float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

float sdfIcosahedron( vec3 center, vec3 p, float l, float t)
{
    vec3 q = p - center;
    q = abs(q);
    float r = 0.5*l, phi = 0.5*(sqrt(5.0)+1.0);
    vec3 a = r*vec3(0,1,phi), b = r*vec3(phi,0,1), c = r*vec3(1,phi,0);
    vec3 n1 = normalize(cross(b,a)), n2 = normalize(cross(c,b)), n3 = normalize(cross(a,c));
    q -= 2.0*n1*max(0.0,dot(q,n1));
    q -= 2.0*n2*max(0.0,dot(q,n2));
    q -= 2.0*n3*max(0.0,dot(q,n3));

    vec3 d = 0.5*(a+b), e = 0.5*(c+b), f = 0.5*(c+a);
    n1 = normalize(cross(e,d)), n2 = normalize(cross(f,e)), n3 = normalize(cross(d,f));
    q -= 2.0*n1*max(0.0,dot(q,n1));
    q -= 2.0*n2*max(0.0,dot(q,n2));
    q -= 2.0*n3*max(0.0,dot(q,n3));

    float t1 = clamp(cos(t),0.0,1.0);
    return min(sdCapsule(q,d+t1*(f-d),f,0.02),min(sdCapsule(q,d,e+t1*(d-e),0.02),sdCapsule(q,e,f+t1*(e-f),0.02)));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = (2.0*fragCoord-iResolution.xy)/min(iResolution.x,iResolution.y);

    float an = 10.0 * iMouse.x / iResolution.x;
    if (iMouse.z <= 0.0) an = 0.5*iTime;

    vec3 ta = vec3(0.0, 0.0, 0.0);
    vec3 ro = ta + vec3(25.0 * sin(an), 5.0, 25.0 * cos(an));

    vec3 ww = normalize(ta-ro);
    vec3 uu = normalize(cross(ww,vec3(0.0,1.0,0.0)));
    vec3 vv = normalize(cross(uu,ww));
    vec3 rd = normalize(p.x*uu + p.y*vv + 1.8*ww);

    float t = 0.0;
    float glow = 0.0;

    for(int i=0; i<80; i++)
    {
        vec3 pos = ro + t*rd;
        float h = sdfIcosahedron(vec3(0),pos,15.0,iTime);

        glow += PIXEL / (0.001+h*h);
        t += h;
        if(t > 100.0) break;
    }
    vec2 uv = fragCoord/iResolution.xy;
    vec3 col = 0.2*(0.5 + 0.5*cos(vec3(0,0,4.0+sin(iTime*0.1))))*glow;

    col = pow(col, vec3(0.4545));

    fragColor = vec4(tanh(col),1.0);
}
```

## Algorithm

### Architecture: Hybrid CPU + Shader

The reference uses icosahedral symmetry folding — elegant but locked to one shape. Supporting all 5 platonic solids with morphing requires either 3 different symmetry groups (tetrahedral, octahedral, icosahedral) with crossfading, or explicit edge capsules. Explicit capsules are simpler: CPU computes vertex positions and edge connectivity (reusing `platonic_solids.h`), shader receives edge data as uniforms and does ray marching + glow accumulation.

Performance: 30 capsules (max) × 80 march steps = 2400 capsule evaluations per pixel. Each capsule SDF is ~10 ops. Acceptable on modern GPUs.

### Substitution Table

| Reference | Polymorph | Notes |
|-----------|-----------|-------|
| `sdfIcosahedron()` symmetry folding + 3 capsules | `sdfEdges()`: min over all capsule edges from uniforms | CPU provides edge endpoints |
| `cos(iTime)` morph oscillation | CPU morph state machine with `slideT` per cycle | Drives expand/collapse animation |
| Hardcoded icosahedron geometry | `uniform vec3 edgeA[30]; uniform vec3 edgeB[30];` from `platonic_solids.h` | CPU computes with morph interpolation |
| `0.5*iTime` camera orbit | CPU-accumulated `orbitAccum += orbitSpeed * deltaTime` | Passed as uniform |
| `vec3(0,0,4.0+sin(iTime*0.1))` color | Gradient LUT sampling by `edgeT[i]` | Standard generator coloring |
| `25.0` camera distance | `cameraDistance` uniform | Configurable |
| `5.0` camera height | `cameraHeight` uniform | Configurable |
| `15.0` shape scale | `scale` uniform | |
| `0.02` capsule radius | `edgeThickness` uniform | |
| `0.2 *` glow multiplier | `glowIntensity` uniform | |
| `pow(col, vec3(0.4545))` gamma | Keep verbatim | |
| `tanh(col)` tonemap | Keep verbatim | |
| `PIXEL 3.0/min(R.x,R.y)` | Keep verbatim | |
| `1.8*ww` FOV factor | `fov` uniform | Controls field of view |

### CPU Morph State Machine

Four-phase cycle, total duration = `1.0 / morphSpeed` seconds:

1. **EXPANDING** (`slideT` 0→1): Each edge grows from a point at one endpoint to full length. Capsule A endpoint lerps from B toward A. Duration: `(1 - holdRatio) / 2` of cycle.
2. **HOLDING**: Full wireframe visible. Duration: `holdRatio` of cycle.
3. **COLLAPSING** (`slideT` 1→0): Each edge shrinks from the opposite end — trailing endpoint catches up to leading. Creates directional "slide through" feel matching the reference animation. Duration: same as expanding.
4. **SWITCHING**: Instantaneous. Pick next shape, update vertex/edge data. All capsules are zero-length (invisible) so the switch is seamless.

The directional slide (expand from end A, collapse toward end B) is what the reference does:
- Expand: `capsule(A + slideT*(B-A), B)` — A endpoint slides toward B, edge grows
- Collapse: `capsule(A, A + slideT*(B-A))` — B endpoint slides toward A, edge shrinks from B-side

When slideT reaches 0 at collapse end, all edges are points at their A-vertices. Switch to next shape, reassign A/B from new edge data, expand again.

### Shape Selection

`randomness` parameter (0.0–1.0) controls next-shape selection:
- Roll a random float [0,1]. If it exceeds `randomness`, pick `baseShape`. Otherwise pick uniformly from all 5 solids (including current — can repeat).
- At `randomness=0`: always base shape (pure breathing like the reference).
- At `randomness=1`: uniform random each cycle.

### Vertex Count Mismatch

Different shapes have different edge counts (6 to 30). On shape switch:
- Set `edgeCount` to the new shape's edge count.
- Edges beyond the new count are zero-length (shader skips them naturally since capsule distance is huge).
- No vertex-slot mapping needed — each shape uses its own edge list directly from `platonic_solids.h`.

### Freeform Mode

`freeform` slider (0.0–1.0) perturbs vertex positions on the unit sphere:
- At 0: exact platonic vertex positions from `platonic_solids.h`.
- At 1: each vertex offset by a random direction, re-projected to the unit sphere.
- Random offsets are re-seeded each morph cycle (new perturbation each shape change).
- Edge connectivity stays the same as the underlying platonic solid — only positions change.

### Shader Core

```glsl
// sdCapsule — verbatim from reference
float sdCapsule(vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h) - r;
}

// Evaluate minimum distance to all edge capsules
float sdfEdges(vec3 p) {
    float d = 1e10;
    for (int i = 0; i < edgeCount; i++) {
        d = min(d, sdCapsule(p, edgeA[i], edgeB[i], edgeThickness));
    }
    return d;
}

// Ray march with glow accumulation — pattern from reference
float t = 0.0;
float glow = 0.0;
for (int i = 0; i < marchSteps; i++) {
    vec3 pos = ro + t * rd;
    float h = sdfEdges(pos);
    glow += PIXEL / (0.001 + h * h);
    t += h;
    if (t > 100.0) break;
}
```

### Camera

Orbit camera matching the reference: circular path in XZ plane at configurable height.
- `ro = vec3(cameraDistance * sin(orbitAccum), cameraHeight, cameraDistance * cos(orbitAccum))`
- Look-at target: origin
- `orbitAccum` accumulated on CPU: `orbitAccum += orbitSpeed * deltaTime`

### Coloring

Per-edge gradient position `edgeT[i]` mapped to gradient LUT, modulated by FFT brightness. Matches the SpinCage coloring approach — standard frequency-spread FFT with base freq, max freq, gain, curve, base bright.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseShape | int | 0-4 | 4 (Icosa) | Starting/base platonic solid (Tetra/Cube/Octa/Dodeca/Icosa) |
| randomness | float | 0.0-1.0 | 0.0 | Shape selection chaos (0=always base, 1=uniform random) |
| freeform | float | 0.0-1.0 | 0.0 | Vertex perturbation (0=exact platonic, 1=random positions) |
| morphSpeed | float | 0.1-5.0 | 0.5 | Morph cycles per second |
| holdRatio | float | 0.0-1.0 | 0.4 | Fraction of cycle showing full wireframe |
| orbitSpeed | float | -PI..+PI | 0.5 | Camera orbit rate rad/s (CPU-accumulated) |
| cameraHeight | float | 0.0-15.0 | 5.0 | Camera Y offset |
| cameraDistance | float | 5.0-50.0 | 25.0 | Camera orbit radius |
| scale | float | 1.0-30.0 | 15.0 | Shape size |
| edgeThickness | float | 0.005-0.1 | 0.02 | Capsule radius for wireframe edges |
| glowIntensity | float | 0.05-1.0 | 0.2 | Glow brightness multiplier |
| fov | float | 1.0-4.0 | 1.8 | Field of view (ray direction spread) |
| baseFreq | float | 27.5-440.0 | 55.0 | Low FFT frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | High FFT frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Baseline edge brightness |

## Modulation Candidates

- **morphSpeed**: Faster morph cycles during energetic sections, slow breathing in ambient passages
- **randomness**: Increase shape chaos during intense moments, lock to one shape in quiet sections
- **freeform**: Push toward abstract during peaks, snap to clean geometry in quiet moments
- **scale**: Pulsing size with bass energy
- **edgeThickness**: Thicker edges on louder notes
- **glowIntensity**: Brightness tracks overall energy
- **orbitSpeed**: Camera movement follows dynamics
- **cameraDistance**: Zoom in/out with amplitude

### Interaction Patterns

**morphSpeed x randomness (cascading threshold)**: At low randomness, morphSpeed just controls breathing rate on one shape. As randomness increases past ~0.3, each breath cycle becomes a potential shape change — verse sections pulse steadily on one solid while chorus sections trigger rapid chaotic morphing.

**freeform x scale (competing forces)**: Freeform pushes geometry toward abstraction while scale determines how much screen space it occupies. Large + freeform = overwhelming chaos. Small + freeform = contained weirdness. The balance between visual weight and geometric chaos creates tension.

## Notes

- The reference's symmetry-folding approach evaluates only 3 capsules per march step (efficient) but is locked to icosahedral symmetry. The explicit-capsule approach evaluates up to 30 capsules per step but supports arbitrary shapes and morphing. If performance is an issue, reduce march steps (80→60) or cap edge count.
- The shape switch happens at the moment all edges are zero-length (invisible), so no crossfade or vertex mapping between different-count shapes is needed. The transition is seamless.
- Random number generation for shape selection and freeform perturbation is CPU-side (simple PRNG seeded per cycle).
- `platonic_solids.h` already provides all 5 shapes with vertices on the unit sphere and edge connectivity — the shape data is ready to use.
