#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float offset;
uniform int samples;
uniform float falloff;

void main() {
    vec2 center = vec2(0.5);
    vec2 toCenter = fragTexCoord - center;
    float dist = length(toCenter);

    // Stable radial direction with center fade to avoid direction instability
    vec2 dir = dist > 0.001 ? toCenter / dist : vec2(0.0);
    float intensity = smoothstep(0.0, 0.15, dist) * pow(dist, falloff);
    float minRes = min(resolution.x, resolution.y);

    vec3 colorSum = vec3(0.0);
    vec3 weightSum = vec3(0.0);

    for (int i = 0; i < samples; i++) {
        float t = float(i) / float(samples - 1);
        vec2 sampleOffset = dir * offset * intensity * (t - 0.5) * 2.0 / minRes;
        vec3 color = texture(texture0, fragTexCoord + sampleOffset).rgb;

        // Spectral weights: R peaks at t=1, G peaks at t=0.5, B peaks at t=0
        vec3 weight = vec3(t, 1.0 - abs(2.0 * t - 1.0), 1.0 - t);

        colorSum += color * weight;
        weightSum += weight;
    }

    finalColor = vec4(colorSum / weightSum, 1.0);
}
