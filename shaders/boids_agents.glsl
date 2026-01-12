#version 430

// Boids flocking simulation with spatial hash acceleration

layout(local_size_x = 1024) in;

struct BoidAgent {
    float x;
    float y;
    float vx;
    float vy;
    float hue;         // 0-1 range for deposit color and hue affinity
    float _pad1;
    float _pad2;
    float _pad3;
};

layout(std430, binding = 0) buffer AgentBuffer {
    BoidAgent boids[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;

layout(std430, binding = 2) buffer CellOffsets {
    uint cellOffsets[];
};

layout(std430, binding = 3) buffer SortedIndices {
    uint sortedIndices[];
};

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
uniform ivec2 gridSize;
uniform float cellSize;

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

// Circular hue distance (handles wrap at 0/1 boundary)
float hueDistance(float hue1, float hue2)
{
    return min(abs(hue1 - hue2), 1.0 - abs(hue1 - hue2));
}

// Position to cell index (mod ensures pos in [0, resolution), so cellCoord is always valid)
int positionToCell(vec2 pos)
{
    pos = mod(pos, resolution);
    ivec2 cellCoord = ivec2(floor(pos / cellSize));
    return cellCoord.y * gridSize.x + cellCoord.x;
}

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= boids.length()) {
        return;
    }

    BoidAgent self = boids[id];
    vec2 selfPos = vec2(self.x, self.y);
    vec2 selfVel = vec2(self.vx, self.vy);
    float selfHue = self.hue;

    // Fused steering accumulators
    vec2 cohesionSum = vec2(0.0);
    vec2 separationSum = vec2(0.0);
    vec2 alignmentSum = vec2(0.0);
    float cohesionWeight_acc = 0.0;
    float alignmentWeight_acc = 0.0;

    // Get cell coordinates for this boid
    ivec2 myCell = ivec2(floor(mod(selfPos, resolution) / cellSize));

    // Dynamic scan radius: minimum 2 (5x5) to avoid edge artifacts, expand further for large perception
    int scanRadius = max(2, int(ceil(perceptionRadius / cellSize)));

    for (int dy = -scanRadius; dy <= scanRadius; dy++) {
        for (int dx = -scanRadius; dx <= scanRadius; dx++) {
            // Toroidal wrap for neighbor cell
            ivec2 neighborCell = (myCell + ivec2(dx, dy) + gridSize) % gridSize;
            int cellIdx = neighborCell.y * gridSize.x + neighborCell.x;

            // Get range for this cell (exclusive prefix sum)
            int start = (cellIdx == 0) ? 0 : int(cellOffsets[cellIdx - 1]);
            int end = int(cellOffsets[cellIdx]);

            // Process all boids in this cell
            for (int i = start; i < end; i++) {
                uint otherId = sortedIndices[i];
                if (otherId == id) {
                    continue;
                }

                BoidAgent other = boids[otherId];
                vec2 otherPos = vec2(other.x, other.y);
                vec2 delta = wrapDelta(selfPos, otherPos);
                float dist = length(delta);

                // Cohesion and alignment (within perception radius)
                if (dist < perceptionRadius) {
                    float affinity = 1.0 - hueDistance(other.hue, selfHue) * hueAffinity;
                    cohesionSum += delta * affinity;
                    cohesionWeight_acc += affinity;

                    vec2 otherVel = vec2(other.vx, other.vy);
                    alignmentSum += otherVel * affinity;
                    alignmentWeight_acc += affinity;
                }

                // Separation (within separation radius)
                if (dist < separationRadius && dist > 0.0) {
                    float repulsion = 1.0 + hueDistance(other.hue, selfHue) * hueAffinity * 2.0;
                    separationSum -= delta / (dist * dist) * repulsion;
                }
            }
        }
    }

    // Compute final steering forces
    vec2 v1 = cohesionWeight_acc > 0.001 ? (cohesionSum / cohesionWeight_acc) * 0.01 : vec2(0.0);
    vec2 v2 = separationSum;
    vec2 v3 = alignmentWeight_acc > 0.001 ? (alignmentSum / alignmentWeight_acc - selfVel) * 0.125 : vec2(0.0);

    // Apply weighted steering forces
    selfVel += v1 * cohesionWeight
             + v2 * separationWeight
             + v3 * alignmentWeight;

    // Clamp velocity magnitude
    float speed = length(selfVel);
    if (speed > maxSpeed) {
        selfVel = (selfVel / speed) * maxSpeed;
    } else if (speed < minSpeed && speed > 0.001) {
        selfVel = (selfVel / speed) * minSpeed;
    }

    // Update position and wrap at boundaries
    selfPos += selfVel;
    selfPos = mod(selfPos, resolution);

    self.x = selfPos.x;
    self.y = selfPos.y;
    self.vx = selfVel.x;
    self.vy = selfVel.y;

    // Deposit color based on agent hue
    ivec2 coord = ivec2(selfPos);
    vec3 depositColor = hsv2rgb(vec3(self.hue, saturation, value));
    vec4 current = imageLoad(trailMap, coord);
    vec3 newColor = current.rgb + depositColor * depositAmount;

    // Scale proportionally to prevent overflow while preserving color ratios
    float maxChan = max(newColor.r, max(newColor.g, newColor.b));
    if (maxChan > 1.0) {
        newColor /= maxChan;
    }

    imageStore(trailMap, coord, vec4(newColor, 0.0));

    boids[id] = self;
}
