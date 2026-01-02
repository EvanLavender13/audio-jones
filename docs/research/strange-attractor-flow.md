# Strange Attractor Flow - Implementation Research

Particles trace trajectories through chaotic dynamical systems. Lorenz, Rössler, and other strange attractors produce perpetual, non-repeating motion by mathematical necessity.

## Mathematical Foundation

### Chaotic Dynamics

Strange attractors exhibit three key properties:

1. **Sensitive dependence on initial conditions** - Nearby trajectories diverge exponentially (Lyapunov exponent > 0)
2. **Bounded phase space** - All trajectories remain within finite volume
3. **Non-periodic orbits** - Motion never exactly repeats

These properties guarantee continuous visual flow without settling into static patterns.

### Core Attractor Systems

**Lorenz System** (atmospheric convection model):
```
dx/dt = σ(y - x)
dy/dt = x(ρ - z) - y
dz/dt = xy - βz

Classic parameters: σ=10, ρ=28, β=8/3
```

**Rössler System** (simpler chaos generator):
```
dx/dt = -y - z
dy/dt = x + ay
dz/dt = b + z(x - c)

Classic parameters: a=0.2, b=0.2, c=5.7
```

**Aizawa System** (3-torus topology):
```
dx/dt = (z - β)x - δy
dy/dt = δx + (z - β)y
dz/dt = γ + αz - z³/3 - (x² + y²)(1 + εz) + fz·x³

Parameters: α=0.95, β=0.7, γ=0.6, δ=3.5, ε=0.25, f=0.1
```

**Thomas System** (biological modeling):
```
dx/dt = sin(y) - bx
dy/dt = sin(z) - by
dz/dt = sin(x) - bz

Classic parameter: b=0.19
```

---

## Implementation Architecture

### Agent Structure

Matches existing 32-byte alignment for GPU coherence:

```cpp
typedef struct AttractorAgent {
    float x, y, z;        // 3D position in attractor space
    float hue;            // Color identity (0-1)
    float age;            // Lifetime for respawn logic
    float _pad[3];        // 32-byte alignment
} AttractorAgent;
```

Agent count: 100K-500K typical (attractor geometry visible at lower counts than physarum/curl).

### Coordinate Systems

Attractor coordinates differ from screen space:
- **Lorenz**: x ∈ [-25, 25], y ∈ [-30, 30], z ∈ [0, 50]
- **Rössler**: x ∈ [-15, 15], y ∈ [-15, 15], z ∈ [0, 30]

Projection to screen requires scaling and centering:

```glsl
vec2 projectToScreen(vec3 pos) {
    // Orthographic: discard z, scale to resolution
    float scale = min(resolution.x, resolution.y) / attractorScale;
    vec2 centered = pos.xy * scale + resolution * 0.5;
    return centered;
}

// Alternative: perspective projection for depth cues
vec2 projectPerspective(vec3 pos, float focalLength) {
    float depth = pos.z - cameraZ;
    float perspScale = focalLength / max(depth, 0.1);
    return pos.xy * perspScale + resolution * 0.5;
}
```

### Integration Methods

**Euler** (fast, less accurate):
```glsl
vec3 eulerStep(vec3 pos, float dt) {
    vec3 dv = lorenzDerivative(pos);
    return pos + dv * dt;
}
```

**RK4** (4th-order Runge-Kutta, stable for chaotic systems):
```glsl
vec3 rk4Step(vec3 pos, float dt) {
    vec3 k1 = lorenzDerivative(pos);
    vec3 k2 = lorenzDerivative(pos + k1 * dt * 0.5);
    vec3 k3 = lorenzDerivative(pos + k2 * dt * 0.5);
    vec3 k4 = lorenzDerivative(pos + k3 * dt);
    return pos + (k1 + 2.0*k2 + 2.0*k3 + k4) * dt / 6.0;
}
```

RK4 costs 4× derivative evaluations but prevents trajectory drift over long runs.

---

## Compute Shader Design

### Agent Update Pass

```glsl
#version 430

layout(local_size_x = 1024) in;

struct AttractorAgent {
    float x, y, z;
    float hue;
    float age;
    float _pad[3];
};

layout(std430, binding = 0) buffer AgentBuffer {
    AttractorAgent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;
layout(binding = 2) uniform sampler2D accumTexture;

uniform vec2 resolution;
uniform float time;
uniform float deltaTime;

// Attractor parameters
uniform float sigma;      // Lorenz: 10.0
uniform float rho;        // Lorenz: 28.0
uniform float beta;       // Lorenz: 2.667

uniform float timeScale;  // Integration step multiplier
uniform float depositAmount;
uniform float saturation;
uniform float value;
uniform float accumSenseBlend;
uniform float attractorScale;
uniform float respawnRate;

// Pseudo-random generator
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

float random(uint seed) {
    return float(hash(seed)) / 4294967295.0;
}

vec3 lorenzDerivative(vec3 p) {
    return vec3(
        sigma * (p.y - p.x),
        p.x * (rho - p.z) - p.y,
        p.x * p.y - beta * p.z
    );
}

vec3 rk4Step(vec3 pos, float dt) {
    vec3 k1 = lorenzDerivative(pos);
    vec3 k2 = lorenzDerivative(pos + k1 * dt * 0.5);
    vec3 k3 = lorenzDerivative(pos + k2 * dt * 0.5);
    vec3 k4 = lorenzDerivative(pos + k3 * dt);
    return pos + (k1 + 2.0*k2 + 2.0*k3 + k4) * dt / 6.0;
}

vec2 projectToScreen(vec3 pos) {
    float scale = min(resolution.x, resolution.y) / attractorScale;
    return pos.xy * scale + resolution * 0.5;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= agents.length()) return;

    AttractorAgent agent = agents[idx];
    vec3 pos = vec3(agent.x, agent.y, agent.z);

    // Respawn check: bounds escape or random culling
    float maxCoord = max(abs(pos.x), max(abs(pos.y), abs(pos.z)));
    uint seed = idx + uint(time * 1000.0);
    bool shouldRespawn = maxCoord > 100.0 ||
                         (respawnRate > 0.0 && random(seed) < respawnRate * deltaTime);

    if (shouldRespawn) {
        // Respawn near attractor center with small perturbation
        pos = vec3(
            (random(seed + 1u) - 0.5) * 1.0,
            (random(seed + 2u) - 0.5) * 1.0 + 1.0,
            (random(seed + 3u) - 0.5) * 1.0 + 25.0
        );
        agent.hue = random(seed + 4u);
        agent.age = 0.0;
    }

    // Integrate trajectory
    float dt = timeScale * deltaTime;
    pos = rk4Step(pos, dt);
    agent.age += deltaTime;

    // Project to screen and deposit trail
    vec2 screenPos = projectToScreen(pos);
    ivec2 coord = ivec2(screenPos);

    if (coord.x >= 0 && coord.x < int(resolution.x) &&
        coord.y >= 0 && coord.y < int(resolution.y)) {

        vec3 depositColor = hsv2rgb(vec3(agent.hue, saturation, value));
        vec4 current = imageLoad(trailMap, coord);
        vec3 newColor = current.rgb + depositColor * depositAmount;

        // Proportional scaling preserves saturation
        float maxChan = max(newColor.r, max(newColor.g, newColor.b));
        if (maxChan > 1.0) newColor /= maxChan;

        imageStore(trailMap, coord, vec4(newColor, 1.0));
    }

    // Write back
    agent.x = pos.x;
    agent.y = pos.y;
    agent.z = pos.z;
    agents[idx] = agent;
}
```

---

## Drawable Injection Scenarios

### Scenario 1: Attractor Perturbation Field

Drawable geometry creates force perturbations that nudge particle trajectories.

**Implementation:**
1. Render drawables to perturbation texture (same as trail map injection)
2. Sample perturbation at projected screen position
3. Add perturbation as external force term to differential equations

```glsl
// Modified derivative with drawable perturbation
vec3 perturbedDerivative(vec3 pos, vec2 screenPos) {
    vec3 base = lorenzDerivative(pos);

    // Sample drawable intensity at projected position
    vec4 drawable = texture(drawableMap, screenPos / resolution);
    float perturbStrength = drawable.a * perturbationScale;

    // Perturbation direction: toward/away from drawable center
    vec2 perturbDir = normalize(drawable.rg - 0.5);  // Encode direction in RG

    // Apply perturbation in XY plane (screen-aligned)
    base.xy += perturbDir * perturbStrength;

    return base;
}
```

**Visual Effect:** Waveforms push particles aside, creating voids that fill back in. Spectrum bars act as barriers or attractors depending on perturbation sign.

### Scenario 2: Hue Injection from Drawables

Drawable color overrides particle hue when particles pass through drawable regions.

**Implementation:**
1. Render drawables at full opacity to trail map (existing pipeline)
2. When depositing, blend agent hue with drawable hue at screen position

```glsl
vec3 getDepositColor(float agentHue, ivec2 coord) {
    vec4 drawable = imageLoad(trailMap, coord);

    // If drawable present (high alpha), blend toward drawable hue
    float drawableHue = rgb2hue(drawable.rgb);
    float blendFactor = drawable.a * drawableInfluence;

    float finalHue = mix(agentHue, drawableHue, blendFactor);
    return hsv2rgb(vec3(finalHue, saturation, value));
}
```

**Visual Effect:** Particles adopt waveform colors as they pass through, creating color gradients that follow attractor geometry.

### Scenario 3: Respawn Attractors

Drawable positions become respawn locations for particles.

**Implementation:**
1. Build list of drawable centroids (CPU-side or compute pass)
2. When respawning, probabilistically spawn at drawable positions instead of random

```glsl
// Respawn with drawable attraction
if (shouldRespawn) {
    float r = random(seed);
    if (r < drawableRespawnBias && drawableCount > 0) {
        // Sample random drawable position
        int drawableIdx = int(random(seed + 100u) * float(drawableCount));
        vec2 drawablePos = drawablePositions[drawableIdx];

        // Convert screen position back to attractor space
        pos = screenToAttractor(drawablePos);
    } else {
        // Standard attractor-space respawn
        pos = randomNearCenter(seed);
    }
}
```

**Visual Effect:** Particle streams originate from waveform positions, creating flow lines that connect drawable elements.

### Scenario 4: Depth-Based Drawable Interaction

Use particle Z coordinate to determine drawable visibility/interaction layer.

**Implementation:**
```glsl
// Layer particles by Z depth
float particleLayer = (pos.z - zMin) / (zMax - zMin);  // 0-1

// Drawables rendered to multiple depth slices
vec4 drawable = texture(drawableLayers, vec3(screenPos / resolution, particleLayer));

// Only interact when in same depth layer
if (drawable.a > 0.0) {
    // Apply interaction...
}
```

**Visual Effect:** Creates depth parallax where drawables at different virtual depths interact with different attractor "slices."

---

## Feedback Texture Interactions

### Interaction 1: Feedback-Modulated Time Scale

Dense feedback regions slow particle integration (similar to curl flow trail influence).

```glsl
float sampleFeedbackDensity(vec2 screenPos) {
    vec2 uv = screenPos / resolution;
    vec4 feedback = texture(accumTexture, uv);
    return dot(feedback.rgb, vec3(0.299, 0.587, 0.114));  // Luminance
}

void main() {
    // ... position update ...

    vec2 screenPos = projectToScreen(pos);
    float density = sampleFeedbackDensity(screenPos);

    // Slow integration in dense regions
    float speedMod = 1.0 - feedbackInfluence * clamp(density, 0.0, 1.0);
    float dt = timeScale * deltaTime * max(speedMod, 0.1);

    pos = rk4Step(pos, dt);
}
```

**Visual Effect:** Particles linger where previous frames accumulated brightness, reinforcing established structures.

### Interaction 2: Attractor Parameter Modulation

Feedback density locally modifies attractor parameters.

```glsl
// Base parameters
float localRho = rho;
float localSigma = sigma;

// Modulate by local feedback
float density = sampleFeedbackDensity(screenPos);
localRho += density * rhoFeedbackMod;     // Chaotic threshold
localSigma += density * sigmaFeedbackMod; // Convection rate

vec3 derivative = vec3(
    localSigma * (p.y - p.x),
    p.x * (localRho - p.z) - p.y,
    p.x * p.y - beta * p.z
);
```

**Visual Effect:** Different attractor behavior in bright vs dark regions. Dense areas become more chaotic (higher rho) or more laminar (lower sigma).

### Interaction 3: Hue Inheritance from Feedback

Particles absorb color from feedback texture over time.

```glsl
// Gradual hue drift toward feedback color
vec4 feedback = texture(accumTexture, screenPos / resolution);
float feedbackHue = rgb2hue(feedback.rgb);

// Exponential moving average
float inheritRate = hueInheritance * deltaTime;
agent.hue = mix(agent.hue, feedbackHue, inheritRate * feedback.a);
```

**Visual Effect:** Particles adopt colors from regions they traverse, creating color continuity between attractor and background.

### Interaction 4: Feedback-Driven Respawn Distribution

High feedback density regions become more likely respawn locations.

```glsl
// Build respawn probability from feedback luminance
// (Requires precomputed CDF or rejection sampling)

vec2 feedbackWeightedRespawn(uint seed) {
    // Rejection sampling approach
    for (int i = 0; i < 16; i++) {
        vec2 candidate = vec2(
            random(seed + uint(i) * 2u),
            random(seed + uint(i) * 2u + 1u)
        ) * resolution;

        float density = sampleFeedbackDensity(candidate);
        if (random(seed + uint(i) * 100u) < density) {
            return candidate;
        }
    }
    return resolution * 0.5;  // Fallback to center
}
```

**Visual Effect:** Particles respawn preferentially in bright areas, concentrating flow where visual activity exists.

---

## Audio Reactivity Mapping

| Audio Band | Parameter | Effect |
|------------|-----------|--------|
| Bass (20-150Hz) | rho | Shape morphing (rho changes attractor topology dramatically) |
| Low-mids (150-500Hz) | timeScale | Flow speed modulation |
| Mids (500-2kHz) | sigma | Convection rate (tighter vs looser spirals) |
| Highs (2-8kHz) | depositAmount | Trail brightness |
| Beat detection | respawnRate | Burst of fresh particles |
| Spectral centroid | attractorType | Switch between Lorenz/Rössler/Thomas |

### Beat-Synchronized Respawn Burst

```cpp
// In audio callback
if (beatDetected) {
    attractorConfig.respawnRate = 1.0f;  // 100% respawn this frame
} else {
    attractorConfig.respawnRate = 0.01f; // Normal trickle
}
```

Creates rhythmic particle bursts that follow music structure.

### Spectral Attractor Selection

```cpp
// Select attractor type based on spectral characteristics
float spectralCentroid = computeSpectralCentroid(fftData);

if (spectralCentroid < 500.0f) {
    attractorConfig.type = ATTRACTOR_LORENZ;      // Bass-heavy: classic butterfly
} else if (spectralCentroid < 2000.0f) {
    attractorConfig.type = ATTRACTOR_ROSSLER;     // Mid-range: band structure
} else {
    attractorConfig.type = ATTRACTOR_THOMAS;      // Highs: smooth curves
}
```

---

## Configuration Structure

```cpp
typedef struct AttractorFlowConfig {
    bool enabled;
    int agentCount;              // 50000-500000

    // Attractor selection
    AttractorType type;          // LORENZ, ROSSLER, AIZAWA, THOMAS

    // Lorenz parameters (most common)
    float sigma;                 // 5.0-15.0, default 10.0
    float rho;                   // 20.0-35.0, default 28.0
    float beta;                  // 1.0-4.0, default 2.667

    // Integration
    float timeScale;             // 0.001-0.05, default 0.01
    bool useRK4;                 // true for accuracy, false for speed

    // Visual
    float attractorScale;        // 50.0-200.0 (projection scaling)
    float depositAmount;         // 0.01-0.2
    float decayHalfLife;         // 0.1-5.0
    int diffusionScale;          // 0-4

    // Respawn
    float respawnRate;           // 0.0-1.0 (fraction per second)
    float boundaryRadius;        // Auto-respawn beyond this

    // Feedback
    float feedbackInfluence;     // 0.0-1.0 (speed modulation)
    float accumSenseBlend;       // 0.0=trail, 1.0=accum

    // Compositing
    float boostIntensity;
    EffectBlendMode blendMode;
    ColorConfig color;
} AttractorFlowConfig;
```

---

## Implementation Checklist

1. **Core files to create:**
   - `src/simulation/attractor_flow.h` - Config struct, API declarations
   - `src/simulation/attractor_flow.cpp` - Init, config, dispatch
   - `shaders/attractor_agents.glsl` - Agent compute shader

2. **Reuse from existing infrastructure:**
   - `TrailMap` for trail storage and processing
   - `BlendCompositor` for trail compositing
   - `ColorConfig` for hue assignment modes
   - Pipeline integration pattern from `ApplyPhysarumPass`

3. **Integration points:**
   - `PostEffect` struct: add `AttractorFlow* attractorFlow`
   - `EffectsConfig`: add `AttractorFlowConfig attractorFlow`
   - `render_pipeline.cpp`: add `ApplyAttractorFlowPass`
   - `main.cpp`: add `RenderDrawablesToAttractor`
   - `imgui_effects.cpp`: add UI panel

4. **Testing milestones:**
   - [ ] Basic Lorenz attractor visible with static particles
   - [ ] Integration produces continuous motion
   - [ ] Trail deposit and diffusion working
   - [ ] Drawable injection modifies flow
   - [ ] Feedback interaction creates emergence
   - [ ] Audio reactivity responds to FFT

---

## References

### GPU Implementations
- [BrutPitt/glChAoS.P](https://github.com/BrutPitt/glChAoS.P) - 256M particles, 40+ attractor types, OpenGL 4.5
- [IM56/Lorenz-Particle-System](https://github.com/IM56/Lorenz-Particle-System) - DirectX 11/HLSL, 500K particles
- [fuse* Strange Attractors GPU](https://fusefactory.github.io/openfuse/strange%20attractors/particle%20system/Strange-Attractors-GPU/) - Fragment shader with curl noise

### Mathematical Background
- [Lorenz System (Wikipedia)](https://en.wikipedia.org/wiki/Lorenz_system)
- [Rössler Attractor (Wikipedia)](https://en.wikipedia.org/wiki/R%C3%B6ssler_attractor)
- [Chaotic Attractors Catalog](https://www.dynamicmath.xyz/strange-attractors/) - Interactive parameter exploration

### Integration Techniques
- [RK4 for Chaotic Systems](https://www.sciencedirect.com/topics/engineering/runge-kutta-method) - Stability analysis
- [Adaptive Step Size](https://en.wikipedia.org/wiki/Adaptive_step_size) - For maintaining accuracy near bifurcations
