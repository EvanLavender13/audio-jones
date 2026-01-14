#version 330

// Voronoi multi-effect shader
// Computes voronoi geometry once, applies multiple blendable effects

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float scale;        // Cell count across screen width
uniform float time;         // Animation driver
uniform float edgeFalloff;  // Edge softness for multiple effects
uniform float isoFrequency; // Ring density for iso effects

// Effect intensities (0.0-1.0)
uniform float uvDistortIntensity;
uniform float edgeIsoIntensity;
uniform float centerIsoIntensity;
uniform float flatFillIntensity;
uniform float organicFlowIntensity;
uniform float edgeGlowIntensity;
uniform float determinantIntensity;
uniform float ratioIntensity;
uniform float edgeDetectIntensity;
uniform bool smoothMode;

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const float SMOOTH_FALLOFF = 6.0;  // Controls blob roundness (4-8 typical)

vec2 hash2(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

void main()
{
    // Early out if no effects active
    float totalIntensity = uvDistortIntensity + edgeIsoIntensity + centerIsoIntensity +
                           flatFillIntensity + organicFlowIntensity + edgeGlowIntensity +
                           determinantIntensity + ratioIntensity + edgeDetectIntensity;
    if (totalIntensity <= 0.0) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    float aspect = resolution.x / resolution.y;
    vec2 st = fragTexCoord * vec2(scale * aspect, scale);
    vec2 ip = floor(st);
    vec2 fp = fract(st);

    // First pass: find nearest cell center
    vec2 mr = vec2(0.0);  // Vector to nearest cell center (sharp)
    vec2 mg = vec2(0.0);  // Grid offset of nearest cell
    float md = 8.0;       // Min distance squared

    // Smooth mode accumulators
    float smoothAccum = 0.0;
    vec2 smoothWeightedR = vec2(0.0);

    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash2(ip + g);
            o = 0.5 + 0.5 * sin(time + TWO_PI * o);
            vec2 r = g + o - fp;
            float d2 = dot(r, r);

            // Track nearest cell (always needed for border calc)
            if (d2 < md) {
                md = d2;
                mr = r;
                mg = g;
            }

            // Smooth mode: accumulate inverse distance powers
            if (smoothMode) {
                float d = max(d2, 1e-4);
                float w = 1.0 / pow(d, SMOOTH_FALLOFF);
                smoothAccum += w;
                smoothWeightedR += r * w;
            }
        }
    }

    // Smooth vectors for displacement effects (bubbly), sharp mr for border calc
    vec2 smoothMr = smoothMode ? smoothWeightedR / smoothAccum : mr;
    float smoothDist = smoothMode ? pow(1.0 / smoothAccum, 0.5 / SMOOTH_FALLOFF) : length(mr);

    // Second pass: find vector to nearest cell border
    vec2 borderVec = vec2(0.0);
    float borderDist = 8.0;
    for (int j = -2; j <= 2; j++) {
        for (int i = -2; i <= 2; i++) {
            vec2 g = mg + vec2(float(i), float(j));
            vec2 o = hash2(ip + g);
            o = 0.5 + 0.5 * sin(time + TWO_PI * o);
            vec2 r = g + o - fp;
            if (dot(mr - r, mr - r) > 0.00001) {
                vec2 segmentNormal = normalize(r - mr);
                float scalarProj = dot(0.5 * (mr + r), segmentNormal);
                if (scalarProj < borderDist) {
                    borderDist = scalarProj;
                    borderVec = segmentNormal * scalarProj;
                }
            }
        }
    }

    // Center distance: smooth (bubbly) or sharp (polygon)
    float centerDist = smoothMode ? smoothDist : length(mr);
    // Border distance for edge-based effects
    float edgeDist = length(borderVec);

    // Voronoi data: xy = vector to border, zw = vector to center
    vec4 voronoiData = vec4(borderVec, mr);

    // Sample cell color from texture at cell center
    vec2 cellID = ip + mg;
    vec2 cellCenterUV = (cellID + 0.5) / vec2(scale * aspect, scale);
    vec3 cellColor = texture(texture0, cellCenterUV).rgb;

    // Start with original UV, apply distortion effects
    vec2 finalUV = fragTexCoord;

    // UV Distort - hard displacement toward cell center
    if (uvDistortIntensity > 0.0) {
        vec2 displacement = smoothMr * uvDistortIntensity * 0.5;
        displacement /= vec2(scale * aspect, scale);
        finalUV = finalUV + displacement;
    }

    // Organic Flow - smooth flowing displacement with center-falloff
    // Pixels near cell centers move more, edges move less
    if (organicFlowIntensity > 0.0) {
        float flowMask = smoothstep(0.0, edgeFalloff, length(smoothMr));
        vec2 displacement = smoothMr * organicFlowIntensity * 0.5 * flowMask;
        displacement /= vec2(scale * aspect, scale);
        finalUV = finalUV + displacement;
    }

    // Sample base color from (potentially distorted) UV
    vec3 color = texture(texture0, finalUV).rgb;

    // Edge Iso Rings - iso lines radiating from border (mix)
    if (edgeIsoIntensity > 0.0) {
        float phase = edgeDist * isoFrequency;
        float rings;
        if (smoothMode) {
            // Anti-aliased using screen-space derivatives
            float fw = fwidth(phase) * 2.0;
            rings = smoothstep(fw, 0.0, abs(fract(phase) - 0.5) - 0.25);
        } else {
            rings = abs(sin(phase * TWO_PI * 0.5));
        }
        color = mix(color, cellColor, rings * edgeIsoIntensity);
    }

    // Center Iso Rings - iso lines radiating from center with falloff (mix)
    if (centerIsoIntensity > 0.0) {
        float phase = centerDist * isoFrequency;
        float rings;
        if (smoothMode) {
            // Anti-aliased using screen-space derivatives
            float fw = fwidth(phase) * 2.0;
            rings = smoothstep(fw, 0.0, abs(fract(phase) - 0.5) - 0.25) * centerDist;
        } else {
            rings = abs(sin(phase * TWO_PI * 0.5)) * centerDist;
        }
        color = mix(color, cellColor, rings * centerIsoIntensity);
    }

    // Flat Fill / Stained Glass - solid cell color with dark borders (mix)
    // Reference: smoothstep(0.07, 0.14, length(voronoiData.xy)) * randomColor
    if (flatFillIntensity > 0.0) {
        float eDist = length(voronoiData.xy);
        float fillMask = smoothstep(0.0, edgeFalloff, eDist);
        color = mix(color, fillMask * cellColor, flatFillIntensity);
    }

    // Edge Glow - brightness falloff from edges toward centers (additive)
    if (edgeGlowIntensity > 0.0) {
        float eDist = length(voronoiData.xy);
        float glow = 1.0 - smoothstep(0.0, edgeFalloff, eDist);
        color += glow * cellColor * edgeGlowIntensity;
    }

    // Determinant Shade - 2D cross product shading (mix)
    // Scaled up 4x since raw determinant values are very small
    if (determinantIntensity > 0.0) {
        float det = abs(voronoiData.x * voronoiData.w - voronoiData.z * voronoiData.y);
        color = mix(color, det * 4.0 * cellColor, determinantIntensity);
    }

    // Distance Ratio - border/center distance ratio (mix)
    // Reference: length(voronoiData.xy) / length(voronoiData.wz) * randomColor
    if (ratioIntensity > 0.0) {
        float ratio = length(voronoiData.xy) / (length(voronoiData.zw) + 0.001);
        color = mix(color, ratio * cellColor, ratioIntensity);
    }

    // Edge Detect - highlight where border is closer than center (mix)
    if (edgeDetectIntensity > 0.0) {
        float edge = smoothstep(0.0, edgeFalloff,
            length(voronoiData.xy) - length(voronoiData.zw));
        color = mix(color, cellColor, edge * edgeDetectIntensity);
    }

    finalColor = vec4(color, 1.0);
}
