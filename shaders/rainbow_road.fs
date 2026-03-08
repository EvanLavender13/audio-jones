// Based on "Rainbow Road" by XorDev
// https://www.shadertoy.com/view/NlGfzz
// License: CC BY-NC-SA 3.0
// Modified: FFT-reactive bars, configurable sway/curvature, gradient LUT coloring

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform float scroll;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform int layers;
uniform int direction;
uniform float width;
uniform float sway;
uniform float curvature;
uniform float phaseSpread;
uniform float glowIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    // Pixel coords (reference uses I for fragCoord)
    vec2 r = resolution;
    vec2 I = fragTexCoord * r;
    if (direction == 1) I.y = r.y - I.y;

    vec3 color = vec3(0.0);
    float step = 25.0 / float(layers);

    for (int idx = 0; idx < layers; idx++) {
        float i = (float(idx) + scroll) * step;
        if (i < 0.01) continue;

        // --- FFT energy ---
        float t = float(idx) / float(layers - 1);
        float t1 = float(idx + 1) / float(layers);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // --- Reference: o = (I+I-r)/r.y*i + cos(i*vec2(.8,.5)+iTime) ---
        vec2 o = (I + I - r) / r.y * i
               + vec2(sway * cos(i * phaseSpread + time),
                      sway * cos(i * phaseSpread * 0.625 + time));
        // --- Reference: o.y += 4.-i ---
        o.y += 4.0 - i;

        // --- Reference: d = vec2(4, sin(i)*.4) ---
        vec2 d = vec2(width, curvature * sin(i * phaseSpread));

        // --- Segment SDF ---
        float proj = clamp(dot(o, d) / dot(d, d), -1.0, 1.0);
        float dist = length(o - proj * d);

        // --- Reference: (cos(i+..)+1.) / max(i*i,5.)*.1 / (dist/i + i/1e3) ---
        float glow = glowIntensity * 0.2 / max(i * i, 5.0)
                   / (dist / i + i / 1000.0);

        vec3 barColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
        color += glow * barColor * brightness;
    }

    finalColor = vec4(color, 1.0);
}
