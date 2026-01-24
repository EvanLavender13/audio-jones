#version 330

// Watercolor: Gradient-flow stroke tracing (Flockaroo algorithm).
// Two trace pairs per pixel — outline strokes along edges, color wash along gradients.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int samples;
uniform float strokeStep;
uniform float washStrength;
uniform float paperScale;
uniform float paperStrength;

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

    return mix(n0, n1, f.x);
}

float noise(vec2 p)
{
    return gnoise(vec3(p, 0.0));
}

float noisePattern(vec2 p)
{
    return gnoise(vec3(p * 0.03, 0.0));
}

float fbmNoise(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += gnoise(vec3(p, 0.0)) * amplitude;
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

vec2 getGrad(vec2 pos, float delta)
{
    vec2 d = vec2(delta, 0.0);
    return vec2(
        dot((texture(texture0, (pos + d.xy) / resolution).rgb -
             texture(texture0, (pos - d.xy) / resolution).rgb), vec3(0.333)),
        dot((texture(texture0, (pos + d.yx) / resolution).rgb -
             texture(texture0, (pos - d.yx) / resolution).rgb), vec3(0.333))
    ) / delta;
}

float luminance(vec2 pos)
{
    return dot(texture(texture0, pos / resolution).rgb, vec3(0.333));
}

vec3 sampleColor(vec2 pos)
{
    return texture(texture0, pos / resolution).rgb;
}

void main()
{
    vec2 pixelPos = fragTexCoord * resolution;

    vec3 outlineAccum = vec3(0.0);
    vec3 washAccum = vec3(0.0);
    float outlineWeight = 0.0;
    float washWeight = 0.0;

    vec2 posA = pixelPos;
    vec2 posB = pixelPos;
    vec2 posC = pixelPos;
    vec2 posD = pixelPos;

    for (int i = 0; i < samples; i++)
    {
        float falloff = 1.0 - float(i) / float(samples);

        vec2 grA = getGrad(posA, 2.0);
        vec2 grB = getGrad(posB, 2.0);
        vec2 grC = getGrad(posC, 2.0);
        vec2 grD = getGrad(posD, 2.0);

        // Outlines: step perpendicular to gradient (tangent direction)
        posA += strokeStep * normalize(vec2(grA.y, -grA.x));
        posB -= strokeStep * normalize(vec2(grB.y, -grB.x));

        float edgeStrength = clamp(10.0 * length(grA), 0.0, 1.0);
        float threshold = smoothstep(0.9, 1.1, luminance(posA) * 0.9 + noisePattern(posA));
        outlineAccum += falloff * mix(vec3(1.2), vec3(threshold * 2.0), edgeStrength);
        outlineWeight += falloff;

        // Color wash: step along gradient, accumulate nearby colors
        posC += 0.25 * normalize(grC) + 0.5 * (noise(pixelPos * 0.07) - 0.5);
        posD -= 0.50 * normalize(grD) + 0.5 * (noise(pixelPos * 0.07) - 0.5);

        float w1 = 3.0 * falloff;
        float w2 = max(0.0, 4.0 * (0.7 - falloff));
        washAccum += w1 * sampleColor(posC);
        washAccum += w2 * sampleColor(posD);
        washWeight += w1 + w2;
    }

    // Flat areas accumulate 1.2, so dividing by 1.2 normalizes to ~1.0 there.
    // Floor at 0.7 prevents harsh black marks at high-contrast edges.
    vec3 outline = max(outlineAccum / (outlineWeight * 1.2), vec3(0.7));
    vec3 wash = washAccum / washWeight;

    // washStrength: 0.0 = outline strokes only, 1.0 = full wash coloring
    vec3 strokeColor = clamp(outline, 0.0, 1.5) * mix(vec3(1.0), wash, washStrength);

    // Paper texture
    float paper = fbmNoise(fragTexCoord * paperScale);
    vec3 result = strokeColor * mix(vec3(1.0), vec3(0.97, 0.97, 0.93) * (paper * 0.3 + 0.85), paperStrength);

    finalColor = vec4(result, 1.0);
}
