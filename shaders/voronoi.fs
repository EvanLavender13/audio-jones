#version 330

// Voronoi cellular transform shader
// Computes voronoi geometry once, applies selected cell sub-effect

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float scale;        // Cell count across screen width
uniform float time;         // Animation driver
uniform float edgeFalloff;  // Edge softness for multiple effects
uniform float isoFrequency; // Ring density for iso effects

uniform int mode;       // Active effect mode (0-8)
uniform float intensity; // Effect strength (0.0-1.0)
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
    // Early out if intensity is zero
    if (intensity <= 0.0) {
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

    // Border calc requires sharp nearest-cell vector; smooth creates edge artifacts
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

    float centerDist = smoothMode ? smoothDist : length(mr);
    float edgeDist = length(borderVec);

    // Voronoi data: xy = vector to border, zw = vector to center
    vec4 voronoiData = vec4(borderVec, mr);

    // Sample cell color from texture at cell center
    vec2 cellID = ip + mg;
    vec2 cellCenterUV = (cellID + 0.5) / vec2(scale * aspect, scale);
    vec3 cellColor = texture(texture0, cellCenterUV).rgb;

    vec2 finalUV = fragTexCoord;

    // UV distort modes apply before texture sample
    if (mode == 0) {
        // UV Distort
        vec2 displacement = smoothMr * intensity * 0.5;
        displacement /= vec2(scale * aspect, scale);
        finalUV = finalUV + displacement;
    } else if (mode == 1) {
        // Organic Flow
        float flowMask = smoothstep(0.0, edgeFalloff, length(smoothMr));
        vec2 displacement = smoothMr * intensity * 0.5 * flowMask;
        displacement /= vec2(scale * aspect, scale);
        finalUV = finalUV + displacement;
    }

    // Sample base color from (potentially distorted) UV
    vec3 color = texture(texture0, finalUV).rgb;

    // Post-sample modes
    if (mode == 2) {
        // Edge Iso Rings - iso lines radiating from border (mix)
        float phase = edgeDist * isoFrequency;
        float rings;
        if (smoothMode) {
            float fw = fwidth(phase) * 2.0;
            rings = smoothstep(fw, 0.0, abs(fract(phase) - 0.5) - 0.25);
        } else {
            rings = abs(sin(phase * TWO_PI * 0.5));
        }
        vec3 isoColor = rings * cellColor;
        color = mix(color, isoColor, intensity);
    } else if (mode == 3) {
        // Center Iso Rings - iso lines radiating from center (mix)
        float phase = centerDist * isoFrequency;
        float rings;
        if (smoothMode) {
            float fw = fwidth(phase) * 2.0;
            rings = smoothstep(fw, 0.0, abs(fract(phase) - 0.5) - 0.25);
        } else {
            rings = abs(sin(phase * TWO_PI * 0.5));
        }
        vec3 isoColor = rings * cellColor;
        color = mix(color, isoColor, intensity);
    } else if (mode == 4) {
        // Flat Fill
        float fillMask = smoothstep(0.0, edgeFalloff, edgeDist);
        color = mix(color, fillMask * cellColor, intensity);
    } else if (mode == 5) {
        // Edge Glow
        float edge = 1.0 - smoothstep(0.0, edgeFalloff, edgeDist);
        vec3 edgeOnly = edge * texture(texture0, fragTexCoord).rgb;
        color = mix(color, edgeOnly, intensity);
    } else if (mode == 6) {
        // Distance Ratio - border/center distance ratio (mix)
        float ratio = length(voronoiData.xy) / (length(voronoiData.zw) + 0.001);
        color = mix(color, ratio * cellColor, intensity);
    } else if (mode == 7) {
        // Determinant Shade - 2D cross product shading (mix)
        // Scaled up 4x since raw determinant values are very small
        float det = abs(voronoiData.x * voronoiData.w - voronoiData.z * voronoiData.y);
        color = mix(color, det * 4.0 * cellColor, intensity);
    } else if (mode == 8) {
        // Edge Detect - highlight where border is closer than center (mix)
        float edge = smoothstep(0.0, edgeFalloff,
            length(voronoiData.xy) - length(voronoiData.zw));
        vec3 detectColor = edge * cellColor;
        color = mix(color, detectColor, intensity);
    }

    finalColor = vec4(color, 1.0);
}
