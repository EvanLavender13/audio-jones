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
uniform int foldMode;
uniform int distMode;
uniform int combineMode;

const float MAX_DEPTH = 12.0;
const int STEPS = 100;

void main() {
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

        // Laser distance field
        vec3 a = cos(p + time);
        vec3 b = cos(p / freqRatio).yzx;
        vec3 q;
        if (combineMode == 1)      { q = a * b; }
        else if (combineMode == 2) { q = a - b; }
        else if (combineMode == 3) { q = min(a, b); }
        else if (combineMode == 4) { q = max(a, b); }
        else                       { q = a + b; }

        vec3 f;
        if (foldMode == 1)      { f = max(q, q.zxy); }
        else if (foldMode == 2) { f = abs(q - q.zxy); }
        else if (foldMode == 3) { f = q * q.zxy; }
        else                    { f = min(q, q.zxy); }

        float dist;
        if (distMode == 1)      { dist = max(abs(f.x), max(abs(f.y), abs(f.z))); }
        else if (distMode == 2) { dist = abs(f.x) + abs(f.y) + abs(f.z); }
        else if (distMode == 3) { dist = dot(f, f); }
        else                    { dist = length(f); }

        // Step forward
        z += 0.3 * (0.01 + dist);

        // Color and audio by depth - same index for gradient and FFT
        float t = z / MAX_DEPTH + colorPhase;
        vec3 sc = textureLod(gradientLUT, vec2(t, 0.5), 0.0).rgb;

        float freq = baseFreq * pow(maxFreq / baseFreq, fract(t));
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float fftBright = baseBright + mag;

        float atten = min(1.0 / max(dist, 0.001) / max(z, 0.001), 50.0);
        color += sc * atten * fftBright;
    }

    // Tonemapping
    color = tanh(color / (700.0 / brightness));
    finalColor = vec4(color, 1.0);
}
