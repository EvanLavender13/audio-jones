#version 330

// Watercolor: Simulates watercolor painting through edge darkening (pigment pooling),
// procedural paper granulation, and soft color bleeding

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float edgeDarkening;
uniform float granulationStrength;
uniform float paperScale;
uniform float softness;
uniform float bleedStrength;
uniform float bleedRadius;
uniform int colorLevels;

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

    return mix(n0, n1, f.x);  // Returns 0-1 (not centered)
}

// Multi-octave FBM for paper texture
float paperTexture(vec2 uv, float scale)
{
    float paper = 0.0;
    float amplitude = 0.5;
    float frequency = scale;

    for (int i = 0; i < 4; i++) {
        paper += gnoise(vec3(uv * frequency, 0.0)) * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return paper;
}

float getLuminance(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

float sobelEdge(vec2 uv, vec2 texel)
{
    // Sample 3x3 neighborhood
    float n[9];
    n[0] = getLuminance(texture(texture0, uv + vec2(-texel.x, -texel.y)).rgb);
    n[1] = getLuminance(texture(texture0, uv + vec2(    0.0, -texel.y)).rgb);
    n[2] = getLuminance(texture(texture0, uv + vec2( texel.x, -texel.y)).rgb);
    n[3] = getLuminance(texture(texture0, uv + vec2(-texel.x,     0.0)).rgb);
    n[4] = getLuminance(texture(texture0, uv).rgb);
    n[5] = getLuminance(texture(texture0, uv + vec2( texel.x,     0.0)).rgb);
    n[6] = getLuminance(texture(texture0, uv + vec2(-texel.x,  texel.y)).rgb);
    n[7] = getLuminance(texture(texture0, uv + vec2(    0.0,  texel.y)).rgb);
    n[8] = getLuminance(texture(texture0, uv + vec2( texel.x,  texel.y)).rgb);

    // Sobel horizontal and vertical gradients
    float sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    float sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    return sqrt(sobelH * sobelH + sobelV * sobelV);
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;
    vec4 original = texture(texture0, uv);
    vec3 color = original.rgb;

    // 1. Optional soft blur for paint diffusion (box filter approximation)
    if (softness > 0.0) {
        vec3 blurred = vec3(0.0);
        float samples = 0.0;
        int radius = int(softness);
        for (int x = -radius; x <= radius; x++) {
            for (int y = -radius; y <= radius; y++) {
                blurred += texture(texture0, uv + vec2(x, y) * texel).rgb;
                samples += 1.0;
            }
        }
        color = blurred / samples;
    }

    // 2. Edge detection
    float edge = sobelEdge(uv, texel);

    // 3. Edge darkening (pigment pooling)
    color *= 1.0 - edge * edgeDarkening;

    // 4. Paper granulation
    float paper = paperTexture(uv, paperScale);
    // Apply more granulation in flat areas, less at edges
    float granulationMask = 1.0 - edge;
    color *= mix(1.0, paper, granulationStrength * granulationMask);

    // 5. Color bleeding at edges
    if (bleedStrength > 0.0 && edge > 0.01) {
        // Directional blur perpendicular to edge
        vec2 edgeDir = normalize(vec2(
            getLuminance(texture(texture0, uv + vec2(texel.x, 0.0)).rgb) -
            getLuminance(texture(texture0, uv - vec2(texel.x, 0.0)).rgb),
            getLuminance(texture(texture0, uv + vec2(0.0, texel.y)).rgb) -
            getLuminance(texture(texture0, uv - vec2(0.0, texel.y)).rgb)
        ) + 0.001);

        vec3 bleed = vec3(0.0);
        for (int i = -2; i <= 2; i++) {
            bleed += texture(texture0, uv + edgeDir * float(i) * bleedRadius * texel).rgb;
        }
        bleed /= 5.0;
        color = mix(color, bleed, edge * bleedStrength);
    }

    // 6. Optional color quantization
    if (colorLevels > 0) {
        float fLevels = float(colorLevels);
        color = floor(color * fLevels + 0.5) / fLevels;
    }

    finalColor = vec4(color, original.a);
}
