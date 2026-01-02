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
uniform float rosslerC;
uniform float thomasB;
uniform vec2 center;           // Screen position (0-1 normalized, 0.5=center)
uniform vec3 rotation;         // Rotation angles in radians (X, Y, Z)
uniform float depositAmount;
uniform float saturation;
uniform float value;
uniform int attractorType;

const float PI = 3.14159265359;
const float EXPLOSION_THRESHOLD = 500.0;  // Respawn when position exceeds this distance from origin

// 3D rotation matrix from Euler angles (XYZ order)
mat3 rotationMatrix(vec3 angles)
{
    float cx = cos(angles.x), sx = sin(angles.x);
    float cy = cos(angles.y), sy = sin(angles.y);
    float cz = cos(angles.z), sz = sin(angles.z);

    mat3 rx = mat3(
        1.0, 0.0, 0.0,
        0.0, cx, -sx,
        0.0, sx, cx
    );
    mat3 ry = mat3(
        cy, 0.0, sy,
        0.0, 1.0, 0.0,
        -sy, 0.0, cy
    );
    mat3 rz = mat3(
        cz, -sz, 0.0,
        sz, cz, 0.0,
        0.0, 0.0, 1.0
    );
    return rz * ry * rx;
}

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

// Rössler system derivatives
// Classic parameters: a=0.2, b=0.2, c=5.7 (c exposed as uniform)
vec3 rosslerDerivative(vec3 p)
{
    const float a = 0.2;
    const float b = 0.2;
    return vec3(
        -p.y - p.z,
        p.x + a * p.y,
        b + p.z * (p.x - rosslerC)
    );
}

// Aizawa system derivatives
// Classic parameters: a=0.95, b=0.7, c=0.6, d=3.5, e=0.25, f=0.1
vec3 aizawaDerivative(vec3 p)
{
    const float a = 0.95;
    const float b = 0.7;
    const float c = 0.6;
    const float d = 3.5;
    const float e = 0.25;
    const float f = 0.1;
    return vec3(
        (p.z - b) * p.x - d * p.y,
        d * p.x + (p.z - b) * p.y,
        c + a * p.z - (p.z * p.z * p.z) / 3.0 - (p.x * p.x + p.y * p.y) * (1.0 + e * p.z) + f * p.z * p.x * p.x * p.x
    );
}

// Thomas system derivatives
// Classic parameter: b=0.208186 (chaotic regime, exposed as uniform)
vec3 thomasDerivative(vec3 p)
{
    return vec3(
        sin(p.y) - thomasB * p.x,
        sin(p.z) - thomasB * p.y,
        sin(p.x) - thomasB * p.z
    );
}

// Select derivative based on attractor type
vec3 attractorDerivative(vec3 p)
{
    if (attractorType == 1) {
        return rosslerDerivative(p);
    } else if (attractorType == 2) {
        return aizawaDerivative(p);
    } else if (attractorType == 3) {
        return thomasDerivative(p);
    }
    return lorenzDerivative(p);
}

// RK4 integration step
vec3 rk4Step(vec3 p, float dt)
{
    vec3 k1 = attractorDerivative(p);
    vec3 k2 = attractorDerivative(p + 0.5 * dt * k1);
    vec3 k3 = attractorDerivative(p + 0.5 * dt * k2);
    vec3 k4 = attractorDerivative(p + dt * k3);
    return p + (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
}

// Orthographic projection: 3D attractor -> 2D screen
// Applies rotation, centers the attractor, and scales to fit screen
vec2 projectToScreen(vec3 p)
{
    // Center the attractor around its natural origin
    vec3 centered;
    if (attractorType == 0) {
        // Lorenz: centered around (0, 0, 27)
        centered = vec3(p.x, p.y, p.z - 27.0);
    } else {
        // Rössler, Aizawa, Thomas: already centered around origin
        centered = p;
    }

    // Apply 3D rotation
    vec3 rotated = rotationMatrix(rotation) * centered;

    // Project to 2D with attractor-specific scaling
    vec2 projected;
    if (attractorType == 0) {
        // Lorenz: project X/Z plane
        projected = vec2(rotated.x, rotated.z);
    } else if (attractorType == 1) {
        // Rössler: project X/Y plane
        projected = vec2(rotated.x, rotated.y);
    } else if (attractorType == 2) {
        // Aizawa: compact shape, scale up
        projected = vec2(rotated.x * 8.0, rotated.z * 8.0);
    } else {
        // Thomas: medium scale
        projected = vec2(rotated.x * 4.0, rotated.y * 4.0);
    }

    // Scale and position on screen
    vec2 screen = projected * attractorScale * min(resolution.x, resolution.y);
    return screen + center * resolution;
}

// Respawn agent near attractor with random offset
void respawnAgent(inout Agent agent, uint id)
{
    uint seed = hash(id + uint(time * 1000.0));

    if (attractorType == 0) {
        // Lorenz: start near one of the two wings
        float wing = hashFloat(seed) > 0.5 ? 1.0 : -1.0;
        agent.x = wing * 8.5 + (hashFloat(seed + 1u) - 0.5) * 5.0;
        agent.y = wing * 8.5 + (hashFloat(seed + 2u) - 0.5) * 5.0;
        agent.z = 27.0 + (hashFloat(seed + 3u) - 0.5) * 10.0;
    } else if (attractorType == 1) {
        // Rössler: start near the spiral center
        agent.x = (hashFloat(seed + 1u) - 0.5) * 4.0;
        agent.y = (hashFloat(seed + 2u) - 0.5) * 4.0;
        agent.z = (hashFloat(seed + 3u) - 0.5) * 2.0;
    } else if (attractorType == 2) {
        // Aizawa: start near the torus center
        agent.x = (hashFloat(seed + 1u) - 0.5) * 1.0;
        agent.y = (hashFloat(seed + 2u) - 0.5) * 1.0;
        agent.z = (hashFloat(seed + 3u) - 0.5) * 1.0;
    } else {
        // Thomas: start with small random position
        agent.x = (hashFloat(seed + 1u) - 0.5) * 2.0;
        agent.y = (hashFloat(seed + 2u) - 0.5) * 2.0;
        agent.z = (hashFloat(seed + 3u) - 0.5) * 2.0;
    }
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
    bool unstable = isnan(magnitude) || isinf(magnitude) || magnitude > EXPLOSION_THRESHOLD;

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

    // Hue is fixed per agent, set at initialization based on ColorConfig

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
