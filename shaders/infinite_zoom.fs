#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float speed;        // Zoom speed (0.1-2.0)
uniform float zoomDepth;    // Zoom range in powers of 2 (1.0=2x, 2.0=4x, 3.0=8x)
uniform vec2 focalOffset;   // Lissajous center offset (UV units)
uniform int layers;         // Layer count (2-8)
uniform float spiralTurns;  // Uniform rotation per zoom cycle (radians)
uniform float spiralTwist;  // Radius-dependent twist via log(r) (radians)

const float TWO_PI = 6.28318530718;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;
    float L = float(layers);

    for (int i = 0; i < layers; i++) {
        // Phase: where this layer sits in zoom cycle [0,1)
        float phase = fract((float(i) - time * speed) / L);

        // Scale: exponential zoom factor (zoomDepth controls range, not layer count)
        float scale = exp2(phase * zoomDepth);

        // Alpha: cosine fade with scale weighting (farther layers contribute less)
        float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5 / scale;

        // Early-out on negligible contribution
        if (alpha < 0.001) continue;

        // Transform UVs relative to center (0.5 + Lissajous offset)
        vec2 center = vec2(0.5) + focalOffset;
        vec2 uv = fragTexCoord - center;

        // Uniform spiral rotation based on phase (spiralTurns)
        if (spiralTurns != 0.0) {
            float angle = phase * spiralTurns;
            float cosA = cos(angle);
            float sinA = sin(angle);
            uv = vec2(uv.x * cosA - uv.y * sinA, uv.x * sinA + uv.y * cosA);
        }

        // Apply scale (zoom out)
        uv = uv / scale;

        // Radius-dependent twist - MUST be after scale
        // log(length(uv)) now equals log(original_r) - phase*zoomDepth*LOG2
        if (spiralTwist != 0.0) {
            float logR = log(length(uv) + 0.001);
            float twistAngle = logR * spiralTwist;
            float cosT = cos(twistAngle);
            float sinT = sin(twistAngle);
            uv = vec2(uv.x * cosT - uv.y * sinT, uv.x * sinT + uv.y * cosT);
        }

        // Recenter
        uv = uv + center;

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
