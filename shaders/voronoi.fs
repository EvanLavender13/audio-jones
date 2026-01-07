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
uniform float edgeDarkenIntensity;
uniform float angleShadeIntensity;
uniform float determinantIntensity;
uniform float ratioIntensity;
uniform float edgeDetectIntensity;

out vec4 finalColor;

const float TWO_PI = 6.28318530718;

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
                           flatFillIntensity + edgeDarkenIntensity + angleShadeIntensity +
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
    vec2 mr = vec2(0.0);  // Vector to nearest cell center
    vec2 mg = vec2(0.0);  // Grid offset of nearest cell
    float md = 8.0;

    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash2(ip + g);
            o = 0.5 + 0.5 * sin(time + TWO_PI * o);
            vec2 r = g + o - fp;
            float d = dot(r, r);
            if (d < md) {
                md = d;
                mr = r;
                mg = g;
            }
        }
    }

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

    // Voronoi data: xy = vector to border, zw = vector to center
    vec4 voronoiData = vec4(borderVec, mr);

    // Sample cell color from texture at cell center
    vec2 cellID = ip + mg;
    vec2 cellCenterUV = (cellID + 0.5) / vec2(scale * aspect, scale);
    vec3 cellColor = texture(texture0, cellCenterUV).rgb;

    // Start with original UV, apply distortion if enabled
    vec2 finalUV = fragTexCoord;
    if (uvDistortIntensity > 0.0) {
        vec2 displacement = mr * uvDistortIntensity * 0.5;
        displacement /= vec2(scale * aspect, scale);
        finalUV = fragTexCoord + displacement;
    }

    // Sample base color from (potentially distorted) UV
    vec3 color = texture(texture0, finalUV).rgb;

    // Edge Iso Rings - iso lines radiating from border (additive)
    // Reference: isoLine(length(voronoiData.xy), 20.) * randomColor
    if (edgeIsoIntensity > 0.0) {
        float eDist = length(voronoiData.xy);
        float rings = abs(sin(eDist * isoFrequency));
        color += rings * cellColor * edgeIsoIntensity;
    }

    // Center Iso Rings - iso lines radiating from center with falloff (additive)
    if (centerIsoIntensity > 0.0) {
        float cDist = length(voronoiData.zw);
        float rings = abs(sin(cDist * isoFrequency)) * cDist;
        color += rings * cellColor * centerIsoIntensity;
    }

    // Flat Fill / Stained Glass - solid cell color with dark borders (mix)
    // Reference: smoothstep(0.07, 0.14, length(voronoiData.xy)) * randomColor
    if (flatFillIntensity > 0.0) {
        float eDist = length(voronoiData.xy);
        float fillMask = smoothstep(0.0, edgeFalloff, eDist);
        color = mix(color, fillMask * cellColor, flatFillIntensity);
    }

    // Edge Darken / Leadframe - darken at cell borders (multiply)
    if (edgeDarkenIntensity > 0.0) {
        float eDist = length(voronoiData.xy);
        float darken = smoothstep(0.0, edgeFalloff, eDist);
        color *= mix(1.0, darken, edgeDarkenIntensity);
    }

    // Angle Shade - shading based on angle between border and center vectors (mix)
    if (angleShadeIntensity > 0.0) {
        float angle = abs(dot(normalize(voronoiData.xy), normalize(voronoiData.zw)));
        color = mix(color, angle * cellColor, angleShadeIntensity);
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

    // Edge Detect - highlight where border is closer than center (additive)
    // Reference: smoothstep(0., 0.1, length(voronoiData.xy) - length(voronoiData.wz)) * randomColor
    if (edgeDetectIntensity > 0.0) {
        float edge = smoothstep(0.0, edgeFalloff,
            length(voronoiData.xy) - length(voronoiData.zw));
        color += edge * cellColor * edgeDetectIntensity;
    }

    finalColor = vec4(color, 1.0);
}
