#version 430

// Boids flocking simulation with hue-weighted cohesion and texture reaction

layout(local_size_x = 1024) in;

struct BoidAgent {
    float x;           // Position X
    float y;           // Position Y
    float vx;          // Velocity X
    float vy;          // Velocity Y
    float hue;         // Agent's hue identity (0-1) for deposit color and affinity
    float spectrumPos; // Position in color distribution (0-1) for FFT lookup
    float _pad1;
    float _pad2;
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
uniform float textureWeight;
uniform float attractMode;
uniform float sensorDistance;
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

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= boids.length()) {
        return;
    }

    BoidAgent b = boids[id];

    // Phase 2 will implement steering behaviors
    // For now, just move forward and wrap

    vec2 pos = vec2(b.x, b.y);
    vec2 vel = vec2(b.vx, b.vy);

    pos += vel;
    pos = mod(pos, resolution);

    b.x = pos.x;
    b.y = pos.y;

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
