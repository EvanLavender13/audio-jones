// Based on "Evil Doom Spiral" by QnRA
// https://www.shadertoy.com/view/Nc2SRd
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized SDF primitive choice, exposed camera/spiral knobs,
//           gradient LUT replaces IQ palette, FFT brightness on shared t index
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float flyPhase;
uniform float cellPhase;

uniform int shapeType;
uniform float shapeSize;
uniform float cellPitchZ;

uniform int marchSteps;
uniform float stepFactor;
uniform float maxDist;
uniform float spiralRate;
uniform float tColorDistScale;
uniform float tColorIterScale;

uniform float fov;
uniform float pitchAngle;
uniform float rollAngle;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

mat2 rot2D(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

float sdfMap(vec3 p, float s, int shape) {
    if (shape == 0) {
        // Octahedron (bound form)
        vec3 q = abs(p);
        return (q.x + q.y + q.z - s) * 0.57735027;
    } else if (shape == 1) {
        // Box
        vec3 q = abs(p) - vec3(s);
        return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
    } else if (shape == 2) {
        // Sphere
        return length(p) - s;
    } else {
        // Torus (major = s, minor = s * 0.4)
        vec2 q = vec2(length(p.xz) - s, p.y);
        return length(q) - s * 0.4;
    }
}

float map(vec3 p) {
    vec3 ohPos = p;
    ohPos.z += flyPhase;
    ohPos.xy = fract(ohPos.xy) - 0.5;
    ohPos.z = mod(ohPos.z, cellPitchZ) - cellPitchZ * 0.5;
    ohPos.xy *= rot2D(cellPhase);
    ohPos.yz *= rot2D(cellPhase);
    return sdfMap(ohPos, shapeSize, shapeType);
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
    // Centered coords: (0,0) at screen center, y in [-1, 1].
    // Equivalent to the reference's `(fragCoord*2 - iResolution.xy)/iResolution.y`,
    // adapted to raylib's normalized fragTexCoord (Shadertoy fragCoord doesn't exist here).
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;

    vec3 ro = vec3(0.0, 0.0, -1.0);
    vec3 rd = normalize(vec3(uv * fov, 1.0));

    ro.yz *= rot2D(pitchAngle);
    rd.yz *= rot2D(pitchAngle);
    ro.yz *= rot2D(rollAngle);
    rd.yz *= rot2D(rollAngle);

    float dt = 0.0;
    int i;
    for (i = 0; i < marchSteps; i++) {
        vec3 p = ro + rd * dt;
        // Spiral the sample point as it marches -- the signature mechanic
        p.xy *= rot2D(dt * spiralRate);
        float d = map(p);
        if (d < 0.001 || dt > maxDist) {
            break;
        }
        dt += d * stepFactor;
    }

    // Shared color/audio index: same t drives gradient LUT and FFT lookup
    float t = fract(dt * tColorDistScale + float(i) * tColorIterScale);
    float bw = 1.0 / float(marchSteps);
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
    float brightness = sampleFFTBand(t, t + bw);

    finalColor = vec4(color * brightness, 1.0);
}
