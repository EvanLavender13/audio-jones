// Based on "Kuantum Dalgalanmalari" by msttezcan
// https://www.shadertoy.com/view/7fj3DK
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT audio reactivity, gradient LUT coloring
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float flyPhase;
uniform int marchSteps;
uniform float gyroidScale;
uniform float surfaceThickness;
uniform float turbulenceScale;
uniform float turbulenceAmount;
uniform float isoFreq;
uniform float isoStrength;
uniform float baseOffset;
uniform float tonemapGain;
uniform float camDrift;
uniform vec2 pan;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform sampler2D gradientLUT;

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
    float gA = gyroidScale;
    float gB = gyroidScale * 0.71;
    float gC = gyroidScale * 0.33;

    float s;
    float d = 0.0;
    vec3 color = vec3(0.0);

    vec2 uv = (fragTexCoord * resolution - 0.5 * resolution) / resolution.y;

    uv += pan * camDrift;

    float bw = 1.0 / float(marchSteps);

    for (int i = 0; i < marchSteps; i++) {
        vec3 p = vec3(uv * d, d + flyPhase);

        // Iso-line warp on z
        float isoWarp = cos(p.z * isoFreq) + sin(p.z * isoFreq);
        p.z += isoWarp * isoStrength;

        // Lateral offset to avoid gyroid origin; lissajous provides drift
        p.x += baseOffset + pan.x * baseOffset * 0.1;

        // Turbulence warp (two octaves with 2:1 wavelength ratio)
        p += cos(p.yzx / turbulenceScale) * turbulenceAmount
           + sin(p.yzx / (turbulenceScale * 2.0)) * (turbulenceAmount * 0.5);

        // Gyroid SDF
        s = 0.005 + 0.8 * abs(surfaceThickness
            * dot(sin(p / gA), cos(p.yzx / gB) + sin(p.yzx / gC)));

        // Depth-mapped FFT and gradient color (index by iteration)
        float t = float(i) / float(marchSteps);
        float brightness = sampleFFTBand(t, t + bw);
        vec3 col = texture(gradientLUT, vec2(t, 0.5)).rgb;

        color += col / s * brightness;

        d += s;
    }

    // tanh tonemapping
    finalColor = vec4(tanh(color * tonemapGain), 1.0);
}
