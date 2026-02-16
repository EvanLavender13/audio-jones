// Iris Rings: Concentric tilted rings with per-ring differential twist rotation,
// arc gating driven by FFT energy. Tilt projection from Spectral Arcs.
// Each ring opens an angular arc proportional to its frequency band energy,
// capped at half circle so rings never close into full circles.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform int layers;
uniform float ringScale;
uniform float rotationAccum;
uniform float tilt;
uniform float tiltAngle;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define PI 3.14159265359
#define TWO_PI 6.28318530718

void main() {
    vec2 r = resolution;
    vec2 fc = fragTexCoord * r;

    vec3 result = vec3(0.0);

    // Rotate centered coords by tiltAngle, then apply perspective tilt.
    vec2 centered = fc - r * 0.5;
    float ca = cos(tiltAngle), sa = sin(tiltAngle);
    vec2 rotated = vec2(centered.x * ca - centered.y * sa,
                        centered.x * sa + centered.y * ca);
    mat2 tiltMat = mat2(1.0, -tilt, 2.0 * tilt, 1.0 + tilt);
    vec2 p = rotated * tiltMat;

    // Perspective projection: depth varies with y for tilted-disc look
    float depth = r.y + r.y - p.y * tilt;
    vec2 uv = p / depth;

    float fl = float(layers);

    for (int i = 0; i < layers; i++) {
        float fi = float(i + 1);

        // Per-ring scramble: fi*fi gives chaotic fixed offsets,
        // sin(fi*fi) makes each ring rotate at a different speed/direction
        float angle = rotationAccum * sin(fi * fi) + fi * fi;
        float c = cos(angle), s = sin(angle);
        vec2 ruv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

        // Ring distance in ring-normalized space (1.0 = one ring gap).
        // Glow width auto-scales with layer count like spectral arcs.
        float dist = length(ruv) * fl / ringScale - fi;

        // Normalized angle for arc gating
        float a = (atan(ruv.y, ruv.x) + PI) / TWO_PI;

        // FFT frequency band — spread across full spectrum in log space
        float t0 = float(i) / fl;
        float t1 = fi / fl;
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float mag = 0.0;
        const int BAND_SAMPLES = 4;
        for (int bs = 0; bs < BAND_SAMPLES; bs++) {
            float bin = mix(binLo, binHi, (float(bs) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                mag += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        mag = pow(clamp(mag / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

        // Arc gating — square mag and cap at 0.5 so arcs max at half circle
        float gate = step(a, mag * mag * 0.5);

        // Clean flat ring via smoothstep (like reference's smoothstep(.02,.015,l))
        float ring = smoothstep(0.5, 0.3, abs(dist));
        vec3 color = texture(gradientLUT, vec2(float(i) / fl, 0.5)).rgb;
        result += ring * gate * color * (baseBright + mag);
    }

    finalColor = vec4(result, 1.0);
}
