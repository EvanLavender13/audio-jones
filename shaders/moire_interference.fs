// Moiré Interference: Wave displacement with multi-layer interference patterns
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

// Global
uniform int patternMode;    // 0=stripes, 1=circles, 2=grid
uniform int profileMode;    // 0=sine, 1=square, 2=triangle, 3=sawtooth
uniform int layerCount;
uniform float centerX;
uniform float centerY;
uniform vec2 resolution;

// Per-layer
struct WaveLayer {
    float frequency;
    float angle;      // static angle + accumulated drift (combined on CPU)
    float amplitude;
    float phase;
};

uniform WaveLayer layer0;
uniform WaveLayer layer1;
uniform WaveLayer layer2;
uniform WaveLayer layer3;

out vec4 finalColor;

#define TAU 6.28318530718
#define PI  3.14159265359

float profile(float t, int mode) {
    if (mode == 1) return sign(sin(t));
    if (mode == 2) return asin(sin(t)) * (2.0 / PI);
    if (mode == 3) return 2.0 * fract(t / TAU + 0.5) - 1.0;
    return sin(t);
}

// Compute displacement for one layer
vec2 layerDisplacement(WaveLayer l, vec2 centered, float aspect) {
    vec2 pos = centered;
    pos.x *= aspect;

    if (patternMode == 1) {
        // Circles: radial displacement
        float dist = length(pos);
        float wave = profile(dist * l.frequency * TAU + l.phase, profileMode);
        vec2 radDir = (dist > 0.001) ? normalize(pos) : vec2(0.0);
        radDir.x /= aspect;
        return radDir * wave * l.amplitude;
    }

    if (patternMode == 2) {
        // Grid: sum of two perpendicular planar waves
        vec2 dir = vec2(cos(l.angle), sin(l.angle));
        vec2 perp = vec2(-dir.y, dir.x);

        float waveA = profile(dot(pos, dir) * l.frequency * TAU + l.phase, profileMode);
        float waveB = profile(dot(pos, perp) * l.frequency * TAU + l.phase, profileMode);

        vec2 perpA = vec2(-dir.y, dir.x);
        perpA.x /= aspect;
        vec2 perpB = vec2(dir.x, dir.y);
        perpB.x /= aspect;

        return (perpA * waveA + perpB * waveB) * l.amplitude * 0.5;
    }

    // Stripes (default): planar wave along direction, displace perpendicular
    vec2 dir = vec2(cos(l.angle), sin(l.angle));
    float wave = profile(dot(pos, dir) * l.frequency * TAU + l.phase, profileMode);
    vec2 perp = vec2(-dir.y, dir.x);
    perp.x /= aspect;
    return perp * wave * l.amplitude;
}

void main() {
    vec2 center = vec2(centerX, centerY);
    vec2 centered = fragTexCoord - center;
    float aspect = resolution.x / resolution.y;

    vec2 displacement = vec2(0.0);
    int count = clamp(layerCount, 1, 4);

    displacement += layerDisplacement(layer0, centered, aspect);
    if (count >= 2) displacement += layerDisplacement(layer1, centered, aspect);
    if (count >= 3) displacement += layerDisplacement(layer2, centered, aspect);
    if (count >= 4) displacement += layerDisplacement(layer3, centered, aspect);

    vec2 uv = fragTexCoord + displacement;
    uv = 1.0 - abs(mod(uv, 2.0) - 1.0);  // mirror repeat

    finalColor = texture(texture0, uv);
}
