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
uniform bool radialMode;
uniform bool depthBlend;

out vec4 finalColor;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 p = (fragTexCoord - 0.5) * 2.0;

    // Precompute original radius and direction for radial mode (avoids banding from recalculation)
    float origRadius = length(p);
    vec2 radialDir = (origRadius > 0.0001) ? p / origRadius : vec2(1.0, 0.0);

    for (int i = 0; i < octaves; i++) {
        float freq = pow(2.0, float(i));
        float amp = 1.0 / freq;

        if (radialMode) {
            // Radial sine displacement using original radius for consistent banding
            float displacement = sin(origRadius * freq + time) * amp * strength;
            p += radialDir * displacement;
        } else {
            // Existing Cartesian displacement
            p.x += sin(p.y * freq + time) * amp * strength;
            p.y += sin(p.x * freq + time * 1.3) * amp * strength;
        }

        // Per-octave rotation (applies to both modes)
        float angle = float(i) * octaveRotation + rotation;
        float c = cos(angle);
        float s = sin(angle);
        p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);

        if (depthBlend) {
            // Sample at this depth
            vec2 mapped = p * 0.5 + 0.5;
            vec2 sampleUV = 1.0 - abs(mod(mapped, 2.0) - 1.0);
            float weight = amp;
            colorAccum += texture(texture0, sampleUV).rgb * weight;
            weightAccum += weight;
        }
    }

    if (depthBlend) {
        finalColor = vec4(colorAccum / weightAccum, 1.0);
    } else {
        // Sample once at final position
        vec2 mapped = p * 0.5 + 0.5;
        vec2 sampleUV = 1.0 - abs(mod(mapped, 2.0) - 1.0);
        finalColor = texture(texture0, sampleUV);
    }
}
