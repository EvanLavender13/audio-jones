#version 330

// Domain Warp: fbm-based UV displacement with animated drift
// Applies layered Perlin-style noise to create organic, flowing distortions

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float strength;    // Warp magnitude
uniform int octaves;       // Fractal octaves
uniform float lacunarity;  // Frequency multiplier per octave
uniform float persistence; // Amplitude decay per octave
uniform float scale;       // Base noise frequency
uniform float drift;       // Accumulated animation phase

out vec4 finalColor;

// Hash function for noise generation (better distribution than sin-based)
vec2 hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy) * 2.0 - 1.0;
}

// Gradient noise
float gradientNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    // Quintic interpolation curve
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    // Four corner gradients
    float n00 = dot(hash22(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0));
    float n10 = dot(hash22(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0));
    float n01 = dot(hash22(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0));
    float n11 = dot(hash22(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0));

    // Bilinear interpolation
    return mix(mix(n00, n10, u.x), mix(n01, n11, u.x), u.y);
}

// Fractal Brownian Motion (fbm)
float fbm(vec2 p, int numOctaves, float lac, float pers) {
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float totalAmplitude = 0.0;

    for (int i = 0; i < numOctaves; i++) {
        value += amplitude * gradientNoise(p * frequency);
        totalAmplitude += amplitude;
        frequency *= lac;
        amplitude *= pers;
    }

    return value / totalAmplitude;
}

// Domain warp: displace coordinates using two offset fbm evaluations
vec2 domainWarp(vec2 p, int numOctaves, float lac, float pers, float time) {
    // Offset vectors for independent X and Y warping
    vec2 offsetA = vec2(1.7, 9.2);
    vec2 offsetB = vec2(8.3, 2.8);
    vec2 timeOffset = vec2(time * 0.7, time * 0.9);

    float warpX = fbm(p + offsetA + timeOffset, numOctaves, lac, pers);
    float warpY = fbm(p + offsetB + timeOffset * 1.2, numOctaves, lac, pers);

    return vec2(warpX, warpY);
}

void main()
{
    vec2 uv = fragTexCoord;

    // Scale UV for noise frequency
    vec2 noiseCoord = uv * scale;

    // Compute warp displacement
    vec2 warp = domainWarp(noiseCoord, octaves, lacunarity, persistence, drift);

    // Apply displacement
    vec2 warpedUV = uv + warp * strength;

    // Clamp to valid texture range (or use mirror repeat logic if preferred)
    warpedUV = clamp(warpedUV, 0.0, 1.0);

    finalColor = texture(texture0, warpedUV);
}
