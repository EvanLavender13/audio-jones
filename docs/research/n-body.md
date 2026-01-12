# N-Body Simulation

Gravitational n-body simulation where every particle attracts every other particle. Produces orbital dynamics, galaxy-like structures, and emergent clustering. Particles orbit, merge into clusters, and form spiral patterns resembling celestial bodies.

## Classification

- **Category**: SIMULATIONS (alongside Physarum, Curl Flow, Attractor Flow, Boids)
- **Core Operation**: Compute shader calculates gravitational acceleration between all particle pairs, integrates velocity/position, deposits trails to texture
- **Pipeline Position**: Particle compute → Flow Field shader → Blur

## References

- [GPU Gems 3: Fast N-Body Simulation with CUDA](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-31-fast-n-body-simulation-cuda) - Core algorithm, force equation, softening factor, CUDA implementation
- [Wikipedia: Barnes-Hut simulation](https://en.wikipedia.org/wiki/Barnes–Hut_simulation) - O(N log N) tree-based optimization, θ accuracy parameter
- [LanLou123/nBody](https://github.com/LanLou123/nBody) - OpenGL compute shader implementation with tiled parallelization

## Algorithm

### Gravitational Force Equation

The force between two bodies:

```
F_ij = G * m_i * m_j / |r_ij|³ * r_ij
```

Where:
- G = gravitational constant (tunable strength parameter)
- m_i, m_j = body masses
- r_ij = position vector from body i to body j

### Softening Factor

Prevents singularities when particles approach each other:

```
distSqr = |r_ij|² + ε²
```

Where ε (epsilon) is the softening length. Models "Plummer point masses" — gravity weakens at very close range.

### Agent State

```glsl
struct Body {
    vec2 position;
    vec2 velocity;
    float mass;
    float hue;        // Trail color
    float spectrumPos; // FFT lookup position
    float _pad;       // Alignment padding
};
```

### Force Calculation (from GPU Gems 3)

```glsl
vec2 bodyBodyInteraction(Body bi, Body bj, vec2 ai) {
    vec2 r = bj.position - bi.position;
    float distSqr = dot(r, r) + softening * softening;
    float distSixth = distSqr * distSqr * distSqr;
    float invDistCube = 1.0 / sqrt(distSixth);
    float s = bj.mass * invDistCube;
    ai += r * s;
    return ai;
}
```

### Main Update Loop

```glsl
void main() {
    Body b = bodies[id];

    // Accumulate gravitational acceleration from all other bodies
    vec2 acceleration = vec2(0.0);
    for (int i = 0; i < numBodies; i++) {
        if (i == id) continue;
        acceleration = bodyBodyInteraction(b, bodies[i], acceleration);
    }
    acceleration *= gravityStrength;

    // Leapfrog-Verlet integration
    b.velocity += acceleration * deltaTime;

    // Clamp velocity magnitude
    float speed = length(b.velocity);
    if (speed > maxSpeed) {
        b.velocity = (b.velocity / speed) * maxSpeed;
    }

    b.position += b.velocity * deltaTime;

    // Boundary handling (wrap or bounce)
    b.position = mod(b.position, resolution);

    // Deposit trail
    deposit(b);

    bodies[id] = b;
}
```

### Leapfrog-Verlet Integration

Chosen for stability in orbital dynamics. Updates velocity at half-steps relative to position, conserving energy better than Euler integration.

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| bodyCount | int | 500-20000 | Number of gravitating bodies |
| gravityStrength | float | 0.0001-0.01 | Gravitational constant G |
| softening | float | 1.0-50.0 | Prevents singularities, smooths close encounters |
| maxSpeed | float | 1.0-20.0 | Velocity clamp prevents runaway |
| deltaTime | float | 0.1-2.0 | Integration timestep |
| massMin | float | 0.1-1.0 | Minimum body mass |
| massMax | float | 1.0-10.0 | Maximum body mass |
| depositAmount | float | 0.01-0.5 | Trail brightness per particle |
| saturation | float | 0-1 | Deposit color saturation |
| value | float | 0-1 | Deposit color brightness |

## Initialization Patterns

Different starting configurations produce distinct orbital behaviors:

| Pattern | Description |
|---------|-------------|
| Uniform random | Chaotic collapse into clusters |
| Ring/disk | Spiral galaxy formation |
| Two clusters | Binary system merger |
| Central mass + orbiting | Solar system dynamics |
| Gaussian distribution | Elliptical galaxy |

## Audio Mapping Ideas

| Parameter | Audio Source | Effect |
|-----------|--------------|--------|
| gravityStrength | Bass energy | Stronger pull on bass hits, particles cluster |
| softening | High frequency | Smoother/choppier motion with treble |
| maxSpeed | Overall amplitude | Faster orbits during loud sections |
| massMax | Beat detection | Spawn heavier particles on beats |
| bodyCount | Sustained energy | Add particles during sustained sections |
| depositAmount | Mid-range energy | Brighter trails with vocals/melody |

## Advanced: Barnes-Hut Optimization

The brute-force O(N²) algorithm limits particle count. Barnes-Hut reduces to O(N log N):

1. Build quadtree (2D) or octree (3D) containing all bodies
2. For each body, traverse tree from root:
   - If node's center-of-mass is far enough (s/d < θ), treat as single mass
   - Otherwise recurse into children
3. θ parameter trades accuracy for speed (θ=0 is exact, θ=0.5-1.0 is common)

**Implementation complexity**: Tree construction and traversal on GPU requires careful memory management. Consider as future optimization if brute-force proves too slow.

## Notes

- Softening parameter ε prevents division by zero and numerical instability — critical for visual stability
- Unlike Boids, n-body has no perception radius — every particle influences every other (unless using Barnes-Hut)
- Mass variation creates natural hierarchy: heavy bodies become orbital centers
- Wrap-around boundaries create torus topology; consider reflective boundaries for more realistic orbits
- Existing spatial hash from Boids could accelerate close-range interactions, but long-range gravity still requires all-pairs or tree
