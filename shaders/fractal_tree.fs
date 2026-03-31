// Fractal Tree: KIFS fractal tree with depth-tracked gradient color and FFT audio reactivity.
// Based on: "Fractal tree" by coffeecake6022
// Source: https://www.shadertoy.com/view/scj3Dc
// License: CC BY-NC-SA 3.0
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

uniform float thickness;
uniform int maxIter;
uniform float zoomAccum;
uniform float rotationAccum;

vec4 samp(vec2 p, float zoom, float ct) {
    // Center on limiting point and zoom
    p = (2.0 * p - resolution) / resolution.y;
    p *= 0.5 * zoom;
    float cs = cos(rotationAccum);
    float sn = sin(rotationAccum);
    p = vec2(p.x * cs - p.y * sn, p.x * sn + p.y * cs);
    p += vec2(0.70062927, 0.9045085);

    // KIFS core -- golden-ratio constants are locked
    float s = zoom;
    p.x = abs(p.x);
    int depth = 0;
    int iMax = maxIter + int(ct);
    for (int i = 0; i < iMax; i++) {
        if (p.y - 1.0 > -0.577 * p.x) {
            p.y -= 1.0;
            p *= mat2(0.809015, -1.401257, 1.401257, 0.809015);
            p.x = abs(p.x);
            s *= 1.618033;
            depth = i + 1;
        } else {
            break;
        }
    }

    // Branch test
    float inBranch = min(step(p.x, s * thickness * 0.5 / resolution.y),
                         step(abs(p.y - 0.5), 0.5));
    if (inBranch < 0.5) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    // Depth-indexed color and audio
    float t = float(depth) / float(iMax);

    vec3 col = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    return vec4(col * brightness, 1.0);
}

void main() {
    float ct = 6.0 * fract(zoomAccum);
    float zoom = pow(0.618, ct);

    vec2 pos = fragTexCoord * resolution;

    // 5-sample supersampling (center + 4 cardinal at 0.5px)
    vec4 col = samp(pos, zoom, ct);
    col += samp(pos - vec2(0.5, 0.0), zoom, ct);
    col += samp(pos + vec2(0.5, 0.0), zoom, ct);
    col += samp(pos - vec2(0.0, 0.5), zoom, ct);
    col += samp(pos + vec2(0.0, 0.5), zoom, ct);
    col *= 0.2;

    finalColor = col;
}
