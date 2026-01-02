#version 430

// Strange attractor flow: agents trace trajectories through chaotic dynamical systems
// Particles follow deterministic ODEs with RK4 integration, projected orthographically

layout(local_size_x = 1024) in;

struct Agent {
    float x;
    float y;
    float z;
    float hue;
    float age;
    float _pad1;
    float _pad2;
    float _pad3;
};

layout(std430, binding = 0) buffer AgentBuffer {
    Agent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;

uniform vec2 resolution;
uniform float time;
uniform float timeScale;
uniform float attractorScale;
uniform float sigma;
uniform float rho;
uniform float beta;
uniform float depositAmount;
uniform float saturation;
uniform float value;

const float PI = 3.14159265359;

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

// Lorenz system derivatives
// Classic parameters: sigma=10, rho=28, beta=8/3
vec3 lorenzDerivative(vec3 p)
{
    return vec3(
        sigma * (p.y - p.x),
        p.x * (rho - p.z) - p.y,
        p.x * p.y - beta * p.z
    );
}

// RK4 integration step
vec3 rk4Step(vec3 p, float dt)
{
    vec3 k1 = lorenzDerivative(p);
    vec3 k2 = lorenzDerivative(p + 0.5 * dt * k1);
    vec3 k3 = lorenzDerivative(p + 0.5 * dt * k2);
    vec3 k4 = lorenzDerivative(p + dt * k3);
    return p + (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
}

// Orthographic projection: 3D attractor -> 2D screen
// Centers the attractor and scales to fit screen
vec2 projectToScreen(vec3 p)
{
    // Lorenz attractor is centered around (0, 0, 27) with wings at x ≈ ±8.5
    // Shift z down to center vertically
    vec2 centered = vec2(p.x, p.z - 27.0);
    vec2 screen = centered * attractorScale * min(resolution.x, resolution.y);
    return screen + resolution * 0.5;
}

// Respawn agent near attractor with random offset
void respawnAgent(inout Agent agent, uint id)
{
    uint seed = hash(id + uint(time * 1000.0));

    // Start near one of the two wings with small random offset
    float wing = hashFloat(seed) > 0.5 ? 1.0 : -1.0;
    agent.x = wing * 8.5 + (hashFloat(seed + 1u) - 0.5) * 5.0;
    agent.y = wing * 8.5 + (hashFloat(seed + 2u) - 0.5) * 5.0;
    agent.z = 27.0 + (hashFloat(seed + 3u) - 0.5) * 10.0;
    agent.hue = hashFloat(seed + 4u);
    agent.age = 0.0;
}

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= agents.length()) {
        return;
    }

    Agent agent = agents[id];
    vec3 pos = vec3(agent.x, agent.y, agent.z);

    // RK4 integration step
    pos = rk4Step(pos, timeScale);

    // Check for numerical instability (NaN or explosion)
    float magnitude = length(pos);
    bool unstable = isnan(magnitude) || isinf(magnitude) || magnitude > 500.0;

    if (unstable) {
        respawnAgent(agent, id);
        agents[id] = agent;
        return;
    }

    // Project to screen coordinates
    vec2 screenPos = projectToScreen(pos);

    // Check if on screen
    bool onScreen = screenPos.x >= 0.0 && screenPos.x < resolution.x &&
                    screenPos.y >= 0.0 && screenPos.y < resolution.y;

    // Update agent state
    agent.x = pos.x;
    agent.y = pos.y;
    agent.z = pos.z;
    agent.age += timeScale;

    // Slowly cycle hue based on age for color variation
    agent.hue = fract(agent.hue + timeScale * 0.001);

    // Deposit trail if on screen
    if (onScreen) {
        ivec2 coord = ivec2(screenPos);
        vec3 depositColor = hsv2rgb(vec3(agent.hue, saturation, value));

        vec4 current = imageLoad(trailMap, coord);
        vec3 newColor = current.rgb + depositColor * depositAmount;

        // Proportional scaling to prevent overflow
        float maxChan = max(newColor.r, max(newColor.g, newColor.b));
        if (maxChan > 1.0) {
            newColor /= maxChan;
        }

        imageStore(trailMap, coord, vec4(newColor, 0.0));
    }

    agents[id] = agent;
}
