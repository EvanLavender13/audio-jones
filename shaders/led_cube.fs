// Based on "LED Cube Visualizer" by orblivius
// https://www.shadertoy.com/view/7fsXWj
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT audio reactivity, gradient LUT coloring,
// walls and volumetric fog removed

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform int gridSize;
uniform float tracerPhase;
uniform float rotPhaseA;
uniform float rotPhaseB;
uniform float driftPhase;
uniform float glowFalloff;
uniform float displacement;
uniform float cubeScale;
uniform float fovDiv;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float TAU = 6.28318530718;
const float INV_SQRT_3 = 0.57735; // Max distance in unit cube

// Maps a continuous phase value to a position on the unit cube's 12-edge
// skeleton. One full cycle = phase advancing by 12.0.
vec3 getCubeEdgePosition(float beat) {
    float e = mod(beat, 12.0);
    float t = fract(beat);
    if (e < 1.0)  return vec3(t, 0.0, 0.0);
    if (e < 2.0)  return vec3(1.0, t, 0.0);
    if (e < 3.0)  return vec3(1.0 - t, 1.0, 0.0);
    if (e < 4.0)  return vec3(0.0, 1.0 - t, 0.0);
    if (e < 5.0)  return vec3(0.0, 0.0, t);
    if (e < 6.0)  return vec3(t, 0.0, 1.0);
    if (e < 7.0)  return vec3(1.0, t, 1.0);
    if (e < 8.0)  return vec3(1.0 - t, 1.0, 1.0);
    if (e < 9.0)  return vec3(0.0, 1.0 - t, 1.0);
    if (e < 10.0) return vec3(1.0, 0.0, 1.0 - t);
    if (e < 11.0) return vec3(1.0, 1.0, t);
    return vec3(0.0, 1.0, 1.0 - t);
}

void main() {
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 uv = (2.0 * fragCoord - resolution) / resolution.x;

    // Camera z derived from cubeScale to preserve reference framing
    // (ref: CAM_Z = -3.8 at CUBE_SCALE = 4.5; ratio 3.8 / 4.5 = 0.845).
    vec3 camRo = vec3(0.0, 0.0, -cubeScale * 0.845);

    mat2 R1 = mat2(cos(rotPhaseA), -sin(rotPhaseA),
                   sin(rotPhaseA),  cos(rotPhaseA));
    mat2 R2 = mat2(cos(rotPhaseB), -sin(rotPhaseB),
                   sin(rotPhaseB),  cos(rotPhaseB));

    vec3 sp = getCubeEdgePosition(tracerPhase);
    vec3 toCenter = normalize(vec3(0.5) - sp);

    float st = 1.0 / float(gridSize);
    // Midpoint of xyz = (gridSize-1) / (2*gridSize) = 0.5 - st/2.
    float centerOffset = 0.5 - st * 0.5;

    vec3 O = vec3(0.0);

    for (int iz = 0; iz < gridSize; iz++) {
      for (int iy = 0; iy < gridSize; iy++) {
        for (int ix = 0; ix < gridSize; ix++) {
            vec3 xyz = vec3(float(ix), float(iy), float(iz)) * st;

            vec3 diff = xyz - sp;
            float d0 = length(diff);
            float ap = clamp(d0 * INV_SQRT_3, 0.0, 1.0);

            float df = max(0.0, dot(diff / (d0 + 1e-5), toCenter));

            float freq = baseFreq * pow(maxFreq / baseFreq, ap);
            float bin = freq / (sampleRate * 0.5);
            float energy = 0.0;
            if (bin <= 1.0) {
                energy = texture(fftTexture, vec2(bin, 0.5)).r;
            }
            float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
            float brightness = baseBright + mag * (1.0 - ap);
            brightness *= 0.5 + (1.0 - df);

            vec3 disp = vec3(sin(xyz.y * TAU + driftPhase),
                             cos(xyz.x * TAU + driftPhase * 0.7),
                             sin(xyz.z * TAU + driftPhase * 1.3)) * displacement;

            vec3 p = xyz - vec3(centerOffset) + disp;
            p.yx *= R1;
            p.zy *= R2;
            p *= cubeScale;

            vec3 pRel = p - camRo;
            if (pRel.z < 0.1) continue;
            vec2 pProj = pRel.xy / (pRel.z * fovDiv);
            float d = length(uv - pProj);

            vec3 color = texture(gradientLUT, vec2(ap, 0.5)).rgb;
            O += color * brightness * exp(-d * d / (glowFalloff * glowFalloff));
        }
      }
    }

    finalColor = vec4(O, 1.0);
}
