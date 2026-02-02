// shaders/constellation.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;    // Source texture (passthrough for additive blend)
uniform sampler2D pointLUT;    // 1D gradient for point colors
uniform sampler2D lineLUT;     // 1D gradient for line colors
uniform vec2 resolution;
uniform float time;

// Parameters
uniform float gridScale;
uniform float animSpeed;
uniform float wanderAmp;
uniform float radialFreq;
uniform float radialAmp;
uniform float radialSpeed;
uniform float glowScale;
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
    vec2 n = hash * (time * animSpeed);
    float radial = sin(length(cellID + cellOffset) * radialFreq - time * radialSpeed) * radialAmp;
    return cellOffset + sin(n + vec2(radial)) * wanderAmp;
}

// Signed distance to line segment
float DistLine(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float t = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * t);
}

// Line rendering with color
vec4 Line(vec2 p, vec2 a, vec2 b, float lineLen, vec2 cellIDA) {
    float dist = DistLine(p, a, b);
    float alpha = smoothstep(lineThickness, lineThickness * 0.2, dist);
    alpha *= smoothstep(maxLineLen, maxLineLen * 0.5, lineLen);
    alpha *= lineOpacity;

    vec3 col;
    if (interpolateLineColor != 0) {
        vec3 colA = textureLod(pointLUT, vec2(N21(a + cellIDA), 0.5), 0.0).rgb;
        vec3 colB = textureLod(pointLUT, vec2(N21(b + cellIDA), 0.5), 0.0).rgb;
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
    vec2 delta = pointPos - p;
    float glow = 1.0 / dot(delta * glowScale, delta * glowScale);
    glow = clamp(glow, 0.0, 1.0);

    float lutPos = N21(cellID);
    vec3 col = textureLod(pointLUT, vec2(lutPos, 0.5), 0.0).rgb;

    return col * glow * pointBrightness;
}

// Single layer of constellation
vec3 Layer(vec2 uv) {
    vec3 result = vec3(0.0);

    vec2 cellCoord = uv * gridScale;
    vec2 gv = fract(cellCoord) - 0.5;
    vec2 id = floor(cellCoord);

    // Gather 9 neighbor positions
    vec2 points[9];
    int idx = 0;
    for (float y = -1.0; y <= 1.0; y++) {
        for (float x = -1.0; x <= 1.0; x++) {
            points[idx++] = GetPos(id, vec2(x, y));
        }
    }

    // Center point (index 4) connects to all neighbors
    for (int i = 0; i < 9; i++) {
        if (i == 4) continue;
        float lineLen = length(points[4] - points[i]);
        vec4 line = Line(gv, points[4], points[i], lineLen, id);
        result += line.rgb * line.a;
    }

    // Corner-to-corner edges (indices: 1-3, 1-5, 7-3, 7-5)
    float len13 = length(points[1] - points[3]);
    vec4 line13 = Line(gv, points[1], points[3], len13, id);
    result += line13.rgb * line13.a;

    float len15 = length(points[1] - points[5]);
    vec4 line15 = Line(gv, points[1], points[5], len15, id);
    result += line15.rgb * line15.a;

    float len73 = length(points[7] - points[3]);
    vec4 line73 = Line(gv, points[7], points[3], len73, id);
    result += line73.rgb * line73.a;

    float len75 = length(points[7] - points[5]);
    vec4 line75 = Line(gv, points[7], points[5], len75, id);
    result += line75.rgb * line75.a;

    // Render all 9 points
    for (int i = 0; i < 9; i++) {
        vec2 cellID = id + vec2(float(i % 3) - 1.0, float(i / 3) - 1.0);
        result += Point(gv, points[i], cellID);
    }

    return result;
}

void main() {
    vec2 uv = fragTexCoord;
    uv.x *= resolution.x / resolution.y; // Aspect correction

    vec3 constellation = Layer(uv);

    // Additive blend with source
    vec3 source = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(source + constellation, 1.0);
}
