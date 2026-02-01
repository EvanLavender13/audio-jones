// shaders/circuit_board.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 patternConst;  // (patternX, patternY)
uniform int iterations;
uniform float scale;
uniform float offset;
uniform float scaleDecay;
uniform float strength;
uniform float scrollOffset;  // CPU-accumulated scroll
uniform int chromatic;

vec2 triangleWave(vec2 p, vec2 c, float s) {
    return abs(fract((p + c) * s) - 0.5);
}

vec2 computeWarp(vec2 uv, vec2 c, int iters, float s, float off, float decay) {
    for (int i = 0; i < iters; i++) {
        uv = triangleWave(uv + off, c, s) + triangleWave(uv.yx, c, s);
        s /= decay;
        off /= decay;
        uv.y = -uv.y;
    }
    return uv;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 scrollUV = uv + vec2(scrollOffset * 0.5, scrollOffset * 0.33);

    if (chromatic == 1) {
        vec3 color;
        for (int c = 0; c < 3; c++) {
            float scaleOffset = float(c) * 0.1;
            vec2 warp = computeWarp(scrollUV, patternConst, iterations,
                                    scale + scaleOffset, offset, scaleDecay);
            vec2 finalUV = clamp(uv + strength * (warp - 0.5), 0.0, 1.0);
            color[c] = texture(texture0, finalUV)[c];
        }
        finalColor = vec4(color, 1.0);
    } else {
        vec2 warp = computeWarp(scrollUV, patternConst, iterations,
                                scale, offset, scaleDecay);
        vec2 finalUV = clamp(uv + strength * (warp - 0.5), 0.0, 1.0);
        finalColor = texture(texture0, finalUV);
    }
}
