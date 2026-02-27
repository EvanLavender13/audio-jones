// Based on "Inside a Prism" by piratehurrdurr
// https://www.shadertoy.com/view/ldy3WG
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, gradient LUT recoloring, hue extraction, orbit/FOV controls

#version 330

in vec2 fragTexCoord;

uniform vec2 resolution;
uniform float cameraTime;
uniform float structureTime;
uniform float displacementScale;
uniform float stepSize;
uniform int iterations;
uniform float orbitRadius;
uniform float fov;
uniform float brightness;
uniform float saturationPower;
uniform sampler2D gradientLUT;

out vec4 finalColor;

vec3 value(vec3 pos, float phase) {
    return vec3(sin(pos.x + phase), sin(pos.y + phase), sin(pos.z + phase));
}

vec3 scan(vec3 pos, vec3 dir, float phase) {
    float itf = float(iterations);
    float str = 1.5 / sqrt(itf);
    vec3 c = vec3(0.5);
    for (int i = 0; i < iterations; i++) {
        float f = (1.0 - float(i) / itf) * str;

        vec3 posMod = value(pos, phase);
        vec3 newPos;
        newPos.x = posMod.y * posMod.z;
        newPos.y = posMod.x * posMod.z;
        newPos.z = posMod.x * posMod.y;

        c += value(pos + newPos * displacementScale, phase) * f;
        pos += dir * stepSize;
    }
    return c;
}

void main() {
    vec2 uv = (gl_FragCoord.xy - resolution * 0.5) / resolution.y;

    // Camera — Lissajous orbit with irrational frequency ratios
    float a = cameraTime;
    vec3 pos = vec3(cos(a / 4.0 + sin(a / 2.0)), cos(a * 0.2), sin(a * 0.31) / 4.5) * orbitRadius;
    vec3 n = normalize(pos + vec3(cos(a * 2.3), cos(a * 2.61), cos(a * 1.62)));
    vec3 crossRight = normalize(cross(n, vec3(0.0, 0.0, 1.0)));
    vec3 crossUp = normalize(cross(n, crossRight));
    n = n * fov + crossRight * uv.x + crossUp * uv.y;

    // Ray march
    vec3 raw = scan(pos, normalize(n), structureTime);

    // Hue extraction from raw accumulation
    float gray = (raw.r + raw.g + raw.b) / 3.0;
    float sat = abs(raw.r - gray) + abs(raw.g - gray) + abs(raw.b - gray);
    float hue = atan(raw.g - raw.b, raw.r - (raw.g + raw.b) * 0.5) / (2.0 * 3.14159265) + 0.5;
    float bright = pow(sat, saturationPower);

    // Final color
    vec3 color = texture(gradientLUT, vec2(hue, 0.5)).rgb * bright * brightness;
    finalColor = vec4(color, 1.0);
}
