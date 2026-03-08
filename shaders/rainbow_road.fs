// Based on "Rainbow Road" by XorDev
// https://www.shadertoy.com/view/NlGfzz
// License: CC BY-NC-SA 3.0
// Modified: FFT-reactive bars, configurable perspective/sway/curvature, gradient LUT coloring

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform int layers;
uniform int direction;
uniform float perspective;
uniform float maxWidth;
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
    // Centered, aspect-corrected UV: (0,0) at center, Y spans -0.5 to +0.5
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

    // Direction: 0 = recede upward (big at bottom), 1 = recede downward (big at top)
    if (direction == 1) uv.y = -uv.y;

    vec3 color = vec3(0.0);

    for (int i = 0; i < layers; i++) {
        float fi = float(i);

        // --- FFT energy (standard BAND_SAMPLES pattern) ---
        float t0 = fi / float(layers - 1);
        float t1 = float(i + 1) / float(layers);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
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

        // --- Perspective depth ---
        // depth = 1 at bar 0 (nearest), grows linearly per bar
        float depth = 1.0 + fi * perspective;

        // --- Position in bar's local space ---
        vec2 p;
        p.x = uv.x * depth + sway * cos(fi * phaseSpread + time);
        p.y = (uv.y + 0.5) * depth - fi;
        // +0.5: shifts UV origin to bottom edge before scaling
        // Bar i center at screen-Y = fi / depth - 0.5

        // --- Bar segment: horizontal with per-bar tilt ---
        float hw = maxWidth * brightness;                   // half-width scales with energy
        float tilt = curvature * sin(fi * phaseSpread);     // vertical component at endpoints
        vec2 barEnd = vec2(hw, tilt * hw);                  // endpoint relative to bar center

        // --- Segment SDF ---
        float proj = clamp(dot(p, barEnd) / dot(barEnd, barEnd), -1.0, 1.0);
        float dist = length(p - proj * barEnd);

        // --- Glow with depth fade ---
        float depthFade = 1.0 / max(depth * depth, 5.0);
        float glow = glowIntensity * depthFade / (dist / depth + 0.001);

        // --- Color from gradient LUT ---
        float t = fi / float(layers - 1);
        vec3 barColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
        color += glow * barColor * brightness;
    }

    finalColor = vec4(color, 1.0);
}
