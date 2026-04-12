// Based on XDoG isotropic shader by Jan Eric Kyprianidis
// https://github.com/jkyprian/flowabs
// License: GPL-3.0
// Modified: GLSL 330, fragTexCoord, resolution uniform, luminance input,
//   derived sigma_r, halfWidth cap, color compositing

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float sigma;
uniform float tau;
uniform float phi;

void main() {
    float sigmaR = sigma * 1.6;
    float twoSigmaESquared = 2.0 * sigma * sigma;
    float twoSigmaRSquared = 2.0 * sigmaR * sigmaR;
    int halfWidth = int(ceil(2.0 * sigmaR));
    halfWidth = min(halfWidth, 12);

    vec2 sum = vec2(0.0);
    vec2 norm = vec2(0.0);

    for (int i = -halfWidth; i <= halfWidth; i++) {
        for (int j = -halfWidth; j <= halfWidth; j++) {
            float d = length(vec2(i, j));
            vec2 kernel = vec2(exp(-d * d / twoSigmaESquared),
                               exp(-d * d / twoSigmaRSquared));

            vec4 texSample = texture(texture0,
                fragTexCoord + vec2(i, j) / resolution);
            float L = dot(texSample.rgb, vec3(0.299, 0.587, 0.114));

            norm += 2.0 * kernel;
            sum += kernel * vec2(L, L);
        }
    }
    sum /= norm;

    float H = 100.0 * (sum.x - tau * sum.y);
    float edge = (H > 0.0) ? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H);

    vec4 inputColor = texture(texture0, fragTexCoord);
    finalColor = vec4(inputColor.rgb * edge, inputColor.a);
}
