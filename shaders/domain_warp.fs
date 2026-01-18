#version 330

// Domain Warp: Nested fbm-based UV displacement (Inigo Quilez technique)
// Iteratively warps coordinates through fbm layers for organic terrain patterns

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float warpStrength;   // Overall displacement magnitude (0.0 to 0.5)
uniform float warpScale;      // Noise frequency (1.0 to 10.0)
uniform int warpIterations;   // Nesting depth (1 to 3)
uniform float falloff;        // Amplitude reduction per iteration (0.3 to 0.8)
uniform vec2 timeOffset;      // Animated drift (accumulated on CPU)

out vec4 finalColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Smoothstep interpolation

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

vec2 fbm2(vec2 p) {
    return vec2(
        fbm(p + vec2(0.0, 0.0)),
        fbm(p + vec2(5.2, 1.3))
    ) * 2.0 - 1.0;  // Center around 0 for bidirectional displacement
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 p = uv * warpScale + timeOffset;

    float amp = warpStrength;
    for (int i = 0; i < warpIterations; i++) {
        vec2 displacement = fbm2(p);
        uv += displacement * amp;
        p = uv * warpScale + timeOffset + vec2(1.7 * float(i), 9.2 * float(i));
        amp *= falloff;
    }

    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
