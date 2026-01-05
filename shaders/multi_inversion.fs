#version 330

// Multi-Inversion Blend
// Chains circle inversions with depth-weighted accumulation
// Animated centers follow phase-offset Lissajous paths

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform int iterations;
uniform float radius;
uniform float radiusScale;
uniform float focalAmplitude;
uniform float focalFreqX;
uniform float focalFreqY;
uniform float phaseOffset;

out vec4 finalColor;

// Circle inversion: reflects point across circle boundary
// Points inside map outside, points outside map inside
vec2 circleInvert(vec2 pos, vec2 center, float r)
{
    vec2 d = pos - center;
    float r2 = r * r;
    float denom = max(dot(d, d), 0.0001);
    return center + d * (r2 / denom);
}

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    // Map UV from [0,1] to [-1,1] centered
    vec2 z = (fragTexCoord - 0.5) * 2.0;

    for (int i = 0; i < iterations; i++) {
        // Phase-offset Lissajous: each iteration sees different point on path
        float t = time + float(i) * phaseOffset;
        vec2 center = focalAmplitude * vec2(sin(t * focalFreqX), cos(t * focalFreqY));

        // Per-iteration radius scaling
        float r = radius * pow(radiusScale, float(i));

        // Apply circle inversion
        z = circleInvert(z, center, r);

        // Smooth remap to UV space (avoids hard boundaries)
        vec2 sampleUV = 0.5 + 0.4 * sin(z * 0.5);

        // Depth-based weight: earlier iterations contribute less
        float weight = 1.0 / float(i + 2);
        colorAccum += texture(texture0, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    finalColor = vec4(colorAccum / weightAccum, 1.0);
}
