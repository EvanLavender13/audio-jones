// Based on "Fractal Tunnel Flight" by diatribes
// https://www.shadertoy.com/view/WXyfWh
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized iters/scale/path/roll/glow, gradient LUT replaces
//           cosine palette with depth-axis t for warm-near/cool-far separation,
//           FFT brightness on shared depth-cycle t index
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float flyPhase;
uniform float rollPhase;

uniform int marchSteps;
uniform int fractalIters;
uniform float preScale;
uniform float verticalOffset;
uniform float tunnelRadius;

uniform float pathAmplitude;
uniform float pathFreq;
uniform float rollAmount;

uniform float glowIntensity;
uniform float fogDensity;
uniform float depthCycle;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

vec3 pathPos(float z) {
    return vec3(cos(z * pathFreq) * pathAmplitude, 0.0, z);
}

mat2 rot2D(float a) {
    return mat2(cos(a + vec4(0.0, 33.0, 11.0, 0.0)));
}

float apollonian(vec3 p) {
    float w = 1.0;
    vec3 b = vec3(0.5, 1.0, 1.5);
    p.y += verticalOffset;
    p.yz = p.zy;
    p /= preScale;
    for (int i = 0; i < fractalIters; i++) {
        p = mod(p + b, 2.0 * b) - b;
        float s = 2.0 / dot(p, p);
        p *= s;
        w *= s;
    }
    return length(p) / w * 6.0;
}

float mapTunnel(vec3 p) {
    float tunnel = tunnelRadius - length((p - pathPos(p.z)).xy);
    float fractal = apollonian(p);
    return max(tunnel, fractal);
}

float sampleFFTBand(float freqT0, float freqT1) {
    float freqLo = baseFreq * pow(maxFreq / baseFreq, freqT0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, freqT1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int b = 0; b < BAND_SAMPLES; b++) {
        float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    return baseBright + mag;
}

void main() {
    vec2 u = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

    float T = flyPhase + 5.0;
    vec3 camPos = pathPos(T * 2.0);
    vec3 camTarget = pathPos(T * 2.0 + 7.0);
    vec3 Z = normalize(camTarget - camPos);
    vec3 X = normalize(vec3(Z.z, 0.0, -Z.x));
    vec3 D = normalize(vec3(rot2D(rollPhase * rollAmount) * u, 1.0)
                       * mat3(-X, cross(X, Z), Z));

    vec3 p = camPos;
    vec3 accumulator = vec3(0.0);
    float d = 0.0;
    float s = 0.0;

    for (int iter = 0; iter < marchSteps; iter++) {
        p += D * s;
        s = mapTunnel(p) * 0.8;
        d += s;

        float t = fract(p.z / depthCycle);
        float bw = 1.0 / float(marchSteps);
        vec3 baseColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
        float brightness = sampleFFTBand(t, t + bw);

        accumulator += baseColor * brightness / max(s, 0.0003);
    }

    vec3 toned = tanh(accumulator * glowIntensity / d * exp(-d * fogDensity));
    finalColor = vec4(toned, 1.0);
}
