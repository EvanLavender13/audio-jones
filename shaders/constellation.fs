// shaders/constellation.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D pointLUT;    // 1D gradient for point colors
uniform sampler2D lineLUT;     // 1D gradient for line colors
uniform vec2 resolution;

// Phase accumulators (accumulated on CPU with speed multipliers)
uniform float animPhase;
uniform float radialPhase;

// Parameters
uniform float gridScale;
uniform float wanderAmp;
uniform float radialFreq;
uniform float radialAmp;
uniform float pointSize;
uniform float pointBrightness;
uniform float lineThickness;
uniform float maxLineLen;
uniform float lineOpacity;
uniform int interpolateLineColor;

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
    float radial = sin(length(cellID + cellOffset) * radialFreq - radialPhase) * radialAmp;
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

    return result;
}

void main() {
    // Center UV so gridScale scales from screen center, not bottom-left
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= resolution.x / resolution.y; // Aspect correction

    vec3 constellation = Layer(uv);

    finalColor = vec4(constellation, 1.0);
}
