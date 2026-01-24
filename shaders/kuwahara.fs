#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int radius;

void main()
{
    vec2 texelSize = 1.0 / resolution;
    vec2 uv = fragTexCoord;

    // 4 overlapping square sectors — select the one with minimum variance
    vec3 mean[4];
    float variance[4];

    for (int sector = 0; sector < 4; sector++) {
        vec3 colorSum = vec3(0.0);
        vec3 colorSqSum = vec3(0.0);
        float count = 0.0;

        int xStart = (sector == 0 || sector == 2) ? -radius : 0;
        int xEnd   = (sector == 0 || sector == 2) ? 0 : radius;
        int yStart = (sector == 0 || sector == 1) ? -radius : 0;
        int yEnd   = (sector == 0 || sector == 1) ? 0 : radius;

        for (int x = xStart; x <= xEnd; x++) {
            for (int y = yStart; y <= yEnd; y++) {
                vec3 c = texture(texture0, uv + vec2(float(x), float(y)) * texelSize).rgb;
                colorSum += c;
                colorSqSum += c * c;
                count += 1.0;
            }
        }

        mean[sector] = colorSum / count;
        vec3 var3 = colorSqSum / count - mean[sector] * mean[sector];
        variance[sector] = dot(var3, vec3(0.299, 0.587, 0.114));
    }

    int minIdx = 0;
    float minVar = variance[0];
    for (int i = 1; i < 4; i++) {
        if (variance[i] < minVar) {
            minVar = variance[i];
            minIdx = i;
        }
    }
    finalColor = vec4(mean[minIdx], 1.0);
}
