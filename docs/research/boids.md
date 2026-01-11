# Boids

Emergent flocking simulation using Craig Reynolds' 1986 algorithm. Agents follow three simple steering rules — separation, alignment, cohesion — producing organic swarm behavior. Extended with texture reactivity for audio visualization: boids attract to or avoid bright areas in the feedback texture.

## Classification

- **Category**: SIMULATIONS (alongside Physarum, Curl Flow, Attractor Flow)
- **Core Operation**: Agent compute shader applies steering forces, deposits points to trail map
- **Pipeline Position**: Particle compute → Flow Field shader → Blur

## References

- [Craig Reynolds' Boids Page](https://www.red3d.com/cwr/boids/) - Original concept, three steering behaviors
- [Boids Pseudocode](https://vergenet.net/~conrad/boids/pseudocode.html) - Complete algorithm with code
- [Wikipedia: Boids](https://en.wikipedia.org/wiki/Boids) - History and applications

## Algorithm

### Agent State

Each boid stores position and velocity (unlike Physarum which uses heading):

```glsl
struct Boid {
    vec2 position;
    vec2 velocity;
    float hue;        // For colored deposits
    float spectrumPos; // FFT lookup position
};
```

### Main Update Loop

```glsl
void main() {
    Boid b = boids[id];

    vec2 v1 = cohesion(b);
    vec2 v2 = separation(b);
    vec2 v3 = alignment(b);
    vec2 v4 = textureReaction(b);

    b.velocity += v1 * cohesionWeight
                + v2 * separationWeight
                + v3 * alignmentWeight
                + v4 * textureWeight;

    // Clamp velocity magnitude
    float speed = length(b.velocity);
    if (speed > maxSpeed) {
        b.velocity = (b.velocity / speed) * maxSpeed;
    }
    if (speed < minSpeed) {
        b.velocity = (b.velocity / speed) * minSpeed;
    }

    b.position += b.velocity;
    b.position = mod(b.position, resolution); // Wrap boundaries

    // Deposit to trail map
    deposit(b);

    boids[id] = b;
}
```

### Rule 1: Cohesion (Steer Toward Flock Center)

Compute perceived center of mass of neighbors, steer toward it:

```glsl
vec2 cohesion(Boid bJ) {
    vec2 center = vec2(0.0);
    int count = 0;

    for (int i = 0; i < numBoids; i++) {
        if (i == id) continue;
        Boid b = boids[i];
        float dist = distance(b.position, bJ.position);

        if (dist < perceptionRadius) {
            center += b.position;
            count++;
        }
    }

    if (count == 0) return vec2(0.0);

    center /= float(count);
    return (center - bJ.position) / 100.0; // Gradual steering
}
```

### Rule 2: Separation (Avoid Crowding)

Steer away from nearby neighbors to prevent collision:

```glsl
vec2 separation(Boid bJ) {
    vec2 avoid = vec2(0.0);

    for (int i = 0; i < numBoids; i++) {
        if (i == id) continue;
        Boid b = boids[i];
        vec2 diff = bJ.position - b.position;
        float dist = length(diff);

        if (dist < separationRadius && dist > 0.0) {
            avoid += diff / dist; // Stronger push when closer
        }
    }

    return avoid;
}
```

### Rule 3: Alignment (Match Flock Velocity)

Steer toward average heading of neighbors:

```glsl
vec2 alignment(Boid bJ) {
    vec2 avgVelocity = vec2(0.0);
    int count = 0;

    for (int i = 0; i < numBoids; i++) {
        if (i == id) continue;
        Boid b = boids[i];
        float dist = distance(b.position, bJ.position);

        if (dist < perceptionRadius) {
            avgVelocity += b.velocity;
            count++;
        }
    }

    if (count == 0) return vec2(0.0);

    avgVelocity /= float(count);
    return (avgVelocity - bJ.velocity) / 8.0;
}
```

### Rule 4: Texture Reaction (Extension)

Sample feedback texture, steer toward or away from bright areas:

```glsl
vec2 textureReaction(Boid bJ) {
    // Sample ahead in movement direction
    vec2 ahead = bJ.position + normalize(bJ.velocity) * sensorDistance;
    vec2 aheadUV = ahead / resolution;
    float luminance = dot(texture(feedbackTex, aheadUV).rgb, vec3(0.299, 0.587, 0.114));

    // attractMode: 1.0 = attract to bright, -1.0 = avoid bright
    vec2 towardBright = normalize(ahead - bJ.position);
    return towardBright * luminance * attractMode;
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| agentCount | int | 1000-50000 | Number of boids |
| perceptionRadius | float | 10-100 px | Neighbor detection range |
| separationRadius | float | 5-50 px | Crowding avoidance range |
| cohesionWeight | float | 0-2 | Strength of center-seeking |
| separationWeight | float | 0-2 | Strength of avoidance |
| alignmentWeight | float | 0-2 | Strength of velocity matching |
| textureWeight | float | 0-2 | Strength of texture reaction |
| attractMode | float | -1 to 1 | -1=avoid bright, 1=attract to bright |
| maxSpeed | float | 1-10 | Velocity clamp |
| minSpeed | float | 0-2 | Prevents stalling |
| depositAmount | float | 0.01-0.5 | Trail brightness |
| saturation | float | 0-1 | Deposit color saturation |
| value | float | 0-1 | Deposit color brightness |

## Audio Mapping Ideas

| Parameter | Audio Source | Effect |
|-----------|--------------|--------|
| cohesionWeight | Bass energy | Swarm tightens on bass hits |
| separationWeight | High frequency | Swarm explodes on cymbals |
| maxSpeed | Overall amplitude | Faster movement during loud sections |
| attractMode | Beat detection | Flip attract/avoid on beats |
| perceptionRadius | Mid-range energy | Tighter/looser flocking |

## Performance Considerations

**O(N²) complexity**: Each boid checks all others. For 10K boids = 100M distance checks per frame.

**Mitigation options (future optimization):**
1. **Brute force with early-out**: Skip boids outside perception radius quickly
2. **Spatial hashing**: Divide space into cells, only check neighboring cells
3. **Shared memory tiling**: Load boid chunks into shared memory for faster access

For initial implementation, brute force with ~10K agents should maintain 60fps on modern GPUs.

## Notes

- Unlike Physarum, boids need velocity vectors (2 floats) not just heading (1 float)
- Agent struct requires 8-float alignment for GPU buffer efficiency
- Wrap-around boundaries match existing simulations
- Color deposit follows Physarum pattern: HSV → RGB based on agent hue
