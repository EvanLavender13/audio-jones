// Based on "Endless Puzzle" by Bingle
// https://www.shadertoy.com/view/N3sGD4
// License: CC BY-NC-SA 3.0 Unported
// Modified: stripped camera, cellSize pulse, rainbow tint, table texture, and
// vignette. Pieces are locked, input image supplies the piece face, edge
// lighting exposed as a uniform, seed shuffles the tab/blank pattern.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float pieceCount;     // Pieces across screen height (4-40)
uniform float seed;           // Hash offset for tab/blank pattern
uniform int fillMode;         // 0 = texture sample, 1 = solid cell-center color
uniform float edgeLight;      // Edge-lighting strength (0-1)

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float sdBox(in vec2 p, in vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float opSmoothUnion(float a, float b, float k) {
    k *= 4.0;
    float h = max(k - abs(a - b), 0.0);
    return min(a, b) - h * h * 0.25 / k;
}

float opSmoothSubtraction(float a, float b, float k) {
    return -opSmoothUnion(a, -b, k);
}

float pieceSDF(vec2 cell, vec2 p) {
    float dist = sdBox(p, vec2(0.5));
    const vec2[4] dirs = vec2[](vec2(1, 0), vec2(0, 1), vec2(-1, 0), vec2(0, -1));
    for (int i = 0; i < 4; i++) {
        bool dir = hash12(cell + dirs[i] * 0.5 + vec2(seed)) > 0.5;
        if (dir == (dirs[i].y < dirs[i].x)) {
            dist = opSmoothUnion(dist, distance(p, dirs[i] * 0.65) - 0.14, 0.025);
            dist = opSmoothUnion(dist, distance(p, dirs[i] * 0.5) - 0.1, 0.025);
        } else {
            dist = opSmoothSubtraction(distance(p, dirs[i] * 0.35) - 0.15, dist, 0.02);
        }
    }
    return dist;
}

vec2 pieceSDFnorm(vec2 cell, vec2 p) {
    float e = 0.0001;
    float d = pieceSDF(cell, p);
    return vec2(d - pieceSDF(cell, p - vec2(e, 0)),
                d - pieceSDF(cell, p - vec2(0, e))) / e;
}

void main() {
    // Aspect-corrected centered world coords. Y axis spans pieceCount cells;
    // X axis spans pieceCount * (W/H) cells, so pieces stay square.
    vec2 aspect = vec2(resolution.x / resolution.y, 1.0);
    vec2 worldUV = (fragTexCoord - 0.5) * aspect * pieceCount;

    vec2 cell = floor(worldUV);

    vec3 col = vec3(0.0);

    for (int i = 0; i < 9; i++) {
        vec2 testCell = cell + vec2(float(i % 3) - 1.0, float(i / 3) - 1.0);

        vec2 local = worldUV - (testCell + 0.5);

        float dist = pieceSDF(testCell, local);
        if (dist < 0.0) {
            // No rotation, so the inverse-transformed normal IS the raw SDF
            // gradient.
            vec2 norm = pieceSDFnorm(testCell, local);
            float edge = smoothstep(-0.075, 0.0, dist);

            // Sample input texture per fillMode.
            vec3 pieceCol;
            if (fillMode == 0) {
                // Texture mode: per-pixel sample from input at this fragment's UV.
                pieceCol = texture(texture0, fragTexCoord).rgb;
            } else {
                // Solid mode: per-cell sample at the cell-center mapped back
                // to UV. All pixels in this cell share one color.
                vec2 cellCenterUV = (testCell + 0.5) / (aspect * pieceCount) + 0.5;
                pieceCol = texture(texture0, cellCenterUV).rgb;
            }

            float lit = dot(norm * edge * edge, vec2(1.0, 1.0)) * 0.5 + 0.5;
            pieceCol *= mix(0.5, lit, edgeLight);

            // Per-piece anti-alias blend. resolution.y/pieceCount = pixels per world unit.
            float blend = min(-resolution.y * dist / pieceCount, 1.0);
            col = mix(col, pieceCol, blend);
        }
    }

    finalColor = vec4(col, 1.0);
}
