#version 330

// Wave Drift - multi-mode wave basis (triangle, sine, sawtooth, square)
// Stacks selectable waveform coordinate shifts with rotation to create organic swirl patterns

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
uniform int waveType; // 0=triangle, 1=sine, 2=sawtooth, 3=square

out vec4 finalColor;

float wave(float x) {
    if (waveType == 0) return (abs(fract(x / 6.283185) - 0.5) - 0.25) * 4.0; // triangle [-1, 1]
    if (waveType == 1) return sin(x);                                          // sine     [-1, 1]
    if (waveType == 2) return fract(x / 6.283185) * 2.0 - 1.0;               // sawtooth [-1, 1]
    return sign(sin(x));                                                       // square   [-1, 1]
}

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
            // Radial wave displacement using original radius for consistent banding
            float displacement = wave(origRadius * freq + time) * amp * strength;
            p += radialDir * displacement;
        } else {
            // Cartesian wave displacement
            p.x += wave(p.y * freq + time) * amp * strength;
            p.y += wave(p.x * freq + time * 1.3) * amp * strength;
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
