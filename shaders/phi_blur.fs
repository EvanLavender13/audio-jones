#version 330

// Phi Blur: Golden-ratio low-discrepancy blur with shaped Vogel spiral
// Gamma-correct accumulation preserves bright regions

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float radius;
uniform int samples;
uniform float gamma;
uniform int shape;
uniform float shapeAngle;
uniform int starPoints;
uniform float starInnerRadius;

out vec4 finalColor;

#define PI 3.141593
#define PHI 1.618034
#define GOLDEN_ANGLE 2.399963
#define HALF_PI 1.570796

// Regular N-gon boundary: inscribed-circle-normalized
float ngonRadius(float theta, int n, float rotation) {
    float halfAngle = PI / float(n);
    float sector = mod(theta + rotation, 2.0 * halfAngle) - halfAngle;
    return cos(halfAngle) / cos(sector);
}

// Star: linear interpolation between outer tips and inner valleys
float starRadius(float theta, int n, float innerRatio, float rotation) {
    float halfAngle = PI / float(n);
    float sector = mod(theta + rotation, 2.0 * halfAngle);
    float t = abs(sector - halfAngle) / halfAngle;
    return mix(innerRatio, 1.0, t);
}

void main()
{
    vec2 uv = fragTexCoord;
    vec4 blur = vec4(0.0);
    float n = float(samples);

    for (int i = 0; i < samples; i++)
    {
        float fi = float(i);

        // Vogel spiral sampling
        float theta = fi * GOLDEN_ANGLE;
        vec2 dir = cos(theta + vec2(0.0, HALF_PI));
        float r = sqrt(fi / n) * radius;

        // shape: 0=Disc (no-op), 1=Box, 2=Hex, 3=Star
        if (shape == 1)      r *= ngonRadius(theta, 4, shapeAngle);
        else if (shape == 2) r *= ngonRadius(theta, 6, shapeAngle);
        else if (shape == 3) r *= starRadius(theta, starPoints, starInnerRadius, shapeAngle);

        vec2 offset = dir * r / resolution;

        vec2 sampleUV = clamp(uv + offset, 0.0, 1.0);
        vec4 s = texture(texture0, sampleUV);
        blur += pow(s, vec4(gamma));
    }

    vec4 result = pow(blur / n, vec4(1.0 / gamma));
    finalColor = result;
}
