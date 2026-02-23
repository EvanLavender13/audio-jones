#version 330

// NeonGlow: Dual-edge detection (Sobel core + DoG halo) with source-derived colored glow
// Creates cyberpunk/Tron wireframe aesthetics with sharp inner lines and soft outer halos

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float glowIntensity;
uniform float glowRadius;
uniform float coreSharpness;
uniform float edgeThreshold;
uniform float edgePower;
uniform float saturationBoost;
uniform float brightnessBoost;
uniform float originalVisibility;

out vec4 finalColor;

// Precomputed Gaussian kernel, sigma=1.5 on 5x5 grid
// G(dx,dy) = exp(-(dx*dx + dy*dy) / 4.5)
const float gauss5x5[25] = float[25](
    0.1691, 0.3292, 0.4111, 0.3292, 0.1691,
    0.3292, 0.6412, 0.8007, 0.6412, 0.3292,
    0.4111, 0.8007, 1.0000, 0.8007, 0.4111,
    0.3292, 0.6412, 0.8007, 0.6412, 0.3292,
    0.1691, 0.3292, 0.4111, 0.3292, 0.1691
);

float getLuminance(vec3 color)
{
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 boostColor(vec3 src, float satBoost, float brightBoost)
{
    vec3 hsv = rgb2hsv(src);
    hsv.y = min(hsv.y + satBoost, 1.0);
    hsv.z = min(hsv.z + brightBoost, 1.0);
    return hsv2rgb(hsv);
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

float shapeEdge(float edge, float threshold, float power)
{
    float shaped = max(edge - threshold, 0.0);
    return pow(shaped, power);
}

vec3 tonemap(vec3 color)
{
    return 1.0 - exp(-color);
}

// Derivative of Gaussian edge detection over 5x5 kernel
// glowRadius scales the physical distance between sample points
float dogHaloEdge(vec2 uv, vec2 texelSize, float radius) {
    float gx = 0.0;
    float gy = 0.0;
    for (int j = -2; j <= 2; j++) {
        for (int i = -2; i <= 2; i++) {
            float w = gauss5x5[(j + 2) * 5 + (i + 2)];
            vec2 offset = vec2(float(i), float(j)) * radius * texelSize;
            float lum = getLuminance(texture(texture0, uv + offset).rgb);
            gx += lum * float(i) * w;
            gy += lum * float(j) * w;
        }
    }
    return sqrt(gx * gx + gy * gy);
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 texelSize = 1.0 / resolution;
    vec4 original = texture(texture0, uv);

    // 1. Sharp core edge (9 texture samples)
    float core = sobelEdge(uv, texelSize);
    core = shapeEdge(core, edgeThreshold, edgePower);

    // 2. Smooth halo edge (25 texture samples)
    float halo = dogHaloEdge(uv, texelSize, glowRadius);
    halo = shapeEdge(halo, edgeThreshold * 0.5, edgePower);

    // 3. Combine core + halo
    float edge = core * coreSharpness + halo * (1.0 - coreSharpness);

    // 4. Source-derived color with HSV boost
    vec3 edgeColor = boostColor(original.rgb, saturationBoost, brightnessBoost);

    // 5. Apply glow intensity and tonemap
    vec3 glow = edge * edgeColor * glowIntensity;
    glow = tonemap(glow);

    // 6. Composite over original
    vec3 result = original.rgb * originalVisibility + glow;
    finalColor = vec4(result, original.a);
}
