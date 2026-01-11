#version 330

// NeonGlow: Sobel edge detection with colored glow and additive blending
// Creates cyberpunk/Tron wireframe aesthetics

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec3 glowColor;
uniform float edgeThreshold;
uniform float edgePower;
uniform float glowIntensity;
uniform float glowRadius;
uniform int glowSamples;
uniform float originalVisibility;

out vec4 finalColor;

float getLuminance(vec3 color)
{
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

float sobelEdge(vec2 uv, vec2 texelSize)
{
    float n[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            vec2 offset = vec2(float(i - 1), float(j - 1)) * texelSize;
            n[i * 3 + j] = getLuminance(texture(texture0, uv + offset).rgb);
        }
    }
    // Horizontal: [-1 0 1; -2 0 2; -1 0 1]
    // Vertical:   [-1 -2 -1; 0 0 0; 1 2 1]
    float gx = -n[0] - 2.0*n[3] - n[6] + n[2] + 2.0*n[5] + n[8];
    float gy = -n[0] - 2.0*n[1] - n[2] + n[6] + 2.0*n[7] + n[8];
    return sqrt(gx*gx + gy*gy);
}

float glowEdge(vec2 uv, vec2 texelSize, float radius, int samples)
{
    float total = 0.0;
    float weight = 0.0;
    for (int i = -samples; i <= samples; i++) {
        float w = 1.0 - abs(float(i)) / float(samples + 1);
        // Horizontal
        total += sobelEdge(uv + vec2(float(i) * radius, 0.0) * texelSize, texelSize) * w;
        // Vertical
        total += sobelEdge(uv + vec2(0.0, float(i) * radius) * texelSize, texelSize) * w;
        weight += w * 2.0;
    }
    return total / weight;
}

float shapeEdge(float edge, float threshold, float power)
{
    float shaped = max(edge - threshold, 0.0);
    return pow(shaped, power);
}

vec3 tonemap(vec3 color)
{
    return 1.0 - exp(-color);
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 texelSize = 1.0 / resolution;
    vec4 original = texture(texture0, uv);

    // Get edge with optional glow spread
    float edge = (glowRadius > 0.0)
        ? glowEdge(uv, texelSize, glowRadius, glowSamples)
        : sobelEdge(uv, texelSize);

    // Shape and color
    edge = shapeEdge(edge, edgeThreshold, edgePower);
    vec3 glow = edge * glowColor * glowIntensity;
    glow = tonemap(glow);

    // Blend with original (additive)
    vec3 base = original.rgb * originalVisibility;
    vec3 result = base + glow;

    finalColor = vec4(result, original.a);
}
