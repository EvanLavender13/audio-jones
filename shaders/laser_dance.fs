// Based on "Laser Dance" by @XorDev
// https://www.shadertoy.com/view/tct3Rf
// License: CC BY-NC-SA 3.0 Unported
// Modified: Removed floor reflection, gradient LUT coloring, FFT audio reactivity
// Warp technique from "Star Field Flight [351]" by diatribes (https://www.shadertoy.com/view/3ft3DS)
// Camera drift and rotation from "Star Field Flight [351]" by diatribes (https://www.shadertoy.com/view/3ft3DS)
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform float freqRatio;
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
uniform float warpAmount;
uniform float warpTime;
uniform float warpFreq;
uniform vec2 cameraDrift;
uniform float cameraAngle;
uniform float zoom;

const float MAX_DEPTH = 12.0;
const int STEPS = 100;
const int BAND_SAMPLES = 8;

void main() {
    // FFT brightness - average energy across baseFreq to maxFreq in log space
    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float t = (float(s) + 0.5) / float(BAND_SAMPLES);
        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float fftBright = baseBright + mag;

    // Ray setup - centered coords, aspect-corrected
    vec2 uv = (fragTexCoord - 0.5) * 2.0;
    uv.x *= resolution.x / resolution.y;
    vec3 rayDir = normalize(vec3(uv, -zoom));

    // Raymarch loop
    vec3 color = vec3(0.0);
    float z = 0.0;

    for (int i = 0; i < STEPS; i++) {
        // Sample point along ray
        vec3 p = z * rayDir;
        p.xy += cameraDrift;                                // camera position offset
        float ca = cos(cameraAngle), sa = sin(cameraAngle); // camera view rotation
        p.xy = mat2(ca, -sa, sa, ca) * p.xy;
        p += cos(p.yzx * warpFreq + warpTime * vec3(1.0, 1.3, 0.7)) * warpAmount;

        // Laser distance field (two cosine fields + crease)
        vec3 q = cos(p + time) + cos(p / freqRatio).yzx;
        float dist = length(min(q, q.zxy));

        // Step forward
        z += 0.3 * (0.01 + dist);

        // Accumulate color from gradient LUT by depth
        vec3 sc = textureLod(gradientLUT, vec2(z / MAX_DEPTH + colorPhase, 0.5), 0.0).rgb;
        float atten = min(1.0 / max(dist, 0.001) / max(z, 0.001), 50.0);
        color += sc * atten;
    }

    // Tonemapping
    color *= fftBright;
    color = tanh(color / (700.0 / brightness));
    finalColor = vec4(color, 1.0);
}
