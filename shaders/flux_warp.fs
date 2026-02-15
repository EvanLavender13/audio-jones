#version 330

// Flux Warp: Dual-phase modular arithmetic displacement with animated divisor blending

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform vec2 resolution;
uniform float warpStrength;  // Overall displacement magnitude
uniform float cellScale;     // UV scaling from center
uniform float coupling;      // x-to-y coupling factor
uniform float waveFreq;      // Primary wave frequency
uniform float time;           // Accumulated animation time
uniform float divisorSpeed;  // Divisor blend oscillation rate
uniform float gateSpeed;     // Amplitude gate oscillation rate

out vec4 finalColor;

float fluxField(vec2 uv, float time, float phaseOffset) {
    float t = time + phaseOffset;
    float x = cos(waveFreq * uv.x + 0.2 * t);
    float y = sin(uv.y + coupling * x - 0.5 * t);

    // Animated divisor blend: product-cells <-> radial-cells (full 0-1 sweep)
    float b = 0.5 * (1.0 + cos(divisorSpeed * t));

    // Amplitude modulation gate
    float b2 = 0.5 * (1.0 + cos(gateSpeed * t));

    float divisor = (1.0 - b) * x * y + b * 0.4 * length(uv);
    return mod(x + y, divisor) * (b2 + (1.0 - b2) * 0.5 * (0.2 + sin(2.0 * uv.x * uv.y + t)));
}

void main()
{
    vec2 uv = fragTexCoord;
    float aspect = resolution.x / resolution.y;
    vec2 centered = cellScale * (vec2((uv.x - 0.5) * aspect, uv.y - 0.5));

    float fx = fluxField(centered, time, 0.0);    // phaseX = 0
    float fy = fluxField(centered, time, 1.7);    // phaseY = 1.7 (decorrelated)

    vec2 displaced = uv + vec2(fx, fy) * warpStrength;
    finalColor = texture(texture0, clamp(displaced, 0.0, 1.0));
}
