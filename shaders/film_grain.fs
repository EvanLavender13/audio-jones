// CeeJay FilmGrain - Box-Muller Gaussian noise with SNR power curve
// https://github.com/killashok/GShade-Shaders/blob/master/Shaders/FilmGrain.fx
// Modified: HLSL to GLSL, per-channel color noise via seed offsets

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float intensity;
uniform float variance;
uniform float snr;
uniform float colorAmount;

const float PI = 3.1415927;

// Box-Muller: uniform hash pair -> Gaussian sample centered at 0.5
float boxMuller(float s, float t, float v) {
    float sine = sin(s);
    float cosine = cos(s);
    float u1 = fract(sine * 43758.5453 + t);
    float u2 = fract(cosine * 53758.5453 - t);

    u1 = max(u1, 0.0001);
    float r = sqrt(-log(u1));
    float theta = 2.0 * PI * u2;
    return v * r * cos(theta) + 0.5;
}

void main() {
    vec3 color = texture(texture0, fragTexCoord).rgb;

    // Inverted luminance: 1.0 in shadows, 0.0 in highlights
    float inv_luma = dot(color, vec3(-1.0 / 3.0)) + 1.0;

    // Luminance-dependent variance via SNR power curve
    float stn = (snr > 0.0) ? pow(abs(inv_luma), snr) : 1.0;
    float v = (variance * variance) * stn;

    // Per-pixel seed from UV (hash, not spatial - centering convention does not apply)
    float seed = dot(fragTexCoord, vec2(12.9898, 78.233));

    // 3 independent Gaussian samples for per-channel color noise
    float gaussR = boxMuller(seed, time, v);
    float gaussG = boxMuller(seed + 1.0, time, v);
    float gaussB = boxMuller(seed + 2.0, time, v);

    // Mono vs per-channel
    vec3 gauss = mix(vec3(gaussR), vec3(gaussR, gaussG, gaussB), colorAmount);

    // Multiplicative grain blend
    vec3 grain = mix(vec3(1.0 + intensity), vec3(1.0 - intensity), gauss);
    color *= grain;

    finalColor = vec4(color, 1.0);
}
