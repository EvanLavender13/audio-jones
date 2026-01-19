#version 330

// Phyllotaxis cellular transform
// Sunflower seed spiral patterns using Vogel's model

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float scale;            // Seed spacing (0.02-0.15)
uniform float divergenceAngle;  // Angle between seeds (radians, golden angle ≈ 2.4)
uniform float phaseTime;        // Per-cell animation time
uniform float cellRadius;       // Effect region size per cell
uniform float isoFrequency;     // Ring density for center iso

// Sub-effect intensities (0.0-1.0)
uniform float uvDistortIntensity;
uniform float flatFillIntensity;
uniform float centerIsoIntensity;
uniform float edgeGlowIntensity;

out vec4 finalColor;

const float TWO_PI = 6.28318530718;

vec2 seedPosition(float i, float s, float angle) {
    float r = s * sqrt(i);
    float theta = i * angle;
    return r * vec2(cos(theta), sin(theta));
}

void main()
{
    // Early out if no effects active
    float totalIntensity = uvDistortIntensity + flatFillIntensity +
                           centerIsoIntensity + edgeGlowIntensity;
    if (totalIntensity <= 0.0) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    float aspect = resolution.x / resolution.y;
    vec2 uv = fragTexCoord;

    // Transform UV to centered coordinates (-0.5 to 0.5 range scaled by aspect)
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);

    // Auto-calculate maxSeeds from scale to cover screen corners
    int maxSeeds = int((1.5 / scale) * (1.5 / scale)) + 50;

    // Estimate starting index from radius: n ≈ (r/scale)²
    float r = length(p);
    float estimatedN = (r / scale) * (r / scale);
    int searchRadius = 20;
    int startIdx = max(0, int(estimatedN) - searchRadius);
    int endIdx = min(maxSeeds, int(estimatedN) + searchRadius);

    // Find nearest seed
    float nearestDist = 1e10;
    float nearestIndex = 0.0;
    vec2 nearestPos = vec2(0.0);

    for (int i = startIdx; i <= endIdx; i++) {
        vec2 seedPos = seedPosition(float(i), scale, divergenceAngle);
        float dist = length(p - seedPos);
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestIndex = float(i);
            nearestPos = seedPos;
        }
    }

    // Cell data
    vec2 toCenter = nearestPos - p;
    float dist = nearestDist;
    vec2 toCenterNorm = length(toCenter) > 0.001 ? normalize(toCenter) : vec2(0.0);

    // Per-cell phase pulse (ripples outward by seed index)
    float cellPhase = phaseTime + nearestIndex * 0.1;
    float pulse = 0.5 + 0.5 * sin(cellPhase);

    // Convert seed position to UV space for sampling
    vec2 seedUV = nearestPos / vec2(aspect, 1.0) + 0.5;

    vec2 finalUV = uv;

    // UV Distortion: displace toward cell center
    if (uvDistortIntensity > 0.0) {
        vec2 displacement = toCenterNorm * uvDistortIntensity * 0.1 * pulse;
        // Scale displacement from world space to UV space
        displacement /= vec2(aspect, 1.0);
        finalUV = uv + displacement;
    }

    // Sample base color
    vec3 color = texture(texture0, finalUV).rgb;

    // Sample seed color for mix effects
    vec3 seedColor = texture(texture0, seedUV).rgb;

    // Flat Fill (Stained Glass): fill cells with seed center color
    if (flatFillIntensity > 0.0) {
        float fillMask = smoothstep(cellRadius * scale, cellRadius * scale * 0.5, dist);
        color = mix(color, seedColor, fillMask * flatFillIntensity * pulse);
    }

    // Center Iso Rings: concentric rings from seed centers
    if (centerIsoIntensity > 0.0) {
        float phase = dist * isoFrequency / scale;
        float rings = abs(sin(phase * TWO_PI * 0.5));
        color = mix(color, seedColor, rings * centerIsoIntensity * pulse);
    }

    // Edge Glow: brightness at cell edges
    if (edgeGlowIntensity > 0.0) {
        float edgeness = smoothstep(cellRadius * scale * 0.3, cellRadius * scale, dist);
        color += edgeness * edgeGlowIntensity * pulse;
    }

    finalColor = vec4(color, 1.0);
}
