// Triangle-Wave fBM Domain Warp
// Based on "Filaments" by nimitz (stormoid)
// https://www.shadertoy.com/view/4lcSWs
// License: CC BY-NC-SA 3.0 Unported
// Modified: Extracted noise warp as standalone radial post-process effect

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float noiseStrength; // Displacement magnitude (0.0-1.0)
uniform float time;          // Crease rotation angle (accumulated on CPU)

float tri(float x) { return abs(fract(x) - 0.5); }

vec2 tri2(vec2 p) {
    return vec2(tri(p.x + tri(p.y * 2.0)), tri(p.y + tri(p.x * 2.0)));
}

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

float triangleNoise(vec2 p) {
    float z  = 1.5;
    float z2 = 1.5;
    float rz = 0.0;
    vec2  bp = p * 0.8;
    for (int i = 0; i <= 3; i++) {
        vec2 dg = tri2(bp * 2.0) * 0.5;
        dg *= rot(time);                           // animate crease directions
        p  += dg / z2;                             // warp by crease gradient
        bp *= 1.5;
        z2 *= 0.6;
        z  *= 1.7;
        p  *= 1.2;
        p  *= mat2(0.970, 0.242, -0.242, 0.970);  // per-octave rotation
        rz += tri(p.x + tri(p.y)) / z;
    }
    return rz;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 p  = (uv - 0.5) * 2.0;                   // center at screen center, -1..1

    float nz = clamp(triangleNoise(p), 0.0, 1.0);
    p *= 1.0 + (nz - 0.5) * noiseStrength;        // radial push/pull

    uv = p * 0.5 + 0.5;                            // back to UV
    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
