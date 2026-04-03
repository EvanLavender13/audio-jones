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
uniform float spatialBias;
uniform float flipChance;
uniform float skewScale;
uniform int propagationMode;
uniform float propagationSpeed;
uniform float propagationAngle;
uniform float propagationPhase;

vec3 hash3(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

mat2 rot2(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

mat2 shear2(float sx) {
    return mat2(1.0, 0.0, sx, 1.0);
}

float shapedWave(float t, float shape) {
    float s = sin(t);
    float k = mix(1.0, 0.1, shape);
    return smoothstep(-k, k, s) * 2.0 - 1.0;
}

vec3 computeSpatial(vec2 center, float t) {
    vec2 p = center * 3.0 + vec2(t * 0.07, t * 0.05);
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    vec3 a = hash3(i);
    vec3 b = hash3(i + vec2(1.0, 0.0));
    vec3 c = hash3(i + vec2(0.0, 1.0));
    vec3 d = hash3(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
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
    float triSign = triId > 0.5 ? 1.0 : -1.0;
    vec2 triOffset = triSign * vec2(1.0 / 6.0, sq3 / 12.0);
    cellCenter = (center + triOffset) / sub;
    cellUV = local - triOffset;
}

vec2 computeTileWarp(vec2 tileId, vec2 tileCellUV, vec2 tileCellCenter, float sub) {
    vec3 h = mix(hash3(tileId), computeSpatial(tileCellCenter, waveTime), spatialBias);
    vec3 h2 = hash3(tileId + vec2(127.1, 311.7));

    // Propagation phase offset
    float propPhase = 0.0;
    if (propagationMode == 1) {
        vec2 dir = vec2(cos(propagationAngle), sin(propagationAngle));
        propPhase = dot(tileCellCenter, dir) * propagationSpeed + propagationPhase;
    } else if (propagationMode == 2) {
        propPhase = length(tileCellCenter) * propagationSpeed + propagationPhase;
    } else if (propagationMode == 3) {
        propPhase = (abs(tileCellCenter.x) + abs(tileCellCenter.y)) * propagationSpeed + propagationPhase;
    }

    float phase = h.x * 6.283 + propPhase;

    // Per-tile flip
    vec2 flip = vec2(1.0);
    if (h2.x < flipChance) { flip.x = -1.0; }
    if (h2.y < flipChance) { flip.y = -1.0; }
    vec2 flippedUV = tileCellUV * flip;

    // Per-tile transforms
    vec2 offset = (h.xy - 0.5) * stagger * offsetScale
                * shapedWave(waveTime + phase, waveShape);
    float angle = (h.z - 0.5) * stagger * rotationScale
                * shapedWave(waveTime * 1.3 + phase, waveShape);
    float shearAmt = (h2.z - 0.5) * stagger * skewScale * sub
                   * shapedWave(waveTime * 0.9 + phase, waveShape);
    float zoom = max(0.2, 1.0 + (h.y - 0.5) * stagger * zoomScale
                * shapedWave(waveTime * 0.7 + phase, waveShape));

    return tileCellCenter + rot2(angle) * shear2(shearAmt) * (flippedUV / (sub * zoom)) + offset;
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
    vec2 mirrorUV = 1.0 - abs(mod(currentUV, 2.0) - 1.0);
    finalColor = texture(texture0, mirrorUV);
}
