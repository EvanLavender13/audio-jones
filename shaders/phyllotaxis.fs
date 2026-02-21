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

uniform int mode;       // Active effect mode (0-8)
uniform float intensity; // Effect strength (0.0-1.0)
uniform bool smoothMode;

uniform float spinOffset;

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const float SMOOTH_FALLOFF = 6.0;  // Controls blob roundness (4-8 typical)

vec2 seedPosition(float i, float s, float angle, float spin) {
    float r = s * sqrt(i);
    float theta = i * angle + spin;  // Global rotation
    return r * vec2(cos(theta), sin(theta));
}

void main()
{
    // Early out if no effects active
    if (intensity <= 0.0) {
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

    // Smooth mode accumulators
    float smoothAccum = 0.0;
    vec2 smoothWeightedR = vec2(0.0);

    for (int i = startIdx; i <= endIdx; i++) {
        vec2 seedPos = seedPosition(float(i), scale, divergenceAngle, spinOffset);
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

        // Smooth mode: accumulate inverse distance powers
        if (smoothMode) {
            float d2 = max(dot(delta, delta), 1e-4);
            float w = 1.0 / pow(d2, SMOOTH_FALLOFF);
            smoothAccum += w;
            smoothWeightedR += delta * w;
        }
    }

    // Smooth mode: blend toward weighted center
    vec2 smoothToNearest = smoothMode ? smoothWeightedR / smoothAccum : toNearest;
    float smoothDist = smoothMode ? pow(1.0 / smoothAccum, 0.5 / SMOOTH_FALLOFF) : nearestDist;

    // Approximate edge distance: halfway between nearest and second-nearest
    float edgeDist = (secondDist - nearestDist) * 0.5;
    float centerDist = smoothMode ? smoothDist : nearestDist;

    // Border vector: direction toward cell boundary (midpoint between seeds)
    vec2 toBorder = (secondNearestPos - nearestPos) * 0.5;
    float toBorderLen = length(toBorder);
    toBorder = (toBorderLen > 0.0001) ? (toBorder / toBorderLen) * edgeDist : vec2(0.0);

    // Per-cell phase pulse (ripples outward by seed index)
    float cellPhase = phaseTime + nearestIndex * 0.1;
    float pulse = 0.5 + 0.5 * sin(cellPhase);

    // Sample cell color from texture at seed center UV
    vec2 seedUV = nearestPos / vec2(aspect, 1.0) + 0.5;
    vec3 cellColor = texture(texture0, seedUV).rgb;

    // Edge falloff based on cellRadius
    float edgeFalloff = cellRadius * scale;

    vec2 finalUV = uv;

    // UV distort modes apply before the texture sample
    if (mode == 0) {
        // UV Distortion: displace toward cell center
        // Pulse modulates displacement for breathing effect
        float pulseMod = 0.5 + 0.5 * pulse;
        vec2 displacement = smoothToNearest * intensity * 0.5 * pulseMod;
        displacement /= vec2(aspect, 1.0);
        finalUV = finalUV + displacement;
    } else if (mode == 1) {
        // Organic Flow: UV distort with edge mask for fluid look
        // Pulse creates wave-like flow motion
        float flowMask = smoothstep(0.0, edgeFalloff, length(smoothToNearest));
        float flowPulse = 0.7 + 0.3 * sin(cellPhase * 0.5);
        vec2 displacement = smoothToNearest * intensity * 0.5 * flowMask * flowPulse;
        displacement /= vec2(aspect, 1.0);
        finalUV = finalUV + displacement;
    }

    // Sample base color from (potentially distorted) UV
    vec3 color = texture(texture0, finalUV).rgb;

    // Post-sample modes
    if (mode == 2) {
        // Edge Iso Rings: iso lines radiating from cell borders
        // Pulse animates ring expansion outward
        float ringPhase = edgeDist * isoFrequency / scale + pulse * 2.0;
        float rings;
        if (smoothMode) {
            // Anti-aliased using screen-space derivatives
            float fw = fwidth(ringPhase) * 2.0;
            rings = smoothstep(fw, 0.0, abs(fract(ringPhase) - 0.5) - 0.25);
        } else {
            rings = abs(sin(ringPhase * TWO_PI * 0.5));
        }
        vec3 isoColor = rings * cellColor;
        color = mix(color, isoColor, intensity);
    } else if (mode == 3) {
        // Center Iso Rings: concentric rings from seed centers
        // Pulse animates ring expansion outward
        float ringPhase = centerDist * isoFrequency / scale - pulse * 2.0;
        float rings;
        if (smoothMode) {
            // Anti-aliased using screen-space derivatives
            float fw = fwidth(ringPhase) * 2.0;
            rings = smoothstep(fw, 0.0, abs(fract(ringPhase) - 0.5) - 0.25);
        } else {
            rings = abs(sin(ringPhase * TWO_PI * 0.5));
        }
        vec3 isoColor = rings * cellColor;
        color = mix(color, isoColor, intensity);
    } else if (mode == 4) {
        // Flat Fill: solid cells with dark edge outlines
        // Pulse modulates edge darkness for pulsing borders
        float edgeWidth = 0.015 * scale;
        float interior = smoothstep(0.0, edgeWidth, edgeDist);
        float edgeDark = 0.1 + 0.1 * pulse;
        vec3 edgeColor = cellColor * edgeDark;
        vec3 fillColor = mix(edgeColor, cellColor, interior);
        color = mix(color, fillColor, intensity);
    } else if (mode == 5) {
        // Edge Glow: black cells with colored edges from original texture
        // Pulse modulates edge brightness
        float edge = 1.0 - smoothstep(0.0, edgeFalloff, edgeDist);
        float edgePulse = 0.8 + 0.2 * pulse;
        vec3 edgeOnly = edge * texture(texture0, fragTexCoord).rgb * edgePulse;
        color = mix(color, edgeOnly, intensity);
    } else if (mode == 6) {
        // Ratio: edge/center distance ratio shading
        // Reveals spiral arm structure
        float ratio = edgeDist / (centerDist + 0.001);
        vec3 ratioColor = ratio * cellColor;
        color = mix(color, ratioColor, intensity);
    } else if (mode == 7) {
        // Determinant: 2D cross product of border and center vectors
        // High where vectors are perpendicular, reveals cell structure
        float centerLen = length(toNearest);
        if (toBorderLen > 0.0001 && centerLen > 0.0001) {
            vec2 normBorder = toBorder / toBorderLen;
            vec2 normCenter = toNearest / centerLen;
            float det = abs(normBorder.x * normCenter.y - normBorder.y * normCenter.x);
            vec3 detColor = det * cellColor;
            color = mix(color, detColor, intensity);
        }
    } else if (mode == 8) {
        // Edge Detect: highlight cell interiors where center is closer than edge
        // Creates blob-like patches in cell centers
        float edge = smoothstep(0.0, edgeFalloff, edgeDist - centerDist);
        vec3 detectColor = edge * cellColor;
        color = mix(color, detectColor, intensity);
    }

    finalColor = vec4(color, 1.0);
}
