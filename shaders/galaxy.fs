// Based on "Galaxy Generator Study" by guinetik (technique from "Megaparsecs" by BigWings)
// https://www.shadertoy.com/view/scl3DH
// License: CC BY-NC-SA 3.0 Unported
// Modified: single galaxy, gradientLUT coloring, FFT per-ring brightness, AudioJones uniforms
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform int layers;
uniform float twist;
uniform float innerStretch;
uniform float ringWidth;
uniform float diskThickness;
uniform float tilt;
uniform float rotation;
uniform float orbitSpeed;
uniform float dustContrast;
uniform float starDensity;
uniform float starBright;
uniform float bulgeSize;
uniform float bulgeBright;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define GAL_TAU 6.2831853
#define GAL_MAX_RADIUS 1.5
#define GAL_MIN_COS_TILT 0.15
#define GAL_RING_PHASE_OFFSET 100.0
#define GAL_DUST_UV_SCALE 0.2
#define GAL_DUST_NOISE_FREQ 4.0
#define GAL_STAR_GLOW_RADIUS 0.5
#define GAL_SUPERNOVA_THRESH 0.9999
#define GAL_SUPERNOVA_MULT 10.0
#define GAL_INNER_RADIUS 0.1
#define GAL_OUTER_RADIUS 1.0
#define GAL_MAX_RINGS 25
#define GAL_RING_DECORR_A 563.2
#define GAL_RING_DECORR_B 673.2
#define GAL_STAR_OFFSET_A 17.3
#define GAL_STAR_OFFSET_B 31.7
#define GAL_TWINKLE_FREQ 784.0
#define GAL_SUPERNOVA_TIME_SCALE 0.05

float hashN2(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453123);
}

float valueNoise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hashN2(i + vec2(0.0, 0.0)), hashN2(i + vec2(1.0, 0.0)), u.x),
               mix(hashN2(i + vec2(0.0, 1.0)), hashN2(i + vec2(1.0, 1.0)), u.x), u.y);
}

mat2 galRot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

vec2 applyTilt(vec2 uv, float tiltAngle) {
    uv.y /= max(abs(cos(tiltAngle)), GAL_MIN_COS_TILT);
    return uv;
}

vec3 renderBulge(vec2 uv, float size, float brightness, vec3 tint) {
    return vec3(exp(-0.5 * dot(uv, uv) * size)) * brightness * tint;
}

vec3 renderRings(vec2 uv, float time) {
    vec3 col = vec3(0.0);
    float flip = 1.0;
    float t = time * orbitSpeed;

    for (int j = 0; j < GAL_MAX_RINGS; j++) {
        float i = float(j) / float(layers);
        if (i >= 1.0) break;
        flip *= -1.0;

        // FFT band lookup for this ring
        float t0 = float(j) / float(layers);
        float t1 = float(j + 1) / float(layers);
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

        // GradientLUT color for this ring
        vec3 dustCol = texture(gradientLUT, vec2(i, 0.5)).rgb;

        float z = mix(diskThickness, 0.0, i) * flip
                  * fract(sin(i * GAL_RING_DECORR_A) * GAL_RING_DECORR_B);
        float r = mix(GAL_INNER_RADIUS, GAL_OUTER_RADIUS, i);
        vec2 ringUv = uv + vec2(0.0, z * 0.5);
        vec2 st = ringUv * galRot(i * GAL_TAU * twist);
        st.x *= mix(innerStretch, 1.0, i);
        float ell = exp(-0.5 * abs(dot(st, st) - r) * ringWidth);
        vec2 texUv = GAL_DUST_UV_SCALE * st * galRot(i * GAL_RING_PHASE_OFFSET + t / r);
        vec3 dust = vec3(valueNoise2D((texUv + vec2(i)) * GAL_DUST_NOISE_FREQ));
        vec3 dL = pow(max(ell * dust / r, vec3(0.0)), vec3(0.5 + dustContrast));
        col += dL * dustCol * brightness;

        // Point Stars
        vec2 starId = floor(texUv * starDensity);
        vec2 starUv = fract(texUv * starDensity) - 0.5;
        float n = hashN2(starId + vec2(i * GAL_STAR_OFFSET_A, i * GAL_STAR_OFFSET_B));
        float starDist = length(starUv);
        float sL = smoothstep(GAL_STAR_GLOW_RADIUS, 0.0, starDist)
                   * pow(max(dL.r, 0.0), 2.0) * starBright
                   / max(starDist, 0.001);
        float sN = sL;
        sL *= sin(n * GAL_TWINKLE_FREQ + time) * 0.5 + 0.5;
        sL += sN * smoothstep(GAL_SUPERNOVA_THRESH, 1.0,
              sin(n * GAL_TWINKLE_FREQ + time * GAL_SUPERNOVA_TIME_SCALE))
              * GAL_SUPERNOVA_MULT;

        if (i > 3.0 / starDensity) {
            vec3 starCol = mix(dustCol, vec3(1.0), 0.3 + n * 0.5);
            col += sL * starCol * brightness;
        }
    }

    col /= float(layers);
    return col;
}

void main() {
    vec2 centered = fragTexCoord * resolution - resolution * 0.5;
    vec2 uv = centered / (min(resolution.x, resolution.y) * 0.5);

    // Apply in-plane rotation and tilt
    uv = applyTilt(uv * galRot(rotation), tilt);

    // Early-out for fragments beyond galaxy radius
    if (length(uv) > GAL_MAX_RADIUS) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 col = renderRings(uv, time);

    // Bulge: tinted toward warm white via gradientLUT center sample
    vec3 bulgeTint = mix(vec3(1.0, 0.9, 0.8),
                         texture(gradientLUT, vec2(0.0, 0.5)).rgb, 0.6);
    col += renderBulge(uv, bulgeSize, bulgeBright, bulgeTint);

    finalColor = vec4(col, 1.0);
}
