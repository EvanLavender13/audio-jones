#version 430

// Physarum agent update compute shader
// Based on Jeff Jones' algorithm (2010) with hue-based species separation
// Agents sense chemical trails, turn toward similar hues (repel from opposites),
// move, and deposit colored trails

layout(local_size_x = 1024) in;

struct Agent {
    float x;
    float y;
    float heading;
    float hue;  // Species identity (0-1 range)
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

// HSV to RGB conversion
vec3 hsv2rgb(vec3 hsv)
{
    float h = hsv.x * 6.0;
    float s = hsv.y;
    float v = hsv.z;

    float c = v * s;
    float x = c * (1.0 - abs(mod(h, 2.0) - 1.0));
    float m = v - c;

    vec3 rgb;
    if (h < 1.0) {
        rgb = vec3(c, x, 0.0);
    } else if (h < 2.0) {
        rgb = vec3(x, c, 0.0);
    } else if (h < 3.0) {
        rgb = vec3(0.0, c, x);
    } else if (h < 4.0) {
        rgb = vec3(0.0, x, c);
    } else if (h < 5.0) {
        rgb = vec3(x, 0.0, c);
    } else {
        rgb = vec3(c, 0.0, x);
    }

    return rgb + m;
}

// RGB to hue extraction (returns 0-1)
float rgb2hue(vec3 rgb)
{
    float maxC = max(rgb.r, max(rgb.g, rgb.b));
    float minC = min(rgb.r, min(rgb.g, rgb.b));
    float delta = maxC - minC;

    if (delta < 0.001) {
        return 0.0;  // Grayscale, hue undefined
    }

    float hue;
    // Use epsilon comparison for floating point reliability
    if (rgb.r >= rgb.g && rgb.r >= rgb.b) {
        hue = (rgb.g - rgb.b) / delta;
    } else if (rgb.g >= rgb.b) {
        hue = 2.0 + (rgb.b - rgb.r) / delta;
    } else {
        hue = 4.0 + (rgb.r - rgb.g) / delta;
    }

    hue /= 6.0;
    if (hue < 0.0) {
        hue += 1.0;
    }

    return hue;
}

// Sample trail and compute attraction score based on hue similarity
// Returns: +1 (same hue, attract) to -1 (opposite hue, repel)
float sampleTrailScore(vec2 pos, float agentHue)
{
    ivec2 coord = ivec2(mod(pos, resolution));
    vec4 trail = imageLoad(trailMap, coord);

    // Get trail intensity (brightness)
    float intensity = max(trail.r, max(trail.g, trail.b));
    if (intensity < 0.01) {
        return 0.0;  // No trail to sense
    }

    // Extract hue from trail color
    float trailHue = rgb2hue(trail.rgb);

    // Circular hue distance (0 = same, 0.5 = opposite on color wheel)
    float hueDiff = min(abs(agentHue - trailHue), 1.0 - abs(agentHue - trailHue));

    // Convert to attraction/repulsion score:
    //   +1.0 = same hue (max attraction)
    //    0.0 = 90 degrees apart (neutral)
    //   -1.0 = opposite hue (max repulsion)
    float hueScore = 1.0 - 4.0 * hueDiff;

    // Weight by trail intensity
    return hueScore * intensity;
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

    // Sample trail scores (hue-based attraction/repulsion)
    float front = sampleTrailScore(frontPos, agent.hue);
    float left = sampleTrailScore(leftPos, agent.hue);
    float right = sampleTrailScore(rightPos, agent.hue);

    // Rotation decision (Jones 2010 algorithm adapted for signed scores)
    uint hashState = hash(id + uint(time * 1000.0));
    float rnd = float(hashState) / 4294967295.0;

    if (front > left && front > right) {
        // Keep heading - best score ahead
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

    // Deposit colored trail at new position
    // Use lerp blending to preserve hue (prevents white saturation from additive mixing)
    ivec2 coord = ivec2(pos);
    vec3 depositColor = hsv2rgb(vec3(agent.hue, saturation, value));
    vec4 current = imageLoad(trailMap, coord);

    // Blend toward deposit color - stronger deposit = more influence
    // This keeps colors saturated instead of washing out to white
    float blendFactor = clamp(depositAmount * 0.5, 0.0, 1.0);
    vec3 blended = mix(current.rgb, depositColor, blendFactor);

    // Also brighten slightly to make trails visible
    blended = min(blended + depositColor * 0.1, vec3(1.0));

    imageStore(trailMap, coord, vec4(blended, current.a));

    agents[id] = agent;
}
