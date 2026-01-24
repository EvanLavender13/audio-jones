#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int radius;
uniform int quality;
uniform float sharpness;
uniform float hardness;

void main()
{
    vec2 texelSize = 1.0 / resolution;
    vec2 uv = fragTexCoord;

    if (quality == 0) {
        // Basic mode: 4 overlapping square sectors
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

        // Output sector with minimum variance
        int minIdx = 0;
        float minVar = variance[0];
        for (int i = 1; i < 4; i++) {
            if (variance[i] < minVar) {
                minVar = variance[i];
                minIdx = i;
            }
        }
        finalColor = vec4(mean[minIdx], 1.0);

    } else {
        // Generalized mode: 8 circular sectors with Gaussian weighting
        vec3 sectorMean[8];
        float sectorVariance[8];

        // Precompute sector directions (45-degree intervals)
        vec2 sectorDir[8];
        for (int i = 0; i < 8; i++) {
            float angle = float(i) * 3.14159265 / 4.0;
            sectorDir[i] = vec2(cos(angle), sin(angle));
        }

        for (int s = 0; s < 8; s++) {
            vec3 weightedColorSum = vec3(0.0);
            vec3 weightedColorSqSum = vec3(0.0);
            float totalWeight = 0.0;

            for (int x = -radius; x <= radius; x++) {
                for (int y = -radius; y <= radius; y++) {
                    vec2 offset = vec2(float(x), float(y));
                    float dist = length(offset);
                    if (dist > float(radius)) continue;

                    // Spatial Gaussian weight
                    vec2 normOffset = offset / float(radius);
                    float g = exp(-3.125 * dot(normOffset, normOffset));

                    // Sector membership weight
                    vec2 dir = normalize(offset + 0.001);
                    float sectorWeight = pow(max(0.0, dot(dir, sectorDir[s])), sharpness);

                    float w = g * sectorWeight;
                    vec3 c = texture(texture0, uv + offset * texelSize).rgb;
                    weightedColorSum += c * w;
                    weightedColorSqSum += c * c * w;
                    totalWeight += w;
                }
            }

            if (totalWeight > 0.0) {
                sectorMean[s] = weightedColorSum / totalWeight;
                vec3 var3 = weightedColorSqSum / totalWeight - sectorMean[s] * sectorMean[s];
                sectorVariance[s] = dot(var3, vec3(0.299, 0.587, 0.114));
            } else {
                sectorMean[s] = texture(texture0, uv).rgb;
                sectorVariance[s] = 1.0;
            }
        }

        // Soft inverse-variance blending
        vec3 result = vec3(0.0);
        float weightSum = 0.0;
        for (int s = 0; s < 8; s++) {
            float w = 1.0 / (1.0 + pow(sectorVariance[s] * 1000.0, 0.5 * hardness));
            result += sectorMean[s] * w;
            weightSum += w;
        }
        finalColor = vec4(result / weightSum, 1.0);
    }
}
