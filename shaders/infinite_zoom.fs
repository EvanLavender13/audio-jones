#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float speed;        // Zoom speed (0.1-2.0)
uniform float baseScale;    // Starting scale (0.5-2.0)
uniform vec2 focalOffset;   // Lissajous center offset (UV units)
uniform int layers;         // Layer count (4-8)
uniform float spiralTurns;  // Rotation per zoom cycle in turns (0.0-4.0)
uniform vec2 resolution;    // Screen resolution for aspect correction

const float TWO_PI = 6.28318530718;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;
    float L = float(layers);

    for (int i = 0; i < layers; i++) {
        // Phase: where this layer sits in zoom cycle [0,1)
        float phase = fract((float(i) - time * speed) / L);

        // Scale: exponential zoom factor
        float scale = exp2(phase * L) * baseScale;

        // Alpha: cosine fade (zero at phase boundaries, peak at 0.5)
        float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5;

        // Early-out on negligible contribution
        if (alpha < 0.001) continue;

        // Transform UVs relative to center (0.5 + Lissajous offset)
        vec2 center = vec2(0.5) + focalOffset;
        vec2 uv = fragTexCoord - center;

        // Optional spiral rotation based on phase
        if (spiralTurns > 0.0) {
            float angle = phase * spiralTurns * TWO_PI;
            float cosA = cos(angle);
            float sinA = sin(angle);
            uv = vec2(uv.x * cosA - uv.y * sinA, uv.x * sinA + uv.y * cosA);
        }

        // Apply scale and recenter
        uv = uv / scale + center;

        // Bounds check: discard samples outside texture
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            continue;
        }

        // Edge softening: fade near boundaries
        float edgeDist = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
        float edgeFade = smoothstep(0.0, 0.1, edgeDist);

        // Sample and accumulate
        vec3 sampleColor = texture(texture0, uv).rgb;
        float weight = alpha * edgeFade;
        colorAccum += sampleColor * weight;
        weightAccum += weight;
    }

    // Normalize and output
    if (weightAccum > 0.0) {
        finalColor = vec4(colorAccum / weightAccum, 1.0);
    } else {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
