#version 430

// Particle Life: emergent behavior from species-based attraction/repulsion rules
// Particles interact via piecewise force function with inner repulsion zone and outer interaction zone

layout(local_size_x = 1024) in;

struct Particle {
    float x;
    float y;
    float z;
    float vx;
    float vy;
    float vz;
    float hue;
    int species;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;

uniform vec2 resolution;
uniform float time;
uniform int numParticles;
uniform int numSpecies;
uniform float rMax;
uniform float forceFactor;
uniform float friction;
uniform float beta;
uniform float centeringStrength;
uniform float centeringFalloff;
uniform float timeStep;
uniform vec2 center;
uniform mat3 rotationMatrix;
uniform float projectionScale;
uniform float depositAmount;
uniform float saturation;
uniform float value;
uniform float attractionMatrix[64];  // Up to 8x8 species

// HSV to RGB conversion
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Hash function for pseudo-random respawn
uint hash(uint x)
{
    x ^= x >> 16;
    x *= 0x85ebca6bu;
    x ^= x >> 13;
    x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}

float hashFloat(uint x)
{
    return float(hash(x)) / 4294967295.0;
}

// Piecewise force function for particle life
// r: normalized distance (0 to 1, where 1 = rMax)
// a: attraction value from matrix [-1, 1]
// b: beta threshold (inner repulsion zone boundary)
float force(float r, float a, float b)
{
    if (r < b) {
        // Inner repulsion zone: -1 at r=0, approaches 0 at r=beta
        return r / b - 1.0;
    } else if (r < 1.0) {
        // Interaction zone: attraction/repulsion based on species pair
        return a * (1.0 - abs(2.0 * r - 1.0 - b) / (1.0 - b));
    }
    return 0.0;
}

// Project 3D position to 2D screen coordinates
vec2 projectToScreen(vec3 pos)
{
    vec3 rotated = rotationMatrix * pos;
    vec2 projected = vec2(rotated.x, rotated.z);  // Drop Y (depth)
    return projected * projectionScale * min(resolution.x, resolution.y) + center * resolution;
}

// Check if screen position is within bounds
bool onScreen(vec2 screenPos)
{
    return screenPos.x >= 0.0 && screenPos.x < resolution.x &&
           screenPos.y >= 0.0 && screenPos.y < resolution.y;
}

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= uint(numParticles)) {
        return;
    }

    Particle p = particles[id];
    vec3 pos = vec3(p.x, p.y, p.z);
    vec3 vel = vec3(p.vx, p.vy, p.vz);

    vec3 totalForce = vec3(0.0);

    // Brute-force neighbor iteration for force accumulation
    for (uint j = 0; j < uint(numParticles); j++) {
        if (id == j) {
            continue;
        }

        Particle other = particles[j];
        vec3 otherPos = vec3(other.x, other.y, other.z);
        vec3 delta = otherPos - pos;
        float r = length(delta);

        if (r > 0.0 && r < rMax) {
            // Look up attraction value from species pair matrix
            int matrixIndex = p.species * numSpecies + other.species;
            float a = attractionMatrix[matrixIndex];

            // Compute force magnitude using normalized distance
            float f = force(r / rMax, a, beta);

            // Accumulate force in direction of neighbor
            totalForce += (delta / r) * f;
        }
    }

    // Centering force: exponential falloff keeps particles from drifting to infinity
    float distFromCenter = length(pos);
    totalForce -= pos * centeringStrength * exp(distFromCenter * centeringFalloff);

    // Scale force by interaction radius and user factor
    totalForce *= rMax * forceFactor;

    // Semi-implicit Euler integration
    vel *= friction;
    vel += totalForce * timeStep;
    pos += vel * timeStep;

    // Project to screen and deposit trail
    vec2 screenPos = projectToScreen(pos);

    if (onScreen(screenPos)) {
        ivec2 coord = ivec2(screenPos);
        vec3 depositColor = hsv2rgb(vec3(p.hue, saturation, value));

        vec4 current = imageLoad(trailMap, coord);
        vec3 newColor = current.rgb + depositColor * depositAmount;

        // Proportional scaling to prevent overflow
        float maxChan = max(newColor.r, max(newColor.g, newColor.b));
        if (maxChan > 1.0) {
            newColor /= maxChan;
        }

        imageStore(trailMap, coord, vec4(newColor, 0.0));
    }

    // Store updated state
    particles[id].x = pos.x;
    particles[id].y = pos.y;
    particles[id].z = pos.z;
    particles[id].vx = vel.x;
    particles[id].vy = vel.y;
    particles[id].vz = vel.z;
    // hue and species remain unchanged
}
