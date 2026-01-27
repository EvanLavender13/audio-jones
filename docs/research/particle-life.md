# Particle Life

Emergent multi-species particle simulation where simple attraction/repulsion rules between particle types create complex self-organizing behaviors: clusters, chasers, orbiters, and territorial competition. Each species interacts differently with every other species via an NxN attraction matrix, producing endless variety from a small set of parameters.

## Classification

- **Type**: Simulation (Per-agent with trails)
- **Compute Model**: Compute shader with spatial hashing for O(N×k) neighbor queries
- **Category**: SIMULATIONS (alongside Physarum, Boids, Curl Flow)

## References

- [Particle Life WebGPU Implementation](https://lisyarus.github.io/blog/posts/particle-life-simulation-in-browser-using-webgpu.html) - GPU architecture, spatial hashing, force model
- [Original Particle Life (hunar4321)](https://github.com/hunar4321/particle-life) - Core algorithm, JavaScript reference
- [Clusters by Jeffrey Ventrella](https://www.ventrella.com/Clusters/) - Original inspiration for the concept
- [Shadertoy 3D Implementation](https://www.shadertoy.com/view/youtu.be/scvuli-zcRc) - User-provided reference (code below)

## Algorithm

### Agent State

Each particle stores 3D position, 3D velocity, and species (following Attractor Flow's 3D pattern):

```glsl
struct Particle {
    float x, y, z;       // 3D position
    float vx, vy, vz;    // 3D velocity
    float hue;           // For colored deposits (derived from species)
    int species;         // 0 to numSpecies-1
};  // 32 bytes, GPU-aligned
```

### Force Model

Two force components act on each particle pair within interaction radius:

**1. Inner repulsion zone (r < beta × R_MAX):**
Always repels to prevent particle overlap.
```glsl
force = (r / (beta * R_MAX)) - 1.0;  // Returns -1 at r=0, approaches 0 at r=beta
```

**2. Interaction zone (beta × R_MAX < r < R_MAX):**
Attraction or repulsion based on the species-pair matrix value `a ∈ [-1, 1]`:
```glsl
force = a * (1.0 - abs(2.0 * r - 1.0 - beta) / (1.0 - beta));
```

**3. Beyond R_MAX:**
No force.

The complete force function:
```glsl
float force(float r, float a, float beta) {
    if (r < beta) {
        return r / beta - 1.0;
    } else if (r < 1.0) {
        return a * (1.0 - abs(2.0 * r - 1.0 - beta) / (1.0 - beta));
    }
    return 0.0;
}
```

### Attraction Matrix

An NxN matrix defines how each species pair interacts. Values range from -1 (strong repulsion) to +1 (strong attraction).

For species count N=4:
```
        To: 0    1    2    3
From 0:   -0.32 -0.17  0.34  0.10
From 1:   -0.34 -0.10  0.20 -0.15
From 2:    0.15 -0.20  0.15  0.25
From 3:    0.10  0.30 -0.10 -0.20
```

**Key insight:** The matrix is asymmetric—species A can attract B while B repels A. This violates Newton's third law, injecting energy into the system and enabling predator-prey dynamics.

### Main Update Loop

```glsl
uniform mat3 rotationMatrix;  // Precomputed on CPU for camera angle
uniform vec2 resolution;
uniform vec2 center;          // Screen center (0.5, 0.5 normalized)
uniform float projectionScale;

void main() {
    uint id = gl_GlobalInvocationID.x;
    Particle p = particles[id];
    vec3 pos = vec3(p.x, p.y, p.z);
    vec3 vel = vec3(p.vx, p.vy, p.vz);

    vec3 totalForce = vec3(0.0);

    // Query spatial hash for neighbors (27 cells in 3D)
    for each neighbor j in nearby cells {
        if (id == j) continue;

        Particle other = particles[j];
        vec3 otherPos = vec3(other.x, other.y, other.z);
        vec3 delta = otherPos - pos;
        float r = length(delta);

        if (r > 0.0 && r < rMax) {
            float a = attractionMatrix[p.species * numSpecies + other.species];
            float f = force(r / rMax, a, beta);
            totalForce += (delta / r) * f;
        }
    }

    // Centering force toward origin (keeps particles from drifting)
    // From Shadertoy: totalForce -= pos * exp(length(pos) * 2.0) * 10.0
    totalForce -= pos * centeringStrength * exp(length(pos) * centeringFalloff);

    totalForce *= rMax * forceFactor;

    // Semi-implicit Euler integration
    vel *= friction;
    vel += totalForce * timeStep;
    pos += vel * timeStep;

    // Project 3D -> 2D for trail deposit (like Attractor Flow)
    vec3 rotated = rotationMatrix * pos;
    vec2 screenPos = vec2(rotated.x, rotated.z) * projectionScale * min(resolution.x, resolution.y);
    screenPos += center * resolution;

    // Deposit to trail map if on screen
    if (screenPos.x >= 0.0 && screenPos.x < resolution.x &&
        screenPos.y >= 0.0 && screenPos.y < resolution.y) {
        ivec2 coord = ivec2(screenPos);
        vec3 color = hsv2rgb(vec3(p.hue, saturation, value));
        vec4 current = imageLoad(trailMap, coord);
        imageStore(trailMap, coord, vec4(current.rgb + color * depositAmount, 0.0));
    }

    // Store updated state
    p.x = pos.x; p.y = pos.y; p.z = pos.z;
    p.vx = vel.x; p.vy = vel.y; p.vz = vel.z;
    particles[id] = p;
}
```

### 3D Projection (Like Attractor Flow)

Project 3D positions to 2D screen using orthographic projection:

```glsl
vec2 projectToScreen(vec3 pos) {
    vec3 rotated = rotationMatrix * pos;      // Apply camera rotation
    vec2 projected = vec2(rotated.x, rotated.z);  // Drop Y axis (depth)
    return projected * projectionScale * min(resolution.x, resolution.y) + center * resolution;
}
```

The rotation matrix is precomputed on CPU from rotation angles (like Attractor Flow). Can be static or slowly auto-rotating.

### Neighbor Queries (Brute Force)

Initial implementation uses O(N²) brute force—each particle checks all others:

```glsl
for (uint j = 0; j < numParticles; j++) {
    if (id == j) continue;
    // ... distance check and force calculation
}
```

For 10K particles this is 100M distance checks/frame. GPU handles this via parallelism—each of 10K threads does 10K iterations.

**Future optimization:** If performance is insufficient, implement 3D spatial hashing (27-cell neighbor query). Only pursue if the effect proves worthwhile.

### Attraction Matrix Generation

Random generation with seed:
```glsl
float attraction(int from, int to, int seed) {
    float v = hash(vec2(from, to) + float(seed * numSpecies));
    return v * 2.0 - 1.0;  // Map [0,1] to [-1,1]
}
```

## Parameters

### Simulation Parameters

| Parameter | Type | Range | Default | Effect on Behavior |
|-----------|------|-------|---------|-------------------|
| agentCount | int | 1000-20000 | 10000 | Particle count (O(N²) limits scaling) |
| speciesCount | int | 2-8 | 4 | Number of distinct types |
| rMax | float | 0.05-0.5 | 0.15 | Interaction radius (3D units) |
| forceFactor | float | 1-50 | 10.0 | Force strength multiplier |
| friction | float | 0.1-0.99 | 0.6 | Velocity damping per frame |
| beta | float | 0.1-0.5 | 0.3 | Inner repulsion zone ratio |
| attractionSeed | int | 0-999 | 0 | Random seed for matrix generation |
| centeringStrength | float | 0-20 | 10.0 | Pull toward origin |
| centeringFalloff | float | 0-5 | 2.0 | Exponential falloff rate |

### 3D View Parameters (Like Attractor Flow)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| rotationAngleX | float | 0-2π | 0 | Camera pitch |
| rotationAngleY | float | 0-2π | 0 | Camera yaw |
| rotationAngleZ | float | 0-2π | 0 | Camera roll |
| rotationSpeedX/Y/Z | float | -1 to 1 | 0 | Auto-rotation rates (rad/s) |
| projectionScale | float | 0.1-2.0 | 0.5 | Zoom level |
| center | vec2 | 0-1 | (0.5, 0.5) | Screen position |

### Trail Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| depositAmount | float | 0.01-0.5 | 0.05 | Trail brightness per deposit |
| decayHalfLife | float | 0.1-5.0 | 0.5 | Trail fade time |
| diffusionScale | int | 0-4 | 1 | Trail blur radius |

## Modulation Candidates

- **forceFactor**: Pumps energy into system on beats
- **friction**: Lower friction = more chaotic motion on loud sections
- **rMax**: Expand/contract interaction range with bass
- **attractionSeed**: Auto-randomize matrix on time intervals (like Shadertoy)

## ColorConfig Integration

Use rainbow mode to distribute hues across species:
```glsl
float hue = rainbowHue + (float(species) / float(speciesCount)) * rainbowRange;
vec3 rgb = hsv2rgb(vec3(hue, saturation, value));
```

- `rainbowHue`: Starting hue offset
- `rainbowRange`: Hue span across species
- `rainbowSat`, `rainbowVal`: Apply uniformly

Gradient mode could map species to gradient stops for more control.

## UI Design

**Recommended layout (mirrors Attractor Flow pattern):**
```
[Particle Life]
├─ Enabled: [x]
├─ Agent Count: [====|====] 10000
├─ Species Count: [2|3|4|5|6|7|8]
├─ [Randomize Rules] Seed: 42
├─ Physics ▼
│   ├─ Interaction Radius: [===|=====] 0.15
│   ├─ Force Factor: [=====|===] 10.0
│   ├─ Friction: [======|==] 0.6
│   ├─ Repulsion Zone: [===|=====] 0.3
│   └─ Centering: [=====|===] 10.0
├─ 3D View ▼
│   ├─ Rotation X/Y/Z: [drag handles or sliders]
│   ├─ Auto-Rotate Speed: [===|=====]
│   └─ Scale: [====|====] 0.5
├─ Color ▼
│   ├─ Mode: Rainbow
│   ├─ Hue Range: [========] 360°
│   └─ Sat/Val: [====] [====]
└─ Trail ▼
    ├─ Deposit: [===|=====]
    ├─ Decay: [=====|===]
    └─ Diffusion: [==|======]
```

**Future enhancement:** Matrix editor grid for advanced users, preset "recipes" (symmetric clusters, predator-prey, etc.).

## User-Provided Shadertoy Reference

```glsl
// Core force function from user's reference
float force(float r, float a) {
    float beta = 0.3;
    if (r < beta) {
        return r / beta - 1.;
    } else if (beta < r && r < 1.) {
        return a * (1. - abs(2. * r - 1. - beta) / (1. - beta));
    } else {
        return 0.;
    }
}

// Attraction matrix lookup
float attraction(int color1, int color2, int seed) {
    float v = rand(vec2(color1, color2) + float(NUM_COLORS * seed));
    return v * 2.0 - 1.0;  // map to [-1.0, 1.0]
}
```

## Pipeline Compatibility

| Requirement | Status | Notes |
|-------------|--------|-------|
| Per-agent compute shader | Compatible | Same pattern as Attractor Flow |
| 3D positions + projection | Compatible | Follows Attractor Flow exactly |
| Rotation matrix uniform | Compatible | CPU-computed, passed to shader |
| Trail map deposits | Compatible | Shared infrastructure |
| SSBO agent buffers | Compatible | Standard pattern |
| Neighbor queries | Brute force | O(N²) for initial 10K implementation |
| ColorConfig | Compatible | Rainbow mode maps species to hues |

## Performance Considerations

- **Initial approach**: Brute force O(N²)—simple, no spatial data structures
- **10K particles**: 100M distance checks/frame. Each of 10K GPU threads does 10K iterations.
- **Memory bandwidth**: Real bottleneck is reading particle positions from global memory, not compute
- **Target**: 10K particles at 60fps
- **Future optimization**: 3D spatial hashing (27-cell query) if needed. Only pursue if effect proves worthwhile.
- **Won't match Attractor Flow scale**: Attractor is O(N), runs 100K+. Particle Life caps ~10-20K without optimization.

## Notes

- Unlike Boids (3 fixed rules), Particle Life's rules are fully configurable via the attraction matrix
- The asymmetric matrix (A→B ≠ B→A) is essential—it breaks conservation laws and enables perpetual motion
- "Recipes" for known interesting patterns could be presets (symmetric clusters, predator-prey chains, etc.)
- Auto-randomizing the matrix on intervals creates ever-changing dynamics (see `ATTRACTION_CHANGE_INTERVAL`)
