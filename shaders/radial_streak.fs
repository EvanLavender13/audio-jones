#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform int samples;        // Sample count (8-32)
uniform float streakLength; // How far toward center to sample (0.0-1.0)
uniform float spiralTwist;  // Rotation per sample (radians) - spiral effect
uniform vec2 focalOffset;   // Lissajous center offset

vec3 deform(vec2 p, float t)
{
    vec2 q = sin(vec2(1.1, 1.2) * t + p * 3.0);
    float r = sqrt(dot(q, q));

    // Lens-like distortion
    vec2 uv = p * sqrt(1.0 + r * r);
    uv += sin(vec2(0.0, 0.6) + vec2(1.0, 1.1) * t) * 0.1;

    // Remap to 0-1 range for our texture
    uv = uv * 0.5 + 0.5;

    // Clamp to valid range
    uv = clamp(uv, 0.0, 1.0);

    return texture(texture0, uv).rgb;
}

void main()
{
    // Center with Lissajous animation applied (scale UV offset to -1 to 1 space)
    vec2 center = focalOffset * 2.0;

    // Convert to -1 to 1 range like ShaderToy
    vec2 p = (fragTexCoord - 0.5) * 2.0;

    vec3 col = vec3(0.0);
    vec2 d = (center - p) * streakLength / float(samples);
    float w = 1.0;
    vec2 s = p;

    for (int i = 0; i < samples; i++) {
        // Apply spiral rotation around center
        vec2 samplePos = s;
        if (spiralTwist != 0.0) {
            float angle = spiralTwist * float(i) / float(samples - 1);
            vec2 offset = samplePos - center;
            float c = cos(angle), sn = sin(angle);
            samplePos = center + vec2(c * offset.x - sn * offset.y,
                                      sn * offset.x + c * offset.y);
        }

        vec3 res = deform(samplePos, time);
        col += w * smoothstep(0.0, 1.0, res);
        w *= 0.98;
        s += d;
    }

    col = col * 3.5 / float(samples);

    finalColor = vec4(col, 1.0);
}
