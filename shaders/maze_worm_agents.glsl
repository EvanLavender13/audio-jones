// Based on "maze worms / graffitis 3" series by FabriceNeyret2
// https://www.shadertoy.com/view/XdjcRD
// License: CC BY-NC-SA 3.0 Unported
// Modified: Ported from Shadertoy fragment feedback to compute shader with SSBO agents and imageStore trail writes
#version 430

layout(local_size_x = 1024) in;

struct MazeWormAgent {
    float x, y;
    float angle;
    float age;
    float alive;
    float hue;
    float respawnTimer;
    float _pad;
};

layout(std430, binding = 0) buffer AgentBuffer {
    MazeWormAgent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;

uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform int turningMode;
uniform float curvature;
uniform float turnAngle;
uniform float trailWidth;
uniform float collisionGap;
uniform float time;
uniform float respawnCooldown;
uniform float stepDeltaTime;

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

// Single-point collision probe (same model as original Shadertoy)
// Probe reach = trailWidth + 1 (self-clear) + collisionGap
bool isWall(vec2 probePos) {
    float margin = trailWidth + 1.0 + collisionGap;
    if (probePos.x < margin || probePos.x > resolution.x - margin ||
        probePos.y < margin || probePos.y > resolution.y - margin) {
        return true;
    }
    return imageLoad(trailMap, ivec2(probePos)).w > 0.1;
}

vec2 probe(vec2 pos, float a) {
    float reach = trailWidth + 1.0 + collisionGap;
    return pos + reach * vec2(cos(a), sin(a));
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= agents.length()) return;

    MazeWormAgent agent = agents[idx];

    // GPU-side respawn for dead agents
    if (agent.alive < 0.5) {
        agent.respawnTimer -= stepDeltaTime;
        if (agent.respawnTimer > 0.0) {
            agents[idx].respawnTimer = agent.respawnTimer;
            return;
        }

        float seed = float(idx) * 1000.0 + time * 137.0;
        float margin = trailWidth + 1.0 + collisionGap;
        float rx = hash(seed);
        float ry = hash(seed + 1.0);
        float newX = margin + rx * (resolution.x - 2.0 * margin);
        float newY = margin + ry * (resolution.y - 2.0 * margin);

        if (imageLoad(trailMap, ivec2(newX, newY)).w < 0.1) {
            agents[idx].x = newX;
            agents[idx].y = newY;
            agents[idx].angle = hash(seed + 2.0) * 6.2832;
            agents[idx].age = 1.0;
            agents[idx].alive = 1.0;
            agents[idx].respawnTimer = 0.0;
        } else {
            agents[idx].respawnTimer = 0.0;
        }
        return;
    }

    vec2 pos = vec2(agent.x, agent.y);
    float a = agent.angle;

    // Steering
    if (turningMode == 0) {
        // SPIRAL: angle += curvature / age
        a += curvature / agent.age;
        if (isWall(probe(pos, a))) {
            agents[idx].alive = 0.0;
            agents[idx].respawnTimer = respawnCooldown;
            return;
        }
    } else if (turningMode == 1) {
        // WALL_FOLLOW: curve then dodge walls
        a -= curvature / sqrt(agent.age);
        float safeAngle = max(abs(turnAngle), 0.05);
        int maxAttempts = min(int(6.2832 / safeAngle) + 1, 128);
        int attempts = 0;
        while (isWall(probe(pos, a)) && attempts < maxAttempts) {
            a += turnAngle;
            attempts++;
        }
        if (attempts >= maxAttempts) {
            agents[idx].alive = 0.0;
            agents[idx].respawnTimer = respawnCooldown;
            return;
        }
    } else if (turningMode == 2) {
        // WALL_HUG: same as wall follow but curves opposite direction
        a += curvature / sqrt(agent.age);
        float safeAngle = max(abs(turnAngle), 0.05);
        int maxAttempts = min(int(6.2832 / safeAngle) + 1, 128);
        int attempts = 0;
        while (isWall(probe(pos, a)) && attempts < maxAttempts) {
            a -= turnAngle;
            attempts++;
        }
        if (attempts >= maxAttempts) {
            agents[idx].alive = 0.0;
            agents[idx].respawnTimer = respawnCooldown;
            return;
        }
    } else if (turningMode == 3) {
        // MIXED: CW/CCW chirality per agent
        float chirality = sign(sin(float(idx) + 0.5));
        a += curvature * 0.001 * agent.age * chirality;
        if (isWall(probe(pos, a))) {
            agents[idx].alive = 0.0;
            agents[idx].respawnTimer = respawnCooldown;
            return;
        }
    }

    // Move
    float newX = agent.x + cos(a);
    float newY = agent.y + sin(a);

    // Bounds check
    float margin = trailWidth + 1.0;
    if (newX < margin || newX > resolution.x - margin ||
        newY < margin || newY > resolution.y - margin) {
        agents[idx].alive = 0.0;
        agents[idx].respawnTimer = respawnCooldown;
        return;
    }

    // Write trail deposit within trailWidth only
    int r = int(ceil(trailWidth));
    vec3 rgb = texture(gradientLUT, vec2(agent.hue, 0.5)).rgb;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            float dist = length(vec2(float(dx), float(dy)));
            if (dist > trailWidth) continue;

            ivec2 writePos = ivec2(newX + float(dx), newY + float(dy));
            if (writePos.x < 0 || writePos.x >= int(resolution.x) ||
                writePos.y < 0 || writePos.y >= int(resolution.y)) continue;

            vec4 existing = imageLoad(trailMap, writePos);
            imageStore(trailMap, writePos, existing + vec4(rgb, 1.0));
        }
    }

    // Update agent state
    agents[idx].x = newX;
    agents[idx].y = newY;
    agents[idx].angle = mod(a, 6.2832);
    agents[idx].age = agent.age + 1.0;
}
