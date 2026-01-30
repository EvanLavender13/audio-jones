#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float amplitude;
uniform float scale;
uniform int axes;
uniform float axisRotation;
uniform int harmonics;
uniform float decay;
uniform float drift;

const float PI = 3.14159265359;
const float TAU = 6.28318530718;

// Sum sine waves across all axes with alternating signs for interference nulls
float psi(float k, vec2 s) {
    float sum = 0.0;
    for (int i = 0; i < axes; i++) {
        float angle = -axisRotation + float(i) * PI / float(axes);
        vec2 dir = vec2(cos(angle), sin(angle));
        float sign = (i == 0) ? 1.0 : -1.0;
        sum += sign * sin(k * dot(s, dir));
    }
    return sum;
}

// Time-varying phase rotation per harmonic
vec2 omega(float k, float t) {
    float p = pow(k, drift) * t;
    return vec2(sin(p), cos(p));
}

// Harmonic summation with amplitude decay
vec2 phi(vec2 s, float t) {
    vec2 p = vec2(0.0);
    for (int j = 1; j < harmonics; j++) {
        float k = float(j) * TAU;
        float amp = pow(k, -decay);
        p += omega(k, t) * psi(k, s) * amp;
    }
    return p;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 centered = (uv - 0.5) * scale;
    vec2 displacement = phi(centered, time) * amplitude;
    vec2 displaced = uv + displacement;

    // Mirror at boundaries
    displaced = 1.0 - abs(mod(displaced, 2.0) - 1.0);

    finalColor = texture(texture0, displaced);
}
