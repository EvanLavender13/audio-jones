#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float subdivision;
uniform float stagger;
uniform float offsetScale;
uniform float rotationScale;
uniform float zoomScale;
uniform int tessellation;
uniform float waveTime;
uniform float waveShape;
uniform float borderBlend;
uniform float spatialBias;

vec3 hash3(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

mat2 rot2(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

float shapedWave(float t, float shape) {
    float s = sin(t);
    float k = mix(1.0, 0.1, shape);
    return smoothstep(-k, k, s) * 2.0 - 1.0;
}

float sabs(float x, float c) {
    return sqrt(x * x + c);
}

vec3 computeSpatial(vec2 center) {
    return vec3(
        fract(sabs(center.x * center.y, 0.1) * 3.7),
        fract(sabs(center.x + center.y, 0.2) * 2.3),
        fract(sabs(center.x - center.y, 0.15) * 3.1)
    );
}

// --- Tessellation functions ---
// All produce: id (cell identifier for hashing), cellUV (local coord in grid-scaled space),
// cellCenter (cell center in grid space for reconstruction)
// Invariant: cellCenter + cellUV / subdivision == p (original grid-space point)

void tessRect(vec2 p, float sub, out vec2 id, out vec2 cellUV, out vec2 cellCenter) {
    id = floor(p * sub);
    cellUV = fract(p * sub) - 0.5;
    cellCenter = (id + 0.5) / sub;
}

// Dual offset grid, pick nearest hex center
void tessHex(vec2 p, float sub, out vec2 id, out vec2 cellUV, out vec2 cellCenter) {
    vec2 s = p * sub;
    vec2 r = vec2(1.0, 1.732);
    vec2 h = r * 0.5;
    vec2 a = mod(s, r) - h;
    vec2 b = mod(s - h, r) - h;

    vec2 gv = dot(a, a) < dot(b, b) ? a : b;
    vec2 rawCenter = s - gv;
    cellCenter = rawCenter / sub;
    cellUV = gv;
    // Snap id to nearest hex center for stable hashing
    // (hex centers sit on a 0.5-spaced x grid and 0.866-spaced y grid)
    id = vec2(round(rawCenter.x * 2.0) / 2.0,
              round(rawCenter.y / 0.866) * 0.866);
}

// Row-staggered grid with up/down triangle detection
void tessTri(vec2 p, float sub, out vec2 id, out vec2 cellUV, out vec2 cellCenter) {
    vec2 s = p * sub;
    float sq3 = 1.732;
    float rowF = s.y / (sq3 * 0.5);
    float row = floor(rowF);
    float oddRow = mod(row, 2.0);
    float col = floor(s.x + oddRow * 0.5);

    vec2 center = vec2(col - oddRow * 0.5 + 0.5, (row + 0.5) * sq3 * 0.5);
    vec2 local = s - center;

    float fy = fract(rowF);
    float fx = fract(s.x + oddRow * 0.5);
    float triId = (fx + fy > 1.0) ? 1.0 : 0.0;

    id = vec2(col * 2.0 + triId, row);
    cellUV = local;
    cellCenter = center / sub;
}

vec2 computeTileWarp(vec2 tileId, vec2 tileCellUV, vec2 tileCellCenter, float sub) {
    vec3 h = mix(hash3(tileId), computeSpatial(tileCellCenter), spatialBias);
    float phase = h.x * 6.283;
    vec2 offset = (h.xy - 0.5) * stagger * offsetScale
                * shapedWave(waveTime + phase, waveShape);
    float angle = (h.z - 0.5) * stagger * rotationScale
                * shapedWave(waveTime * 1.3 + phase, waveShape);
    float zoom = max(0.2, 1.0 + (h.y - 0.5) * stagger * zoomScale
                * shapedWave(waveTime * 0.7 + phase, waveShape));
    return tileCellCenter + rot2(angle) * (tileCellUV / (sub * zoom)) + offset;
}

void main() {
    vec2 uv = fragTexCoord;
    float aspect = resolution.x / resolution.y;

    // Pass through at zero subdivision
    if (subdivision < 0.01) {
        finalColor = texture(texture0, uv);
        return;
    }

    // Center and aspect-correct into grid space
    vec2 p = vec2((uv.x - 0.5) * aspect, uv.y - 0.5);

    vec2 id, cellUV, cellCenter;
    if (tessellation == 1) {
        tessHex(p, subdivision, id, cellUV, cellCenter);
    } else if (tessellation == 2) {
        tessTri(p, subdivision, id, cellUV, cellCenter);
    } else {
        tessRect(p, subdivision, id, cellUV, cellCenter);
    }

    // Per-cell warp via extracted helper
    vec2 sampleP = computeTileWarp(id, cellUV, cellCenter, subdivision);

    // Back to UV space (undo aspect correction)
    vec2 currentUV = vec2(sampleP.x / aspect + 0.5, sampleP.y + 0.5);

    // Mirror-wrap to avoid line artifacts when UVs leave [0,1]
    vec2 currentMirrorUV = 1.0 - abs(mod(currentUV, 2.0) - 1.0);

    // Edge distance for border blending
    float edgeDist;
    vec2 edgeDir;
    if (tessellation == 0) {
        vec2 ac = abs(cellUV);
        edgeDist = 0.5 - max(ac.x, ac.y);
        edgeDir = (ac.x > ac.y) ? vec2(sign(cellUV.x), 0.0) : vec2(0.0, sign(cellUV.y));
    } else {
        edgeDist = 0.5 - length(cellUV);
        edgeDir = (length(cellUV) > 0.001) ? normalize(cellUV) : vec2(1.0, 0.0);
    }

    float blendWidth = borderBlend * 0.4;
    float blendFactor = smoothstep(0.0, blendWidth + 0.001, edgeDist);

    vec4 currentSample = texture(texture0, currentMirrorUV);

    if (borderBlend > 0.001 && blendFactor < 0.999) {
        vec2 neighborP = p + edgeDir / subdivision;

        vec2 id2, cellUV2, cellCenter2;
        if (tessellation == 1) tessHex(neighborP, subdivision, id2, cellUV2, cellCenter2);
        else if (tessellation == 2) tessTri(neighborP, subdivision, id2, cellUV2, cellCenter2);
        else tessRect(neighborP, subdivision, id2, cellUV2, cellCenter2);

        vec2 neighborWarpedP = computeTileWarp(id2, cellUV2, cellCenter2, subdivision);
        vec2 neighborUV = vec2(neighborWarpedP.x / aspect + 0.5, neighborWarpedP.y + 0.5);
        vec2 neighborMirror = 1.0 - abs(mod(neighborUV, 2.0) - 1.0);
        vec4 neighborSample = texture(texture0, neighborMirror);

        finalColor = mix(neighborSample, currentSample, blendFactor);
    } else {
        finalColor = currentSample;
    }
}
