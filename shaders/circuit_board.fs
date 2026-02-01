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
uniform float scrollOffset;   // CPU-accumulated scroll distance
uniform float rotationAngle;  // CPU-accumulated scroll direction
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

    // Scroll direction rotates with accumulated angle
    vec2 scrollDir = vec2(cos(rotationAngle), sin(rotationAngle));
    vec2 scrollUV = uv + scrollDir * scrollOffset * 0.5;

    if (chromatic == 1) {
        vec3 color;
        for (int ch = 0; ch < 3; ch++) {
            float scaleOffset = float(ch) * 0.1;
            vec2 warp = computeWarp(scrollUV, patternConst, iterations,
                                    scale + scaleOffset, offset, scaleDecay);
            vec2 finalUV = clamp(uv + strength * (warp - 0.5), 0.0, 1.0);
            color[ch] = texture(texture0, finalUV)[ch];
        }
        finalColor = vec4(color, 1.0);
    } else {
        vec2 warp = computeWarp(scrollUV, patternConst, iterations,
                                scale, offset, scaleDecay);
        vec2 finalUV = clamp(uv + strength * (warp - 0.5), 0.0, 1.0);
        finalColor = texture(texture0, finalUV);
    }
}
