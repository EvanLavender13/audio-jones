#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float zoomDepth;        // Zoom range in powers of 2 (1.0=2x, 2.0=4x, 3.0=8x)
uniform vec2 focalOffset;       // Lissajous center offset (UV units)
uniform int layers;             // Layer count (2-8)
uniform float spiralAngle;      // Uniform rotation per zoom cycle (radians)
uniform float spiralTwist;      // Radius-dependent twist via log(r) (radians)
uniform float drosteIntensity;  // Blend: 0=standard layered, 1=full Droste spiral
uniform float drosteScale;      // Zoom ratio per spiral revolution (2.0-256.0)

const float TWO_PI = 6.28318530718;

// Droste log-polar spiral transformation
// Maps UV to recursive self-similar spiral pattern
vec3 sampleDroste(vec2 uv, vec2 center)
{
    vec2 p = uv - center;
    float r = length(p) + 0.0001;
    float theta = atan(p.y, p.x);

    // Log-polar coordinates
    float logR = log(r);
    float period = log(drosteScale);

    // Spiral shear: rotation causes zoom progression
    logR += theta / TWO_PI * period;

    // Tile in log space for infinite repetition
    logR = mod(logR + period * 100.0, period);

    // Back to linear radius, normalized to [0, 0.5]
    float newR = exp(logR);
    newR = (newR - 1.0) / (drosteScale - 1.0 + 0.0001) * 0.5;

    // Animation via time-based rotation
    theta += time * 0.3;

    // Apply spiral twist if enabled
    if (spiralTwist != 0.0) {
        theta += log(newR + 0.001) * spiralTwist;
    }

    vec2 newUV = vec2(cos(theta), sin(theta)) * newR + center;
    return texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
}

// Standard layered zoom effect
vec3 sampleLayeredZoom(vec2 center)
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;
    float L = float(layers);

    for (int i = 0; i < layers; i++) {
        // Phase: where this layer sits in zoom cycle [0,1)
        float phase = fract((float(i) - time) / L);

        // Scale: exponential zoom factor (zoomDepth controls range, not layer count)
        float scale = exp2(phase * zoomDepth);

        // Alpha: cosine fade with scale weighting (farther layers contribute less)
        float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5 / scale;

        // Early-out on negligible contribution
        if (alpha < 0.001) continue;

        // Transform UVs relative to center
        vec2 uv = fragTexCoord - center;

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

    if (weightAccum > 0.0) {
        return colorAccum / weightAccum;
    }
    return vec3(0.0);
}

void main()
{
    vec2 center = vec2(0.5) + focalOffset;

    // Pure Droste mode (early-out for performance)
    if (drosteIntensity >= 1.0) {
        finalColor = vec4(sampleDroste(fragTexCoord, center), 1.0);
        return;
    }

    // Standard layered zoom
    vec3 layeredColor = sampleLayeredZoom(center);

    // Pure layered mode (early-out for performance)
    if (drosteIntensity <= 0.0) {
        finalColor = vec4(layeredColor, 1.0);
        return;
    }

    // Blend between layered and Droste
    vec3 drosteColor = sampleDroste(fragTexCoord, center);
    finalColor = vec4(mix(layeredColor, drosteColor, drosteIntensity), 1.0);
}
