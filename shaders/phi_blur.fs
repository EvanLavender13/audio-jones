#version 330

// Phi Blur: Golden-ratio low-discrepancy blur with rect and disc modes
// Gamma-correct accumulation preserves bright regions

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int mode;
uniform float radius;
uniform float angle;
uniform float aspectRatio;
uniform int samples;
uniform float gamma;

out vec4 finalColor;

#define PHI 1.618034
#define GOLDEN_ANGLE 2.399963
#define HALF_PI 1.570796

void main()
{
    vec2 uv = fragTexCoord;
    vec4 blur = vec4(0.0);
    float n = float(samples);

    float ca = cos(angle);
    float sa = sin(angle);
    mat2 rot = mat2(ca, -sa, sa, ca);

    for (int i = 0; i < samples; i++)
    {
        vec2 offset;
        float fi = float(i);

        if (mode == 0)
        {
            // Rect mode: PHI low-discrepancy grid
            offset = vec2(fi / n, fract(fi * PHI)) - 0.5;
            offset = rot * offset;
            offset *= vec2(radius * aspectRatio, radius) / resolution;
        }
        else
        {
            // Disc mode: golden-angle Vogel spiral
            vec2 dir = cos(fi * GOLDEN_ANGLE + vec2(0.0, HALF_PI));
            float r = sqrt(fi / n) * radius;
            offset = dir * r / resolution;
        }

        vec2 sampleUV = clamp(uv + offset, 0.0, 1.0);
        vec4 s = texture(texture0, sampleUV);
        blur += pow(s, vec4(gamma));
    }

    vec4 result = pow(blur / n, vec4(1.0 / gamma));
    finalColor = result;
}
