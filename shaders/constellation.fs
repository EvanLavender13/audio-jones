// shaders/constellation.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D pointLUT;    // 1D gradient for point colors
uniform sampler2D lineLUT;     // 1D gradient for line colors
uniform vec2 resolution;

// Phase accumulators (accumulated on CPU with speed multipliers)
uniform float animPhase;
uniform float wavePhase;

// Parameters
uniform float gridScale;
uniform float wanderAmp;
uniform float waveFreq;
uniform float waveAmp;
uniform float pointSize;
uniform float pointBrightness;
uniform float lineThickness;
uniform float maxLineLen;
uniform float lineOpacity;
uniform int interpolateLineColor;
uniform int fillEnabled;
uniform float fillOpacity;
uniform float fillThreshold;
uniform vec2 waveCenter;

// Hash functions
float N21(vec2 p) {
    p = fract(p * vec2(233.34, 851.73));
    p += dot(p, p + 23.456);
    return fract(p.x * p.y);
}

vec2 N22(vec2 p) {
    float n = N21(p);
    return vec2(n, N21(p + n));
}

// Point position within cell
vec2 GetPos(vec2 cellID, vec2 cellOffset) {
    vec2 hash = N22(cellID + cellOffset);
    // Bounded oscillation (1.7 to 2.7) like reference, not unbounded growth
    vec2 n = hash * (sin(animPhase) * 0.5 + 2.2);
    // Radial wave propagates outward - points at same distance move in sync
    float radial = sin(length(cellID + cellOffset - waveCenter) * waveFreq - wavePhase) * waveAmp;
    return cellOffset + sin(n * 1.2 + vec2(radial)) * wanderAmp;
}

// Signed distance to line segment
float DistLine(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float t = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * t);
}

// Line rendering with color
vec4 Line(vec2 p, vec2 a, vec2 b, float lineLen, vec2 cellIDA, vec2 cellIDB) {
    float dist = DistLine(p, a, b);
    float alpha = smoothstep(lineThickness, lineThickness * 0.2, dist);
    alpha *= smoothstep(maxLineLen, maxLineLen * 0.5, lineLen);
    alpha *= lineOpacity;

    // Fade near endpoints so points stay visible (radius matches point glow)
    float fadeRadius = pointSize * 0.15;
    float distToA = length(p - a);
    float distToB = length(p - b);
    alpha *= smoothstep(0.0, fadeRadius, min(distToA, distToB));

    vec3 col;
    if (interpolateLineColor != 0) {
        vec3 colA = textureLod(pointLUT, vec2(N21(cellIDA), 0.5), 0.0).rgb;
        vec3 colB = textureLod(pointLUT, vec2(N21(cellIDB), 0.5), 0.0).rgb;
        float t = clamp(dot(p - a, b - a) / dot(b - a, b - a), 0.0, 1.0);
        col = mix(colA, colB, t);
    } else {
        float lutPos = lineLen / maxLineLen;
        col = textureLod(lineLUT, vec2(lutPos, 0.5), 0.0).rgb;
    }

    return vec4(col, alpha);
}

// Point glow rendering
vec3 Point(vec2 p, vec2 pointPos, vec2 cellID) {
    vec2 j = (pointPos - p) * (15.0 / pointSize);
    float sparkle = 1.0 / dot(j, j);
    sparkle = clamp(sparkle, 0.0, 1.0);

    float lutPos = N21(cellID);
    vec3 col = textureLod(pointLUT, vec2(lutPos, 0.5), 0.0).rgb;

    return col * sparkle * pointBrightness;
}

// Exact signed distance to triangle (iq)
float sdTriangle(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
    vec2 e0 = p1 - p0, e1 = p2 - p1, e2 = p0 - p2;
    vec2 v0 = p - p0, v1 = p - p1, v2 = p - p2;
    vec2 pq0 = v0 - e0 * clamp(dot(v0, e0) / dot(e0, e0), 0.0, 1.0);
    vec2 pq1 = v1 - e1 * clamp(dot(v1, e1) / dot(e1, e1), 0.0, 1.0);
    vec2 pq2 = v2 - e2 * clamp(dot(v2, e2) / dot(e2, e2), 0.0, 1.0);
    float s = sign(e0.x * e2.y - e0.y * e2.x);
    vec2 d = min(min(vec2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
                     vec2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),
                     vec2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));
    return -sqrt(d.x) * sign(d.y);
}

// Interpolate vertex colors via barycentric weights
vec3 BarycentricColor(vec2 p, vec2 a, vec2 b, vec2 c, vec2 idA, vec2 idB, vec2 idC) {
    float area = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    if (abs(area) < 0.0001) return vec3(0.0); // Skip degenerate (collinear) triangles
    float w0 = ((b.x - p.x) * (c.y - p.y) - (c.x - p.x) * (b.y - p.y)) / area;
    float w1 = ((c.x - p.x) * (a.y - p.y) - (a.x - p.x) * (c.y - p.y)) / area;
    float w2 = 1.0 - w0 - w1;
    vec3 colA = textureLod(pointLUT, vec2(N21(idA), 0.5), 0.0).rgb;
    vec3 colB = textureLod(pointLUT, vec2(N21(idB), 0.5), 0.0).rgb;
    vec3 colC = textureLod(pointLUT, vec2(N21(idC), 0.5), 0.0).rgb;
    return w0 * colA + w1 * colB + w2 * colC;
}

// Single layer of constellation
vec3 Layer(vec2 uv) {
    vec3 result = vec3(0.0);

    vec2 cellCoord = uv * gridScale;
    vec2 gv = fract(cellCoord) - 0.5;
    vec2 id = floor(cellCoord);

    // Gather 9 neighbor positions and cell IDs
    // Index layout: 0=(-1,-1) 1=(0,-1) 2=(1,-1) 3=(-1,0) 4=(0,0) 5=(1,0) 6=(-1,1) 7=(0,1) 8=(1,1)
    vec2 points[9];
    vec2 cellIDs[9];
    int idx = 0;
    for (float y = -1.0; y <= 1.0; y++) {
        for (float x = -1.0; x <= 1.0; x++) {
            vec2 offset = vec2(x, y);
            points[idx] = GetPos(id, offset);
            cellIDs[idx] = id + offset;
            idx++;
        }
    }

    // Center point (index 4) connects to all neighbors
    for (int i = 0; i < 9; i++) {
        if (i == 4) continue;
        float lineLen = length(points[4] - points[i]);
        vec4 line = Line(gv, points[4], points[i], lineLen, cellIDs[4], cellIDs[i]);
        result += line.rgb * line.a;
    }

    // Corner-to-corner edges: (1,3), (1,5), (7,3), (7,5)
    ivec2 corners[4] = ivec2[](ivec2(1, 3), ivec2(1, 5), ivec2(7, 3), ivec2(7, 5));
    for (int c = 0; c < 4; c++) {
        int a = corners[c].x, b = corners[c].y;
        float len = length(points[a] - points[b]);
        vec4 line = Line(gv, points[a], points[b], len, cellIDs[a], cellIDs[b]);
        result += line.rgb * line.a;
    }

    // Render all 9 points
    for (int i = 0; i < 9; i++) {
        result += Point(gv, points[i], cellIDs[i]);
    }

    // Triangle fill: test all unique triples, shade interior with barycentric color
    if (fillEnabled != 0) {
        for (int i = 0; i < 9; i++) {
            for (int j = i + 1; j < 9; j++) {
                for (int k = j + 1; k < 9; k++) {
                    float perimeter = length(points[i] - points[j])
                                    + length(points[j] - points[k])
                                    + length(points[i] - points[k]);
                    if (perimeter > fillThreshold) continue;
                    float dist = sdTriangle(gv, points[i], points[j], points[k]);
                    float alpha = smoothstep(0.0, -lineThickness, dist);
                    vec3 fillCol = BarycentricColor(gv, points[i], points[j], points[k],
                                                    cellIDs[i], cellIDs[j], cellIDs[k]);
                    result += fillCol * alpha * fillOpacity;
                }
            }
        }
    }

    return result;
}

void main() {
    // Center UV so gridScale scales from screen center, not bottom-left
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= resolution.x / resolution.y; // Aspect correction

    vec3 constellation = Layer(uv);

    finalColor = vec4(constellation, 1.0);
}
