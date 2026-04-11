#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float spatialSigma;
uniform float rangeSigma;

#define EPS 1e-5

float lum(in vec4 color) {
    return length(color.xyz);
}

void main() {
    float sigS = max(spatialSigma, EPS);
    float sigL = max(rangeSigma, EPS);

    float facS = -1.0 / (2.0 * sigS * sigS);
    float facL = -1.0 / (2.0 * sigL * sigL);

    float sumW = 0.0;
    vec4  sumC = vec4(0.0);
    int halfSize = min(int(sigS * 2.0), 12);

    float l = lum(texture(texture0, fragTexCoord));

    for (int i = -halfSize; i <= halfSize; i++) {
        for (int j = -halfSize; j <= halfSize; j++) {
            vec2 pos = vec2(float(i), float(j));
            vec4 offsetColor = texture(texture0, fragTexCoord + pos / resolution);

            float distS = length(pos);
            float distL = lum(offsetColor) - l;

            float wS = exp(facS * distS * distS);
            float wL = exp(facL * distL * distL);
            float w = wS * wL;

            sumW += w;
            sumC += offsetColor * w;
        }
    }

    finalColor = sumC / sumW;
}
