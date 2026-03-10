// Based on "Gltch" by Xor (@XorDev)
// https://www.shadertoy.com/view/mdfGRs
// License: CC BY-NC-SA 3.0 Unported
// Modified: samples input texture with spectral tinting instead of procedural color

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform int iterations;
uniform float zoom;
uniform float aberrationSpread;
uniform float noiseScale;
uniform float rotationLevels;
uniform float softness;
uniform float mixAmount;

// Cell noise — each cell reshuffles at its own staggered time offset
// so blocks pop independently rather than all at once.
float hash21(vec2 p) {
    float base = dot(p, vec2(127.1, 311.7));
    float phase = fract(sin(base * 1.731) * 21345.6789);
    return fract(sin(base + floor(time + phase)) * 43758.5453);
}

#define N(x) hash21(floor((x) * 64.0 / noiseScale))

void main() {
    float angleStep = 6.2831853 / rotationLevels;
    float invIter = 1.0 / float(iterations);
    vec3 O = vec3(0.0);
    vec2 aspect = vec2(resolution.x / resolution.y, 1.0);

    for (int j = 0; j < iterations; j++) {
        // Map j to i in [-1, +1]
        float i = float(j) * invIter * 2.0 - 1.0;

        // Centered coordinates with per-iteration aberration offset
        vec2 c = (fragTexCoord - 0.5) * aspect / (zoom + i * aberrationSpread);

        // Noise-driven coordinate division
        c /= (0.1 + N(c));

        // Quantized rotation angle from cell hash
        float angle = ceil(N(c) * rotationLevels) * angleStep;
        vec2 rotated = c * mat2(cos(angle + vec4(0, 33, 11, 0)));

        // Frequency stripe mask
        float cosVal = cos(rotated.x / N(N(c) + ceil(c) + time));
        float mask = smoothstep(-softness, softness + 0.001, cosVal);

        // Cell-driven UV displacement: use hash to offset sampling position
        // independent of screen position so center blocks displace too
        float cellHash = N(c + vec2(31.0, 17.0));
        vec2 offset = vec2(cos(angle), sin(angle)) * (cellHash - 0.5)
                       * aberrationSpread * 20.0;
        vec2 sampleUV = clamp(fragTexCoord + offset + vec2(i * aberrationSpread), 0.0, 1.0);

        vec3 texSample = texture(texture0, sampleUV).rgb;

        // Spectral tint: red at i=-1, green at i=0, blue at i=1
        vec3 spectral = vec3(1.0 + i, 2.0 - abs(2.0 * i), 1.0 - i);

        O += texSample * spectral * mask * invIter;
    }

    vec3 original = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(mix(original, O, mixAmount), 1.0);
}
