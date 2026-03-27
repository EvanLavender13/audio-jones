#version 330

// Based on "logpolar mapping - 2" by FabriceNeyret2
// https://www.shadertoy.com/view/3sSSDR
// License: CC BY-NC-SA 3.0 Unported
// Modified: cellular sub-effects, zoom/spin phase

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float scale;
uniform float zoomPhase;
uniform float spinPhase;
uniform float edgeFalloff;
uniform float isoFrequency;
uniform int mode;
uniform float intensity;

out vec4 finalColor;

const float TWO_OVER_PI = 0.6366;
const float TWO_PI = 6.28318530718;

void main()
{
    // Early out if intensity is zero
    if (intensity <= 0.0) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    vec2 pos = fragTexCoord * resolution - resolution * 0.5;

    // Clamp to avoid log(0) singularity at center
    float r = max(length(pos), 1.0);

    // Log-polar transform
    // The atan() has a discontinuity at angle +/-PI (left edge of screen).
    // The 2/PI normalization maps the angular period to 4 units, so
    // 4 * scale being an integer guarantees seamless tiling.
    vec2 lp = vec2(log(r / resolution.y) - zoomPhase,
                   atan(pos.y, pos.x) - spinPhase);

    // Conformal 45-degree rotation (diamond cells)
    vec2 conformal = TWO_OVER_PI * lp * mat2(1, -1, 1, 1);

    // Scale to cell grid
    vec2 cellCoord = conformal * scale;
    vec2 ip = floor(cellCoord);   // cell ID
    vec2 fp = fract(cellCoord);   // position within cell [0,1)

    // Cell geometry from fract coordinates
    vec2 toCenter = vec2(0.5) - fp;
    float centerDist = length(toCenter);

    float edgeDistX = min(fp.x, 1.0 - fp.x);
    float edgeDistY = min(fp.y, 1.0 - fp.y);
    float edgeDist = min(edgeDistX, edgeDistY);

    // Direction toward nearest cell edge
    vec2 borderVec;
    if (edgeDistX < edgeDistY) {
        borderVec = vec2(sign(0.5 - fp.x) * edgeDistX, 0.0);
    } else {
        borderVec = vec2(0.0, sign(0.5 - fp.y) * edgeDistY);
    }

    // Cell center UV (inverse transform for cellColor sampling)
    vec2 cellCenterLP = (ip + 0.5) / scale;
    // Inverse conformal: inverse of mat2(1,-1,1,1) is mat2(1,1,-1,1) * 0.5
    cellCenterLP = (cellCenterLP * mat2(1, 1, -1, 1)) * 0.5 / TWO_OVER_PI;
    // Add back animation phases
    float rho = cellCenterLP.x + zoomPhase;
    float theta = cellCenterLP.y + spinPhase;
    // Inverse log-polar
    float cellR = exp(rho) * resolution.y;
    vec2 cartesian = cellR * vec2(cos(theta), sin(theta));
    // To UV
    vec2 cellCenterUV = (cartesian + resolution * 0.5) / resolution;
    vec3 cellColor = texture(texture0, cellCenterUV).rgb;


    // Inverse Jacobian: cell-space displacement to pixel displacement
    // Forward transform scales as 1/r, so inverse scales as r
    float invJacobian = max(length(pos), 1.0) / (TWO_OVER_PI * scale);

    vec2 finalUV = fragTexCoord;

    // UV distort modes apply before texture sample
    if (mode == 0) {
        // UV Distort
        vec2 displacement = toCenter * intensity * 0.5 * invJacobian;
        finalUV = finalUV + displacement / resolution;
    } else if (mode == 1) {
        // Organic Flow
        float flowMask = smoothstep(0.0, edgeFalloff, length(toCenter));
        vec2 displacement = toCenter * intensity * 0.5 * flowMask * invJacobian;
        finalUV = finalUV + displacement / resolution;
    }

    // Sample base color from (potentially distorted) UV
    vec3 color = texture(texture0, finalUV).rgb;

    // Post-sample modes
    if (mode == 2) {
        // Edge Iso Rings - iso lines radiating from border (mix)
        float phase = edgeDist * isoFrequency;
        float rings = abs(sin(phase * TWO_PI * 0.5));
        vec3 isoColor = rings * cellColor;
        color = mix(color, isoColor, intensity);
    } else if (mode == 3) {
        // Center Iso Rings - iso lines radiating from center (mix)
        float phase = centerDist * isoFrequency;
        float rings = abs(sin(phase * TWO_PI * 0.5));
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
        float ratio = length(borderVec) / (length(toCenter) + 0.001);
        color = mix(color, ratio * cellColor, intensity);
    } else if (mode == 7) {
        // Determinant Shade - 2D cross product shading (mix)
        // Scaled up 4x since raw determinant values are very small
        float det = abs(borderVec.x * toCenter.y - toCenter.x * borderVec.y);
        color = mix(color, det * 4.0 * cellColor, intensity);
    } else if (mode == 8) {
        // Edge Detect - highlight where border is closer than center (mix)
        float edge = smoothstep(0.0, edgeFalloff,
            length(borderVec) - length(toCenter));
        vec3 detectColor = edge * cellColor;
        color = mix(color, detectColor, intensity);
    }

    finalColor = vec4(color, 1.0);
}
