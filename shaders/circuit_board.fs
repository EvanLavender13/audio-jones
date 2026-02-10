// shaders/circuit_board.fs — Squarenoi SDF warp: second-closest Voronoi
// distance creates PCB trace pathways via UV displacement.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float tileScale;
uniform float strength;
uniform float baseSize;
uniform float breathe;
uniform float time;
uniform int dualLayer;
uniform float layerOffset;
uniform float contourFreq;

const float TAU = 6.28318530718;

// --- Murmur hash: uvec3 -> uvec3, 3 random values per cell ---

uvec3 murmurHash33(uvec3 src) {
    const uint M = 0x5bd1e995u;
    uvec3 h = uvec3(1190494759u, 2147483647u, 3559788179u);
    src *= M; src ^= src >> 24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y; h *= M; h ^= src.z;
    h ^= h >> 13u; h *= M; h ^= h >> 15u;
    return h;
}

vec3 hash(vec3 src) {
    uvec3 h = murmurHash33(floatBitsToUint(src));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

// --- Cell point lookup: decorrelate neighbors via (ivec2 - 1) * 4 ---

vec3 get_point(vec2 pos) {
    ivec2 p = (ivec2(pos) - 1) * 4;
    vec3 rng = hash(vec3(float(p.x), float(p.y), float(p.x + p.y)));
    return rng.xyz;
}

// --- SDF primitives ---

float sdBox(vec2 p, vec2 rad) {
    p = abs(p) - rad;
    return max(p.x, p.y);
}

// --- Evaluate squarenoi: second-closest SDF distance over 5x5 neighborhood ---

float sdVoronoi(vec2 pos) {
    pos -= 0.5;
    vec2 p = fract(pos);
    vec3 rng = get_point(pos);

    // Initialize from center cell
    vec2 nearest = rng.xy;
    vec2 lastnearest = vec2(100.0);
    vec2 current = vec2(0.0);
    float nearestRng = rng.z;
    float lastrng = rng.z;

    float phase = rng.z * TAU;
    vec2 rad = vec2(baseSize + cos(time + phase) * breathe,
                    baseSize + sin(time + phase) * breathe);

    vec3 dists = vec3(
        sdBox(p - nearest, rad),
        sdBox(p - lastnearest, rad),
        sdBox(p - current, rad));

    // 24 neighbors (5x5 minus center)
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            if (x == 0 && y == 0) continue;

            current = vec2(float(x), float(y));
            rng = get_point(pos + current);
            current += rng.xy;

            phase = rng.z * TAU;
            rad = vec2(baseSize + cos(time + phase) * breathe,
                       baseSize + sin(time + phase) * breathe);

            dists.z = sdBox(p - current, rad);

            if (dists.z < dists.y) {
                dists.yz = dists.zy;
                lastnearest = current;
                lastrng = rng.z;
            }

            if (dists.y < dists.x) {
                dists.xy = dists.yx;
                lastnearest = nearest;
                lastrng = nearestRng;
                nearest = current;
                nearestRng = rng.z;
            }
        }
    }

    // Re-evaluate second-closest cell SDF
    phase = lastrng * TAU;
    rad = vec2(baseSize + cos(time + phase) * breathe,
               baseSize + sin(time + phase) * breathe);
    return sdBox(p - lastnearest, rad);
}

void main() {
    // Small offset hides tiling artifacts at grid edges
    vec2 uv = fragTexCoord + 0.001;

    vec2 scaledUV = (uv - 0.5) * tileScale;

    // Evaluate squarenoi distance field
    float sdField = sdVoronoi(scaledUV);

    // Dual layer: second grid at spatial offset, sum distance fields
    if (dualLayer == 1) {
        sdField += sdVoronoi(scaledUV + vec2(layerOffset));
    }

    // Gradient via central differences for warp direction
    float eps = 0.5 / tileScale;
    float dx = sdVoronoi(scaledUV + vec2(eps, 0.0)) - sdVoronoi(scaledUV - vec2(eps, 0.0));
    float dy = sdVoronoi(scaledUV + vec2(0.0, eps)) - sdVoronoi(scaledUV - vec2(0.0, eps));

    // Dual layer gradient must match dual layer field
    if (dualLayer == 1) {
        vec2 scaledUV2 = scaledUV + vec2(layerOffset);
        dx += sdVoronoi(scaledUV2 + vec2(eps, 0.0)) - sdVoronoi(scaledUV2 - vec2(eps, 0.0));
        dy += sdVoronoi(scaledUV2 + vec2(0.0, eps)) - sdVoronoi(scaledUV2 - vec2(0.0, eps));
    }

    vec2 gradient = vec2(dx, dy);
    float gradLen = length(gradient);

    // Displace UV perpendicular to trace edges
    vec2 displaceUV = uv;
    if (gradLen > 0.001) {
        float warp = (contourFreq > 0.0)
            ? cos(contourFreq * sdField)
            : sdField;
        displaceUV += strength * warp * normalize(gradient);
    }

    // Clamp to valid texture range
    displaceUV = clamp(displaceUV, 0.0, 1.0);

    finalColor = texture(texture0, displaceUV);
}
