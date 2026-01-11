#version 330

// OilPaint: 4-sector Kuwahara filter for painterly effect
// Smooths flat regions while preserving hard edges

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int radius;

out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    // 4 sectors: compute mean and variance for each
    // Sector 0: top-left     Sector 1: top-right
    // Sector 2: bottom-left  Sector 3: bottom-right
    vec3 mean[4];
    float variance[4];

    for (int s = 0; s < 4; s++) {
        vec3 colorSum = vec3(0.0);
        vec3 colorSqSum = vec3(0.0);
        float count = 0.0;

        // Determine sector bounds based on index
        int xStart = (s == 0 || s == 2) ? -radius : 0;
        int xEnd   = (s == 0 || s == 2) ? 0 : radius;
        int yStart = (s == 0 || s == 1) ? -radius : 0;
        int yEnd   = (s == 0 || s == 1) ? 0 : radius;

        for (int x = xStart; x <= xEnd; x++) {
            for (int y = yStart; y <= yEnd; y++) {
                vec2 offset = vec2(float(x), float(y)) * texel;
                vec3 c = texture(texture0, uv + offset).rgb;
                colorSum += c;
                colorSqSum += c * c;
                count += 1.0;
            }
        }

        mean[s] = colorSum / count;
        vec3 v = (colorSqSum / count) - (mean[s] * mean[s]);
        variance[s] = dot(v, vec3(0.299, 0.587, 0.114));
    }

    // Find minimum variance sector
    int minIdx = 0;
    float minVar = variance[0];
    for (int s = 1; s < 4; s++) {
        if (variance[s] < minVar) {
            minVar = variance[s];
            minIdx = s;
        }
    }

    finalColor = vec4(mean[minIdx], 1.0);
}
