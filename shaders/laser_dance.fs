// Based on "Laser Dance" by @XorDev
// https://www.shadertoy.com/view/tct3Rf
// License: CC BY-NC-SA 3.0 Unported
// Modified: Removed floor reflection, gradient LUT coloring, FFT audio reactivity
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform float freqRatio;
uniform float cameraOffset;
uniform float brightness;
uniform float colorPhase;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float MAX_DEPTH = 12.0;
const int STEPS = 100;
const int BAND_SAMPLES = 8;

void main() {
    // FFT brightness — average energy across baseFreq to maxFreq in log space
    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float t = (float(s) + 0.5) / float(BAND_SAMPLES);
        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float fftBright = baseBright + mag;

    // Ray setup — centered coords, aspect-corrected
    vec2 uv = (fragTexCoord - 0.5) * 2.0;
    uv.x *= resolution.x / resolution.y;
    vec3 rayDir = normalize(vec3(uv, -1.0));

    // Raymarch loop
    vec3 color = vec3(0.0);
    float z = 0.0;

    for (int i = 0; i < STEPS; i++) {
        // Sample point along ray
        vec3 p = z * rayDir + vec3(cameraOffset);

        // Laser distance field (two cosine fields + crease)
        vec3 q = cos(p + time) + cos(p / freqRatio).yzx;
        float dist = length(min(q, q.zxy));

        // Step forward
        z += 0.3 * (0.01 + dist);

        // Accumulate color from gradient LUT by depth
        vec3 sc = textureLod(gradientLUT, vec2(z / MAX_DEPTH + colorPhase, 0.5), 0.0).rgb;
        color += sc / max(dist, 0.001) / max(z, 0.001);
    }

    // Tonemapping
    color *= fftBright;
    color = tanh(color / (700.0 / brightness));
    finalColor = vec4(color, 1.0);
}
