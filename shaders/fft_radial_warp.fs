#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D fftTexture;  // 1D FFT magnitudes (FFT_BIN_COUNT x 1)

uniform float intensity;
uniform float freqStart;
uniform float freqEnd;
uniform float maxRadius;
uniform int segments;
uniform float pushPullPhase;

const float TAU = 6.283185307;

void main() {
    vec2 center = vec2(0.5);
    vec2 delta = fragTexCoord - center;
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);

    // Map radius [0, maxRadius] to FFT bin range [freqStart, freqEnd]
    float freqCoord = clamp(radius / maxRadius, 0.0, 1.0);
    freqCoord = mix(freqStart, freqEnd, freqCoord);
    float magnitude = texture(fftTexture, vec2(freqCoord, 0.5)).r;

    // Bidirectional: angular segments alternate push/pull
    float segmentPhase = angle * float(segments) / TAU + pushPullPhase;
    float pushPull = mix(-1.0, 1.0, step(0.5, fract(segmentPhase)));

    // Compute radial displacement
    vec2 radialDir = (radius > 0.0001) ? normalize(delta) : vec2(1.0, 0.0);
    vec2 displacement = radialDir * magnitude * intensity * pushPull;

    vec2 sampleUV = fragTexCoord + displacement;
    finalColor = texture(texture0, sampleUV);
}
