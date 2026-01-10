#version 330

// Toon: Cartoon posterization with Sobel edge outlines
// Noise-varied edge thickness creates organic brush-stroke appearance

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int levels;
uniform float edgeThreshold;
uniform float edgeSoftness;
uniform float thicknessVariation;
uniform float noiseScale;

out vec4 finalColor;

// Hash-based 3D noise (no texture dependency)
float hash(vec3 p)
{
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yxz + 19.19);
    return fract((p.x + p.y) * p.z);
}

float gnoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash(i + vec3(0.0, 0.0, 0.0));
    float n001 = hash(i + vec3(0.0, 0.0, 1.0));
    float n010 = hash(i + vec3(0.0, 1.0, 0.0));
    float n011 = hash(i + vec3(0.0, 1.0, 1.0));
    float n100 = hash(i + vec3(1.0, 0.0, 0.0));
    float n101 = hash(i + vec3(1.0, 0.0, 1.0));
    float n110 = hash(i + vec3(1.0, 1.0, 0.0));
    float n111 = hash(i + vec3(1.0, 1.0, 1.0));

    float n00 = mix(n000, n001, f.z);
    float n01 = mix(n010, n011, f.z);
    float n10 = mix(n100, n101, f.z);
    float n11 = mix(n110, n111, f.z);

    float n0 = mix(n00, n01, f.y);
    float n1 = mix(n10, n11, f.y);

    return mix(n0, n1, f.x) * 2.0 - 1.0;
}

void main()
{
    vec2 uv = fragTexCoord;
    vec4 color = texture(texture0, uv);

    // Posterization: quantize luminance via max-RGB
    float greyscale = max(color.r, max(color.g, color.b));
    float fLevels = float(levels);
    float lower = floor(greyscale * fLevels) / fLevels;
    float upper = ceil(greyscale * fLevels) / fLevels;
    float level = (abs(greyscale - lower) <= abs(upper - greyscale)) ? lower : upper;
    float adjustment = level / max(greyscale, 0.001);
    vec3 posterized = color.rgb * adjustment;

    // Noise-varied texel size for brush stroke effect
    float noise = gnoise(vec3(uv * noiseScale, 0.0));
    float thicknessMultiplier = 1.0 + noise * thicknessVariation;
    vec2 texel = thicknessMultiplier / resolution;

    // Sample 3x3 neighborhood for Sobel
    vec4 n[9];
    n[0] = texture(texture0, uv + vec2(-texel.x, -texel.y));
    n[1] = texture(texture0, uv + vec2(    0.0, -texel.y));
    n[2] = texture(texture0, uv + vec2( texel.x, -texel.y));
    n[3] = texture(texture0, uv + vec2(-texel.x,     0.0));
    n[4] = texture(texture0, uv);
    n[5] = texture(texture0, uv + vec2( texel.x,     0.0));
    n[6] = texture(texture0, uv + vec2(-texel.x,  texel.y));
    n[7] = texture(texture0, uv + vec2(    0.0,  texel.y));
    n[8] = texture(texture0, uv + vec2( texel.x,  texel.y));

    // Sobel horizontal and vertical gradients
    vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    // Edge magnitude from RGB channels
    float edge = length(sqrt(sobelH * sobelH + sobelV * sobelV).rgb);

    // Soft threshold to binary outline
    float outline = smoothstep(edgeThreshold - edgeSoftness, edgeThreshold + edgeSoftness, edge);

    // Mix posterized color with black outlines
    vec3 result = mix(posterized, vec3(0.0), outline);

    finalColor = vec4(result, color.a);
}
