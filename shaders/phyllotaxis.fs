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
uniform float isoFrequency;     // Ring density for iso effects

// Sub-effect intensities (0.0-1.0)
uniform float uvDistortIntensity;
uniform float organicFlowIntensity;
uniform float edgeIsoIntensity;
uniform float centerIsoIntensity;
uniform float flatFillIntensity;
uniform float edgeGlowIntensity;
uniform float ratioIntensity;
uniform float determinantIntensity;
uniform float edgeDetectIntensity;

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
    float totalIntensity = uvDistortIntensity + organicFlowIntensity +
                           edgeIsoIntensity + centerIsoIntensity +
                           flatFillIntensity + edgeGlowIntensity +
                           ratioIntensity + determinantIntensity + edgeDetectIntensity;
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
    int searchRadius = 25;
    int startIdx = max(0, int(estimatedN) - searchRadius);
    int endIdx = min(maxSeeds, int(estimatedN) + searchRadius);

    // Find nearest and second-nearest seed
    float nearestDist = 1e10;
    float secondDist = 1e10;
    float nearestIndex = 0.0;
    vec2 nearestPos = vec2(0.0);
    vec2 secondNearestPos = vec2(0.0);
    vec2 toNearest = vec2(0.0);

    for (int i = startIdx; i <= endIdx; i++) {
        vec2 seedPos = seedPosition(float(i), scale, divergenceAngle);
        vec2 delta = seedPos - p;
        float dist = length(delta);
        if (dist < nearestDist) {
            secondDist = nearestDist;
            secondNearestPos = nearestPos;
            nearestDist = dist;
            nearestIndex = float(i);
            nearestPos = seedPos;
            toNearest = delta;
        } else if (dist < secondDist) {
            secondDist = dist;
            secondNearestPos = seedPos;
        }
    }

    // Approximate edge distance: halfway between nearest and second-nearest
    float edgeDist = (secondDist - nearestDist) * 0.5;
    float centerDist = nearestDist;

    // Border vector: direction toward cell boundary (midpoint between seeds)
    vec2 toBorder = (secondNearestPos - nearestPos) * 0.5;
    toBorder = normalize(toBorder) * edgeDist;

    // Per-cell phase pulse (ripples outward by seed index)
    float cellPhase = phaseTime + nearestIndex * 0.1;
    float pulse = 0.5 + 0.5 * sin(cellPhase);

    // Sample cell color from texture at seed center UV
    vec2 seedUV = nearestPos / vec2(aspect, 1.0) + 0.5;
    vec3 cellColor = texture(texture0, seedUV).rgb;

    // Edge falloff based on cellRadius
    float edgeFalloff = cellRadius * scale;

    vec2 finalUV = uv;

    // UV Distortion: displace toward cell center
    // Pulse modulates displacement for breathing effect
    if (uvDistortIntensity > 0.0) {
        float pulseMod = 0.5 + 0.5 * pulse;
        vec2 displacement = toNearest * uvDistortIntensity * 0.5 * pulseMod;
        displacement /= vec2(aspect, 1.0);
        finalUV = finalUV + displacement;
    }

    // Organic Flow: UV distort with edge mask for fluid look
    // Pulse creates wave-like flow motion
    if (organicFlowIntensity > 0.0) {
        float flowMask = smoothstep(0.0, edgeFalloff, centerDist);
        float flowPulse = 0.7 + 0.3 * sin(cellPhase * 0.5);
        vec2 displacement = toNearest * organicFlowIntensity * 0.5 * flowMask * flowPulse;
        displacement /= vec2(aspect, 1.0);
        finalUV = finalUV + displacement;
    }

    // Sample base color from (potentially distorted) UV
    vec3 color = texture(texture0, finalUV).rgb;

    // Edge Iso Rings: iso lines radiating from cell borders
    // Pulse animates ring expansion outward
    if (edgeIsoIntensity > 0.0) {
        float ringPhase = edgeDist * isoFrequency / scale + pulse * 2.0;
        float rings = abs(sin(ringPhase * TWO_PI * 0.5));
        vec3 isoColor = rings * cellColor;
        color = mix(color, isoColor, edgeIsoIntensity);
    }

    // Center Iso Rings: concentric rings from seed centers
    // Pulse animates ring expansion outward
    if (centerIsoIntensity > 0.0) {
        float ringPhase = centerDist * isoFrequency / scale - pulse * 2.0;
        float rings = abs(sin(ringPhase * TWO_PI * 0.5));
        vec3 isoColor = rings * cellColor;
        color = mix(color, isoColor, centerIsoIntensity);
    }

    // Flat Fill: solid cells with dark edge outlines
    // Pulse modulates edge darkness for pulsing borders
    if (flatFillIntensity > 0.0) {
        float edgeWidth = 0.015 * scale;
        float interior = smoothstep(0.0, edgeWidth, edgeDist);
        float edgeDark = 0.1 + 0.1 * pulse;
        vec3 edgeColor = cellColor * edgeDark;
        vec3 fillColor = mix(edgeColor, cellColor, interior);
        color = mix(color, fillColor, flatFillIntensity);
    }

    // Edge Glow: black cells with colored edges from original texture
    // Pulse modulates edge brightness
    if (edgeGlowIntensity > 0.0) {
        float edge = 1.0 - smoothstep(0.0, edgeFalloff, edgeDist);
        float edgePulse = 0.8 + 0.2 * pulse;
        vec3 edgeOnly = edge * texture(texture0, fragTexCoord).rgb * edgePulse;
        color = mix(color, edgeOnly, edgeGlowIntensity);
    }

    // Ratio: edge/center distance ratio shading
    // Reveals spiral arm structure
    if (ratioIntensity > 0.0) {
        float ratio = edgeDist / (centerDist + 0.001);
        vec3 ratioColor = ratio * cellColor;
        color = mix(color, ratioColor, ratioIntensity);
    }

    // Determinant: 2D cross product of border and center vectors
    // High where vectors are perpendicular, reveals cell structure
    if (determinantIntensity > 0.0) {
        vec2 normBorder = normalize(toBorder);
        vec2 normCenter = normalize(toNearest);
        float det = abs(normBorder.x * normCenter.y - normBorder.y * normCenter.x);
        vec3 detColor = det * cellColor;
        color = mix(color, detColor, determinantIntensity);
    }

    // Edge Detect: highlight where edge is closer than center
    // Creates blob-like patches at cell boundaries
    if (edgeDetectIntensity > 0.0) {
        float edge = smoothstep(0.0, edgeFalloff, edgeDist - centerDist);
        vec3 detectColor = edge * cellColor;
        color = mix(color, detectColor, edgeDetectIntensity);
    }

    finalColor = vec4(color, 1.0);
}
