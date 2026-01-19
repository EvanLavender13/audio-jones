#version 330

// Wave Ripple: pseudo-3D radial wave displacement
// Summed sine waves create height field; gradient displaces UVs for parallax

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform int octaves;
uniform float strength;
uniform float frequency;
uniform float steepness;
uniform float centerHole;
uniform vec2 origin;
uniform bool shadeEnabled;
uniform float shadeIntensity;

out vec4 finalColor;

float getHeight(float dist, float t, int oct, float baseFreq)
{
    float height = 0.0;
    float freq = baseFreq;
    float amp = 1.0;

    for (int i = 0; i < oct; i++) {
        height += sin(dist * freq - t) * amp;
        freq *= 2.0;
        amp *= 0.5;
    }
    return height;
}

float getSlope(float dist, float t, int oct, float baseFreq)
{
    float slope = 0.0;
    float freq = baseFreq;
    float amp = 1.0;

    for (int i = 0; i < oct; i++) {
        slope += cos(dist * freq - t) * amp;
        freq *= 2.0;
        amp *= 0.5;
    }
    return slope;
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 delta = uv - origin;
    float dist = length(delta);

    float height = getHeight(dist, time, octaves, frequency);

    // Gerstner asymmetry: sharper crests, flatter troughs
    float shaped = height - steepness * height * height;

    // Center hole: fade waves to zero within the hole radius
    if (centerHole > 0.0) {
        shaped *= smoothstep(0.0, centerHole, dist);
    }

    // Displace UV along radial direction
    vec2 displaced = uv;
    if (dist > 0.001) {
        vec2 dir = delta / dist;
        displaced = uv + dir * shaped * strength;
    }

    vec4 color = texture(texture0, displaced);

    // Slope-based shading: light/dark on wave slopes, neutral at crests/troughs
    if (shadeEnabled) {
        float slope = getSlope(dist, time, octaves, frequency);
        float shade = 1.0 + slope * shadeIntensity;
        color.rgb *= shade;
    }

    finalColor = color;
}
