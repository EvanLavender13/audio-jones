#version 430

// Physarum algorithm with hue-based species sensing and color deposition

layout(local_size_x = 1024) in;

struct Agent {
    float x;
    float y;
    float heading;
    float hue;         // Agent's hue identity (0-1) for deposit color and affinity
    float _pad1;
    float _pad2;
    float _pad3;
    float _pad4;
};

layout(std430, binding = 0) buffer AgentBuffer {
    Agent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;
layout(binding = 2) uniform sampler2D accumMap;

uniform vec2 resolution;
uniform float accumSenseBlend;  // 0 = trail only, 1 = accum only
uniform float sensorDistance;
uniform float sensorDistanceVariance;
uniform float sensorAngle;
uniform float turningAngle;
uniform float stepSize;
uniform float depositAmount;
uniform float time;
uniform float saturation;
uniform float value;
uniform float repulsionStrength;
uniform float samplingExponent;
uniform float vectorSteering;

// Standard luminance weights (Rec. 601)
const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

// Hash for stochastic behavior (based on Sage Jenson's approach)
uint hash(uint state)
{
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    return state;
}

// Box-Muller: generate Gaussian from two uniform randoms (mean=0, stddev=1)
float gaussian(inout uint state)
{
    float u1 = float(hash(state)) / 4294967295.0;
    state = hash(state);
    float u2 = float(hash(state)) / 4294967295.0;
    state = hash(state);
    u1 = max(u1, 1e-6);
    return sqrt(-2.0 * log(u1)) * cos(6.28318530718 * u2);
}

// RGB to HSV conversion
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// HSV to RGB conversion
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Compute hue difference with wraparound handling (hue is circular 0-1)
float hueDifference(float h1, float h2)
{
    float diff = abs(h1 - h2);
    return min(diff, 1.0 - diff);
}

// Compute affinity from color (lower = more attractive)
// repulsionStrength=0: original behavior (all trails attract, similar hue more)
// repulsionStrength>0: competitive species (same hue attracts, different repels)
float computeAffinity(vec3 color, float agentHue)
{
    float intensity = dot(color, LUMA_WEIGHTS);

    if (intensity < 0.001) {
        return mix(1.0, 0.5, repulsionStrength);
    }

    vec3 hsv = rgb2hsv(color);
    float hueDiff = hueDifference(agentHue, hsv.x);  // 0 = same, 0.5 = opposite

    // Original: all trails attract, similar hue attracts more
    float oldAffinity = hueDiff + (1.0 - intensity) * 0.3;

    // Competitive: same hue attracts, different hue repels
    float attraction = intensity * (1.0 - hueDiff * 2.0);
    float repulsion = intensity * hueDiff * 2.0;
    float newAffinity = 0.5 - attraction * 0.5 + repulsion * 2.0;

    return mix(oldAffinity, newAffinity, repulsionStrength);
}

float sampleTrailAffinity(vec2 pos, float agentHue)
{
    ivec2 coord = ivec2(mod(pos, resolution));
    vec3 trailColor = imageLoad(trailMap, coord).rgb;
    return computeAffinity(trailColor, agentHue);
}

float sampleAccumAffinity(vec2 pos, float agentHue)
{
    vec2 uv = pos / resolution;
    vec3 accumColor = texture(accumMap, uv).rgb;
    return computeAffinity(accumColor, agentHue);
}

// Sample and blend trail + accum affinity at position
float sampleBlendedAffinity(vec2 pos, float agentHue)
{
    float trail = sampleTrailAffinity(pos, agentHue);
    float accum = sampleAccumAffinity(pos, agentHue);
    return mix(trail, accum, accumSenseBlend);
}

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= agents.length()) {
        return;
    }

    Agent agent = agents[id];
    vec2 pos = vec2(agent.x, agent.y);

    uint hashState = hash(id + uint(time * 1000.0));

    // Sensor directions (Jones 2010: three forward-facing sensors)
    vec2 frontDir = vec2(cos(agent.heading), sin(agent.heading));
    vec2 leftDir = vec2(cos(agent.heading + sensorAngle), sin(agent.heading + sensorAngle));
    vec2 rightDir = vec2(cos(agent.heading - sensorAngle), sin(agent.heading - sensorAngle));

    // Sample per-agent sensing distance from Gaussian distribution
    float agentSensorDist = sensorDistance;
    if (sensorDistanceVariance > 0.001) {
        float offset = gaussian(hashState) * sensorDistanceVariance;
        agentSensorDist = clamp(sensorDistance + offset, 1.0, sensorDistance * 2.0);
    }

    vec2 frontPos = pos + frontDir * agentSensorDist;
    vec2 leftPos = pos + leftDir * agentSensorDist;
    vec2 rightPos = pos + rightDir * agentSensorDist;

    // Sample blended affinity (lower = more attractive)
    float front = sampleBlendedAffinity(frontPos, agent.hue);
    float left = sampleBlendedAffinity(leftPos, agent.hue);
    float right = sampleBlendedAffinity(rightPos, agent.hue);

    float rnd = float(hashState) / 4294967295.0;

    if (vectorSteering > 0.5) {
        // Vector steering: attractive pulls toward, repulsive pushes away
        vec2 frontForce = frontDir * (0.5 - front);
        vec2 leftForce = leftDir * (0.5 - left);
        vec2 rightForce = rightDir * (0.5 - right);

        vec2 steering = frontForce + leftForce + rightForce;
        float steerMag = length(steering);

        if (steerMag > 0.001) {
            float targetAngle = atan(steering.y, steering.x);
            float angleDiff = targetAngle - agent.heading;
            angleDiff = mod(angleDiff + 3.14159, 6.28318) - 3.14159;
            agent.heading += clamp(angleDiff, -turningAngle, turningAngle);
        } else {
            agent.heading += (rnd - 0.5) * turningAngle * 2.0;
        }
    } else {
        // Discrete steering mode
        if (samplingExponent > 0.0) {
            // Stochastic mutation (MCPM): probabilistic choice between forward and mutation
            float d0 = front;  // Forward affinity
            float d1;          // Best lateral affinity
            float turnDir;     // Mutation turn direction

            if (left < right) {
                d1 = left;
                turnDir = 1.0;
            } else {
                d1 = right;
                turnDir = -1.0;
            }

            // MCPM probability formula: Pmut = d1^exp / (d0^exp + d1^exp)
            float p0 = pow(d0, samplingExponent);
            float p1 = pow(d1, samplingExponent);
            float Pmut = p1 / (p0 + p1 + 0.0001);

            if (rnd < Pmut) {
                agent.heading += turnDir * turningAngle;
            }
            // else: no turn (maintain heading)
        } else {
            // Original deterministic steering (samplingExponent = 0)
            if (front < left && front < right) {
                // Front is best, no turn
            } else if (front > left && front > right) {
                // Both sides better: random turn
                agent.heading += (rnd - 0.5) * turningAngle * 2.0;
            } else if (left < right) {
                agent.heading += turningAngle;
            } else if (right < left) {
                agent.heading -= turningAngle;
            }
        }
    }

    // Move forward in the NEW heading direction (after turning)
    vec2 moveDir = vec2(cos(agent.heading), sin(agent.heading));
    pos += moveDir * stepSize;

    // Wrap at boundaries
    pos = mod(pos, resolution);

    agent.x = pos.x;
    agent.y = pos.y;

    // Deposit color based on agent hue
    ivec2 coord = ivec2(pos);
    vec3 depositColor = hsv2rgb(vec3(agent.hue, saturation, value));
    vec4 current = imageLoad(trailMap, coord);
    vec3 newColor = current.rgb + depositColor * depositAmount;

    // Scale proportionally to prevent overflow while preserving color ratios (saturation)
    // Independent clamping causes desaturation: (1.2, 0.3, 0.1) → (1.0, 0.3, 0.1)
    // Proportional scaling preserves hue: (1.2, 0.3, 0.1) → (1.0, 0.25, 0.083)
    float maxChan = max(newColor.r, max(newColor.g, newColor.b));
    if (maxChan > 1.0) {
        newColor /= maxChan;
    }

    imageStore(trailMap, coord, vec4(newColor, 0.0));

    agents[id] = agent;
}
