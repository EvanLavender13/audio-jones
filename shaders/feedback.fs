#version 330

// Feedback pass: sample previous frame with zoom + rotation + domain warp
// Creates MilkDrop-style infinite tunnel effect with organic flowing distortion

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float zoom;        // 0.9-1.0, lower = faster inward motion
uniform float rotation;    // radians per frame
uniform float desaturate;  // 0.0-1.0, higher = faster fade to gray

// Domain warp parameters
uniform float warpStrength;   // 0.0-0.2, distortion intensity
uniform float warpScale;      // 1.0-20.0, frequency of noise pattern
uniform int warpOctaves;      // 1-5, detail layers
uniform float warpLacunarity; // 1.5-3.0, frequency multiplier per octave
uniform float warpTime;       // animation time

out vec4 finalColor;

// Hash function for noise generation
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// Value noise with cubic interpolation, returns [-1, 1]
float noise2D(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Cubic Hermite interpolation

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y) * 2.0 - 1.0;
}

// Fractional Brownian motion - layered noise
vec2 fbm(vec2 p)
{
    vec2 offset = vec2(0.0);
    float amplitude = 1.0;
    float frequency = 1.0;

    for (int i = 0; i < warpOctaves; i++) {
        offset.x += noise2D(p * frequency + warpTime) * amplitude;
        offset.y += noise2D(p * frequency + warpTime + vec2(5.2, 1.3)) * amplitude;
        amplitude *= 0.5;
        frequency *= warpLacunarity;
    }
    return offset;
}

void main()
{
    vec2 uv = fragTexCoord;

    // Apply domain warp before rotation/zoom (compounds over frames)
    if (warpStrength > 0.0) {
        vec2 warpOffset = fbm(uv * warpScale);
        uv += warpOffset * warpStrength;
    }

    // Center UV for rotation/zoom transforms
    vec2 center = vec2(0.5);
    uv -= center;

    // Rotate around center
    float s = sin(rotation);
    float c = cos(rotation);
    uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

    // Scale toward center (zoom < 1.0 = zoom in)
    uv *= zoom;

    uv += center;

    // Sample with clamping to avoid edge artifacts
    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));

    // Fade trails toward luminance-matched dark gray
    float luma = dot(finalColor.rgb, vec3(0.299, 0.587, 0.114));
    finalColor.rgb = mix(finalColor.rgb, vec3(luma * 0.3), desaturate);
}
