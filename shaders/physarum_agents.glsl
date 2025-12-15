#version 430

// Jones 2010 physarum algorithm - intensity-based sensing and deposition

layout(local_size_x = 1024) in;

struct Agent {
    float x;
    float y;
    float heading;
    float _pad;  // Maintain 16-byte alignment for GPU
};

layout(std430, binding = 0) buffer AgentBuffer {
    Agent agents[];
};

layout(r32f, binding = 1) uniform image2D trailMap;

uniform vec2 resolution;
uniform float sensorDistance;
uniform float sensorAngle;
uniform float turningAngle;
uniform float stepSize;
uniform float depositAmount;
uniform float time;

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

float sampleTrail(vec2 pos)
{
    ivec2 coord = ivec2(mod(pos, resolution));
    return imageLoad(trailMap, coord).r;
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

    float front = sampleTrail(frontPos);
    float left = sampleTrail(leftPos);
    float right = sampleTrail(rightPos);

    uint hashState = hash(id + uint(time * 1000.0));
    float rnd = float(hashState) / 4294967295.0;

    if (front > left && front > right) {
        // No turn needed
    } else if (front < left && front < right) {
        // Both sides better: random turn
        agent.heading += (rnd - 0.5) * turningAngle * 2.0;
    } else if (left > right) {
        agent.heading += turningAngle;
    } else if (right > left) {
        agent.heading -= turningAngle;
    }

    // Move forward in the NEW heading direction (after turning)
    vec2 moveDir = vec2(cos(agent.heading), sin(agent.heading));
    pos += moveDir * stepSize;

    // Wrap at boundaries
    pos = mod(pos, resolution);

    agent.x = pos.x;
    agent.y = pos.y;

    ivec2 coord = ivec2(pos);
    float current = imageLoad(trailMap, coord).r;
    float deposited = min(current + depositAmount, 1.0);
    imageStore(trailMap, coord, vec4(deposited, 0.0, 0.0, 0.0));

    agents[id] = agent;
}
