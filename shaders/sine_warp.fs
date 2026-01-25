#version 330

// Turbulence Cascade with depth accumulation
// Stacks sine-based coordinate shifts with rotation to create organic swirl patterns

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform float rotation;
uniform int octaves;
uniform float strength;
uniform float octaveRotation;

out vec4 finalColor;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    // Map UV from [0,1] to [-1,1] centered
    vec2 p = (fragTexCoord - 0.5) * 2.0;

    for (int i = 0; i < octaves; i++) {
        float freq = pow(2.0, float(i));
        float amp = 1.0 / freq;

        // Animated sine-based shifts
        p.x += sin(p.y * freq + time) * amp * strength;
        p.y += sin(p.x * freq + time * 1.3) * amp * strength;

        // Rotation per octave
        float angle = float(i) * octaveRotation + rotation;
        float c = cos(angle);
        float s = sin(angle);
        p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);

        // Sample at this depth (mirror repeat for smooth boundaries)
        vec2 mapped = p * 0.5 + 0.5;
        vec2 sampleUV = 1.0 - abs(mod(mapped, 2.0) - 1.0);

        // Depth-based weight: higher frequencies contribute less
        float weight = amp;
        colorAccum += texture(texture0, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    finalColor = vec4(colorAccum / weightAccum, 1.0);
}
