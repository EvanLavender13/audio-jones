#version 430

// Physarum algorithm with hue-based species sensing and color deposition

layout(local_size_x = 1024) in;

struct Agent {
    float x;
    float y;
    float heading;
    float hue;  // Agent's hue identity (0-1)
};

layout(std430, binding = 0) buffer AgentBuffer {
    Agent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;

uniform vec2 resolution;
uniform float sensorDistance;
uniform float sensorAngle;
uniform float turningAngle;
uniform float stepSize;
uniform float depositAmount;
uniform float time;
uniform float saturation;
uniform float value;

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

// Sample trail and compute hue affinity (lower = more similar to agent)
float sampleTrailAffinity(vec2 pos, float agentHue)
{
    ivec2 coord = ivec2(mod(pos, resolution));
    vec3 trailColor = imageLoad(trailMap, coord).rgb;
    float trailIntensity = dot(trailColor, LUMA_WEIGHTS);

    if (trailIntensity < 0.001) {
        return 1.0;  // No trail = maximum difference (least attractive)
    }

    vec3 trailHSV = rgb2hsv(trailColor);
    return hueDifference(agentHue, trailHSV.x);
}

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= agents.length()) {
        return;
    }

    Agent agent = agents[id];
    vec2 pos = vec2(agent.x, agent.y);

    // Sensor positions (Jones 2010: three forward-facing sensors)
    vec2 frontDir = vec2(cos(agent.heading), sin(agent.heading));
    vec2 leftDir = vec2(cos(agent.heading + sensorAngle), sin(agent.heading + sensorAngle));
    vec2 rightDir = vec2(cos(agent.heading - sensorAngle), sin(agent.heading - sensorAngle));

    vec2 frontPos = pos + frontDir * sensorDistance;
    vec2 leftPos = pos + leftDir * sensorDistance;
    vec2 rightPos = pos + rightDir * sensorDistance;

    // Sample hue affinity (lower = more attractive, turn toward similar colors)
    float front = sampleTrailAffinity(frontPos, agent.hue);
    float left = sampleTrailAffinity(leftPos, agent.hue);
    float right = sampleTrailAffinity(rightPos, agent.hue);

    uint hashState = hash(id + uint(time * 1000.0));
    float rnd = float(hashState) / 4294967295.0;

    // Turn toward LOWEST hue difference (most similar color)
    if (front < left && front < right) {
        // Front is most similar, no turn needed
    } else if (front > left && front > right) {
        // Both sides more similar: random turn
        agent.heading += (rnd - 0.5) * turningAngle * 2.0;
    } else if (left < right) {
        // Left is more similar
        agent.heading += turningAngle;
    } else if (right < left) {
        // Right is more similar
        agent.heading -= turningAngle;
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
    vec4 deposited = clamp(current + vec4(depositColor * depositAmount, 0.0), 0.0, 1.0);
    imageStore(trailMap, coord, deposited);

    agents[id] = agent;
}
