#version 430

// Boids flocking simulation with hue-weighted cohesion

layout(local_size_x = 1024) in;

struct BoidAgent {
    float x;           // Position X
    float y;           // Position Y
    float vx;          // Velocity X
    float vy;          // Velocity Y
    float hue;         // Agent's hue identity (0-1) for deposit color and affinity
    float _pad1;
    float _pad2;
    float _pad3;
};

layout(std430, binding = 0) buffer AgentBuffer {
    BoidAgent boids[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;

uniform vec2 resolution;
uniform float perceptionRadius;
uniform float separationRadius;
uniform float cohesionWeight;
uniform float separationWeight;
uniform float alignmentWeight;
uniform float hueAffinity;
uniform float maxSpeed;
uniform float minSpeed;
uniform float depositAmount;
uniform float saturation;
uniform float value;
uniform int numBoids;

// HSV to RGB conversion
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Toroidal offset: shortest path accounting for screen wrap
vec2 wrapDelta(vec2 from, vec2 to)
{
    vec2 delta = to - from;
    return mod(delta + resolution * 0.5, resolution) - resolution * 0.5;
}

// Rule 1: Cohesion with hue affinity weighting
// Steer toward flock center, weighted by hue similarity
vec2 cohesion(uint selfId, vec2 selfPos, float selfHue)
{
    vec2 offsetSum = vec2(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < numBoids; i++) {
        if (i == int(selfId)) {
            continue;
        }

        BoidAgent other = boids[i];
        vec2 otherPos = vec2(other.x, other.y);
        vec2 delta = wrapDelta(selfPos, otherPos);
        float dist = length(delta);

        if (dist < perceptionRadius) {
            // Hue affinity: 1.0 when same hue, decays with hue difference
            float hueDiff = min(abs(other.hue - selfHue), 1.0 - abs(other.hue - selfHue));
            float affinity = 1.0 - hueDiff * hueAffinity;

            offsetSum += delta * affinity;
            totalWeight += affinity;
        }
    }

    if (totalWeight < 0.001) {
        return vec2(0.0);
    }

    return (offsetSum / totalWeight) * 0.01;
}

// Rule 2: Separation
// Steer away from nearby neighbors, repelling different hues more strongly
vec2 separation(uint selfId, vec2 selfPos, float selfHue)
{
    vec2 avoid = vec2(0.0);

    for (int i = 0; i < numBoids; i++) {
        if (i == int(selfId)) {
            continue;
        }

        BoidAgent other = boids[i];
        vec2 otherPos = vec2(other.x, other.y);
        vec2 delta = wrapDelta(selfPos, otherPos);
        float dist = length(delta);

        if (dist < separationRadius && dist > 0.0) {
            // Hue repulsion: different hues repel more strongly
            float hueDiff = min(abs(other.hue - selfHue), 1.0 - abs(other.hue - selfHue));
            float repulsion = 1.0 + hueDiff * hueAffinity * 2.0;

            avoid -= delta / (dist * dist) * repulsion;
        }
    }

    return avoid;
}

// Rule 3: Alignment
// Match average velocity of similar-hued neighbors
vec2 alignment(uint selfId, vec2 selfPos, vec2 selfVel, float selfHue)
{
    vec2 weightedVelocity = vec2(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < numBoids; i++) {
        if (i == int(selfId)) {
            continue;
        }

        BoidAgent other = boids[i];
        vec2 otherPos = vec2(other.x, other.y);
        vec2 delta = wrapDelta(selfPos, otherPos);
        float dist = length(delta);

        if (dist < perceptionRadius) {
            // Hue affinity: align more with same-hue neighbors
            float hueDiff = min(abs(other.hue - selfHue), 1.0 - abs(other.hue - selfHue));
            float affinity = 1.0 - hueDiff * hueAffinity;

            weightedVelocity += vec2(other.vx, other.vy) * affinity;
            totalWeight += affinity;
        }
    }

    if (totalWeight < 0.001) {
        return vec2(0.0);
    }

    weightedVelocity /= totalWeight;
    return (weightedVelocity - selfVel) * 0.125;
}

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= boids.length()) {
        return;
    }

    BoidAgent b = boids[id];
    vec2 pos = vec2(b.x, b.y);
    vec2 vel = vec2(b.vx, b.vy);

    // Compute steering forces
    vec2 v1 = cohesion(id, pos, b.hue);
    vec2 v2 = separation(id, pos, b.hue);
    vec2 v3 = alignment(id, pos, vel, b.hue);

    // Apply weighted steering forces
    vel += v1 * cohesionWeight
         + v2 * separationWeight
         + v3 * alignmentWeight;

    // Clamp velocity magnitude
    float speed = length(vel);
    if (speed > maxSpeed) {
        vel = (vel / speed) * maxSpeed;
    }
    if (speed < minSpeed && speed > 0.001) {
        vel = (vel / speed) * minSpeed;
    }

    // Update position and wrap at boundaries
    pos += vel;
    pos = mod(pos, resolution);

    b.x = pos.x;
    b.y = pos.y;
    b.vx = vel.x;
    b.vy = vel.y;

    // Deposit color based on agent hue
    ivec2 coord = ivec2(pos);
    vec3 depositColor = hsv2rgb(vec3(b.hue, saturation, value));
    vec4 current = imageLoad(trailMap, coord);
    vec3 newColor = current.rgb + depositColor * depositAmount;

    // Scale proportionally to prevent overflow while preserving color ratios
    float maxChan = max(newColor.r, max(newColor.g, newColor.b));
    if (maxChan > 1.0) {
        newColor /= maxChan;
    }

    imageStore(trailMap, coord, vec4(newColor, 0.0));

    boids[id] = b;
}
