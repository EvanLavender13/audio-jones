#version 330

// Pencil Sketch: Directional gradient-aligned stroke accumulation with paper texture
// Strokes appear perpendicular to image edges, following object contours like graphite hatching
// Based on Flockaroo (2016) https://www.shadertoy.com/view/MsSGD1

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int angleCount;
uniform int sampleCount;
uniform float strokeFalloff;
uniform float gradientEps;
uniform float paperStrength;
uniform float vignetteStrength;
uniform float wobbleTime;
uniform float wobbleAmount;

out vec4 finalColor;

#define PI2 6.28318530718

// Luminance extraction (equal weight RGB)
float getVal(vec2 pos)
{
    return dot(texture(texture0, pos / resolution).rgb, vec3(0.333));
}

// Central-difference gradient at position
vec2 getGrad(vec2 pos, float eps)
{
    vec2 d = vec2(eps, 0.0);
    return vec2(
        getVal(pos + d.xy) - getVal(pos - d.xy),
        getVal(pos + d.yx) - getVal(pos - d.yx)
    ) / eps / 2.0;
}

void main()
{
    // Wobble offset (CPU-accumulated time, not shader time)
    vec2 pos = fragTexCoord * resolution + wobbleAmount * sin(wobbleTime * vec2(1.0, 1.7)) * resolution.y / 400.0;

    // Scale factor for stroke sampling
    float scale = resolution.y / 400.0;

    // Stroke accumulation
    float col = 0.0;

    for (int i = 0; i < angleCount; i++)
    {
        // Angle offset by 0.8 to avoid horizontal/vertical alignment artifacts
        float ang = PI2 / float(angleCount) * (float(i) + 0.8);
        vec2 v = vec2(cos(ang), sin(ang));

        for (int j = 0; j < sampleCount; j++)
        {
            // Perpendicular offset (creates stroke width)
            vec2 dpos = v.yx * vec2(1.0, -1.0) * float(j) * scale;
            // Parallel offset (curved stroke path, j^2 creates natural taper)
            vec2 dpos2 = v.xy * float(j * j) / float(sampleCount) * 0.5 * scale;

            // Sample both positive and negative directions
            for (float s = -1.0; s <= 1.0; s += 2.0)
            {
                vec2 pos2 = pos + s * dpos + dpos2;
                vec2 g = getGrad(pos2, gradientEps);

                // Stroke visibility: gradient aligned with sample direction
                // Positive dot = gradient points along v, so stroke perpendicular to edge
                float fact = dot(g, v) - 0.5 * abs(dot(g, v.yx * vec2(1.0, -1.0)));
                fact = clamp(fact, 0.0, 0.05);

                // Distance fade (strokeFalloff controls rate)
                fact *= 1.0 - float(j) / float(sampleCount) * strokeFalloff;

                col += fact;
            }
        }
    }

    // Normalize by loop iterations
    col /= float(angleCount * 2);

    // Paper texture: subtle grid pattern via sin waves
    vec2 s = sin(pos.xy * 0.1 / sqrt(resolution.y / 400.0));
    vec3 karo = vec3(1.0);
    karo -= paperStrength * 0.5 * vec3(0.25, 0.1, 0.1) * dot(exp(-s * s * 80.0), vec2(1.0));

    // Vignette: radial cubic falloff
    float r = length(pos - resolution.xy * 0.5) / resolution.x;
    float vign = 1.0 - r * r * r * vignetteStrength;

    // Final composition: white paper darkened by strokes
    vec3 result = karo * clamp(1.0 - col * 10.0, 0.0, 1.0) * vign;

    finalColor = vec4(result, 1.0);
}
