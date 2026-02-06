#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float zoomDepth;        // Zoom range in powers of 2 (1.0=2x, 2.0=4x, 3.0=8x)
uniform int layers;             // Layer count (2-8)
uniform float spiralAngle;      // Uniform rotation per zoom cycle (radians)
uniform float spiralTwist;      // Radius-dependent twist via log(r) (radians)
uniform float layerRotate;      // Fixed rotation per layer index (radians)
uniform vec2 resolution;        // Viewport size for aspect correction

const float TWO_PI = 6.28318530718;

void main()
{
    vec2 center = vec2(0.5);

    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;
    float L = float(layers);

    for (int i = 0; i < layers; i++) {
        // Get UV relative to center
        vec2 uv = fragTexCoord - center;

        // Phase: where this layer sits in zoom cycle [0,1)
        float phase = fract((float(i) - time) / L);

        // Scale: exponential zoom factor
        float scale = exp2(phase * zoomDepth);

        // Alpha: cosine fade with scale weighting (farther layers contribute less)
        float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5 / scale;

        // Early-out on negligible contribution
        if (alpha < 0.001) continue;

        // Correct to square-pixel space so rotations preserve circles
        float aspect = resolution.x / resolution.y;
        uv.x *= aspect;

        // Fixed per-layer rotation stagger (layerRotate)
        if (layerRotate != 0.0) {
            float lr = float(i) * layerRotate;
            float cosL = cos(lr);
            float sinL = sin(lr);
            uv = vec2(uv.x * cosL - uv.y * sinL, uv.x * sinL + uv.y * cosL);
        }

        // Uniform spiral rotation based on phase (spiralAngle)
        if (spiralAngle != 0.0) {
            float angle = phase * spiralAngle;
            float cosA = cos(angle);
            float sinA = sin(angle);
            uv = vec2(uv.x * cosA - uv.y * sinA, uv.x * sinA + uv.y * cosA);
        }

        // Apply scale (zoom out)
        uv = uv / scale;

        // Radius-dependent twist - MUST be after scale
        if (spiralTwist != 0.0) {
            float logR = log(length(uv) + 0.001);
            float twistAngle = logR * spiralTwist;
            float cosT = cos(twistAngle);
            float sinT = sin(twistAngle);
            uv = vec2(uv.x * cosT - uv.y * sinT, uv.x * sinT + uv.y * cosT);
        }

        // Back to texture space
        uv.x /= aspect;

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
