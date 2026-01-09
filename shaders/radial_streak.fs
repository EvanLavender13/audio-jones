#version 330

// Radial Blur / Zoom Blur
// Samples along radial lines toward center, creating motion blur streaks

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform int samples;        // Sample count (8-32)
uniform float streakLength; // How far toward center to sample (0.0-1.0)

void main()
{
    vec2 center = vec2(0.5);
    vec2 uv = fragTexCoord;

    // Direction from pixel toward center
    vec2 dir = center - uv;
    vec2 step = dir * streakLength / float(samples);

    vec3 col = vec3(0.0);
    float totalWeight = 0.0;
    float weight = 1.0;

    for (int i = 0; i < samples; i++) {
        vec2 sampleUV = uv + step * float(i);

        // Sample with bounds check
        if (sampleUV.x >= 0.0 && sampleUV.x <= 1.0 &&
            sampleUV.y >= 0.0 && sampleUV.y <= 1.0) {
            col += texture(texture0, sampleUV).rgb * weight;
            totalWeight += weight;
        }

        weight *= 0.95;
    }

    finalColor = vec4(col / max(totalWeight, 0.001), 1.0);
}
