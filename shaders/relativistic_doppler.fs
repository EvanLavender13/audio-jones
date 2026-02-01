// shaders/relativistic_doppler.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;

uniform float velocity;    // 0.0 - 0.99
uniform vec2 center;       // Normalized center point
uniform float aberration;  // 0.0 - 1.0
uniform float colorShift;  // 0.0 - 1.0
uniform float headlight;   // 0.0 - 1.0

const float PI = 3.14159265359;

// Rodrigues rotation for hue shift
vec3 hueShift(vec3 color, float hue) {
    const vec3 k = vec3(0.57735);
    float c = cos(hue);
    float s = sin(hue);
    return color * c + cross(k, color) * s + k * dot(k, color) * (1.0 - c);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 aspect = vec2(resolution.x / resolution.y, 1.0);

    // Vector from center to pixel (aspect-corrected)
    vec2 toPixel = (uv - center) * aspect;
    float dist = length(toPixel);
    vec2 dir = dist > 0.0001 ? toPixel / dist : vec2(0.0);

    // Angle from "forward" (center = 0, edge = π)
    float maxDist = length((vec2(1.0) - center) * aspect);
    float theta = (dist / maxDist) * PI;

    // Lorentz factor
    float beta = velocity;
    float gamma = 1.0 / sqrt(max(1.0 - beta * beta, 0.0001));

    // Relativistic aberration: θ' = acos((cos(θ) - β) / (1 - β·cos(θ)))
    float cosTheta = cos(theta);
    float cosThetaPrime = (cosTheta - beta) / (1.0 - beta * cosTheta);
    cosThetaPrime = clamp(cosThetaPrime, -1.0, 1.0);
    float thetaPrime = acos(cosThetaPrime);

    // Aberrated distance (compressed toward center)
    float aberratedDist = (thetaPrime / PI) * maxDist;
    vec2 aberratedUV = center + dir * aberratedDist / aspect;

    // Blend between original and aberrated UV
    vec2 finalUV = mix(uv, aberratedUV, aberration);

    // Sample texture
    vec3 color = texture(texture0, finalUV).rgb;

    // Doppler factor: D = 1 / (γ · (1 - β·cos(θ)))
    float D = 1.0 / (gamma * (1.0 - beta * cosTheta));

    // Hue shift: D > 1 = blue shift (negative), D < 1 = red shift (positive)
    float hueOffset = (1.0 - D) * 0.8 * colorShift;
    color = hueShift(color, hueOffset);

    // Headlight effect: boost brightness toward center
    float intensityBoost = pow(D, 3.0);
    color = mix(color, color * intensityBoost, headlight);

    finalColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
