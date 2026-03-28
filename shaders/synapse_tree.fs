// Based on "Arvore 3b - sinapses" by Elsio
// https://www.shadertoy.com/view/XfXcR7
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized constants, gradient LUT synapse coloring,
// camera orbit.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float animPhase;
uniform int marchSteps;
uniform int foldIterations;
uniform float fov;
uniform int branchShape;
uniform int foldType;
uniform float foldOffset;
uniform float branchThickness;
uniform float orbitAngle;
uniform float synapseIntensity;
uniform float synapseBounceFreq;
uniform float synapsePulseFreq;
uniform float colorPhase;
uniform sampler2D gradientLUT;

mat2 rot(float a) {
    vec4 v = cos(a + vec4(0.0, 11.0, 33.0, 0.0));
    return mat2(v.x, v.y, v.z, v.w);
}

vec2 applyFold(vec2 v) {
    if (foldType == 1) {
        // Box fold (Dream Fractal mode 0, adapted to 2D)
        return clamp(v, -foldOffset, foldOffset) * 2.0 - v;
    }
    if (foldType == 2) {
        // Triangle fold - periodic reflection
        return foldOffset - abs(mod(v + foldOffset, foldOffset * 2.0) - foldOffset);
    }
    // Abs fold (original)
    return abs(v) - foldOffset;
}

float branchDist(vec3 p) {
    if (branchShape == 1) { return max(abs(p.x), abs(p.z)); }
    if (branchShape == 2) { return abs(p.x) + abs(p.z); }
    if (branchShape == 3) { return max(length(p.xz), min(abs(p.x), abs(p.z)) * 2.0); }
    return length(p.xz);
}

void main() {
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 u = (fragCoord - resolution * 0.5) / resolution.y;

    float i, d = 0.0, s, a, c, cyl;
    float j;
    vec3 color = vec3(0.0);

    // Camera orbit
    float co = cos(orbitAngle), so = sin(orbitAngle);
    vec3 camOff = vec3(-0.1, 0.7, -0.2);
    camOff.xz = mat2(co, -so, so, co) * camOff.xz;

    float glowNorm = 8.0 / float(foldIterations);

    for (i = 0.0; i < float(marchSteps); i += 1.0) {
        vec3 p = vec3(d * u * fov, d * 1.8) + camOff;

        j = 8.45;
        s = 8.45;
        a = 8.45;

        int minFoldIdx = 0;
        float minCyl = 1e10;
        float jEnd = 8.45 + float(foldIterations);
        int foldIdx = 0;
        while (j < jEnd) {
            j += 1.0;
            c = dot(p, p);
            p /= c + 0.005;
            a /= c;
            p.xz = applyFold(rot(sin(animPhase - 1.0 / c) / a - j) * p.xz);
            p.y = 1.78 - p.y;
            cyl = branchDist(p) * 2.5 - branchThickness / c;
            cyl = max(cyl, p.y) / a;
            if (cyl < minCyl) {
                minCyl = cyl;
                minFoldIdx = foldIdx;
            }
            s = min(s, cyl) * 0.9;
            foldIdx++;
        }

        d += abs(s) + 1e-6;

        // Glow - accumulates inversely with distance
        if (s < 0.001) {
            color += glowNorm * 0.000001 / d * i;
        }

        // Synapse pulse - LUT-colored glow at bounce intersection
        if (cyl > length(p - vec3(0.0, sin(synapseBounceFreq * animPhase), 0.0))
                  - 0.5 - 0.5 * cos(synapsePulseFreq * animPhase)) {
            float foldT = float(minFoldIdx) / float(max(foldIterations - 1, 1));
            vec3 foldColor = texture(gradientLUT, vec2(fract(foldT + colorPhase), 0.5)).rgb;
            if (s < 0.001) {
                color += foldColor * synapseIntensity * glowNorm * 0.000005 / d * i;
            }
            if (s < 0.02) {
                color += foldColor * synapseIntensity * glowNorm * 0.0002 / d;
            }
        }
    }

    finalColor = vec4(color, 1.0);
}
