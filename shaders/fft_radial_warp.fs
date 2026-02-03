#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D fftTexture;  // 1D FFT magnitudes (FFT_BIN_COUNT x 1)

uniform float intensity;
uniform float freqStart;
uniform float freqEnd;
uniform float maxRadius;
uniform float freqCurve;
uniform float bassBoost;
uniform int segments;
uniform float pushPullBalance;
uniform float pushPullSmoothness;
uniform float phaseOffset;  // Accumulated phase (base + speed * time)

const float TAU = 6.283185307;

void main() {
    vec2 center = vec2(0.5);
    vec2 delta = fragTexCoord - center;
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);

    // Map radius [0, maxRadius] to FFT bin range [freqStart, freqEnd]
    float freqCoord = clamp(radius / maxRadius, 0.0, 1.0);
    float normalizedRadius = freqCoord;  // Save for bass boost
    freqCoord = mix(freqStart, freqEnd, freqCoord);
    float magnitude = texture(fftTexture, vec2(freqCoord, 0.5)).r;

    // Freq curve: higher = punchier (invert the power so curve>1 boosts)
    magnitude = pow(magnitude, 1.0 / freqCurve);

    // Bass boost: amplify center based on actual bass energy
    float bassEnergy = texture(fftTexture, vec2(freqStart, 0.5)).r;
    float centerWeight = pow(1.0 - normalizedRadius, 2.0);
    magnitude += bassBoost * bassEnergy * centerWeight;

    // Angular wave: cosine gives symmetric smooth/hard transitions
    float segmentAngle = (angle + phaseOffset) * float(segments);
    float wave = cos(segmentAngle);

    // Smoothness: blend from hard sign() to smooth cosine
    float hardWave = sign(wave);
    float smoothedWave = mix(hardWave, wave, pushPullSmoothness);

    // Balance: 0=all pull, 0.5=alternating, 1=all push
    // At extremes, wave amplitude is 0 (uniform push or pull, no segments)
    // At center, full wave amplitude (visible alternating segments)
    float balanceOffset = (pushPullBalance - 0.5) * 2.0;
    float waveAmplitude = 1.0 - abs(balanceOffset);
    float pushPull = balanceOffset + smoothedWave * waveAmplitude;

    // Compute radial displacement
    vec2 radialDir = (radius > 0.0001) ? normalize(delta) : vec2(1.0, 0.0);
    vec2 displacement = radialDir * magnitude * intensity * pushPull;

    vec2 sampleUV = fragTexCoord + displacement;
    finalColor = texture(texture0, sampleUV);
}
