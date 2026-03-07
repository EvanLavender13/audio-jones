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
uniform vec2 center;            // Zoom center point
uniform vec2 offset;            // Parallax direction per layer
uniform int warpType;           // 0=None, 1=Sine, 2=Noise
uniform float warpStrength;     // Distortion amplitude
uniform float warpFreq;         // Spatial frequency
uniform float warpTime;         // Animation phase
uniform int blendMode;          // 0=WeightedAvg, 1=Additive, 2=Screen

const float TWO_PI = 6.28318530718;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;
    float L = float(layers);

    for (int i = 0; i < layers; i++) {
        // Parallax center shift per layer
        vec2 layerCenter = center + offset * float(i);

        // Get UV relative to center
        vec2 uv = fragTexCoord - layerCenter;

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

        // UV warp (after spiral twist, before aspect correction back)
        if (warpType == 1) {
            // Sine warp, attenuated by depth
            uv.x += sin(uv.y * warpFreq + warpTime) * warpStrength / scale;
            uv.y += sin(uv.x * warpFreq * 1.3 + warpTime * 0.7) * warpStrength / scale;
        } else if (warpType == 2) {
            // Noise warp via fBM, attenuated by depth
            vec2 warpOffset = vec2(
                fbm(uv * warpFreq + warpTime),
                fbm(uv * warpFreq + warpTime + vec2(5.2, 1.3))
            ) - 0.5;
            uv += warpOffset * warpStrength / scale;
        }

        // Back to texture space
        uv.x /= aspect;

        // Recenter
        uv = uv + layerCenter;

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

        if (blendMode == 0) {
            // Weighted Average (default)
            colorAccum += sampleColor * weight;
            weightAccum += weight;
        } else if (blendMode == 1) {
            // Additive
            colorAccum += sampleColor * weight;
        } else {
            // Screen
            colorAccum = 1.0 - (1.0 - colorAccum) * (1.0 - sampleColor * weight);
        }
    }

    // Finalize based on blend mode
    if (blendMode == 0) {
        finalColor = (weightAccum > 0.0)
            ? vec4(colorAccum / weightAccum, 1.0)
            : vec4(0.0, 0.0, 0.0, 1.0);
    } else if (blendMode == 1) {
        finalColor = vec4(clamp(colorAccum, 0.0, 1.0), 1.0);
    } else {
        finalColor = vec4(colorAccum, 1.0);
    }
}
