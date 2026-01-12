#version 330

// Turbulence Cascade with depth accumulation
// Stacks sine-based coordinate shifts with rotation to create organic swirl patterns

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform int octaves;
uniform float strength;
uniform float octaveRotation;
uniform float uvScale;
uniform bool polarFold;      // Enable polar pre-fold for radial symmetry
uniform int polarFoldSegments; // Wedge count for polar fold (2-12)

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const float PI = 3.14159265359;

vec2 doPolarFold(vec2 p, int segments)
{
    float radius = length(p);
    float angle = atan(p.y, p.x);

    // Fold angle into segment
    float segmentAngle = TWO_PI / float(segments);
    float halfSeg = PI / float(segments);
    float c = floor((angle + halfSeg) / segmentAngle);
    angle = mod(angle + halfSeg, segmentAngle) - halfSeg;
    angle *= mod(c, 2.0) * 2.0 - 1.0;  // Mirror alternating segments

    return vec2(cos(angle), sin(angle)) * radius;
}

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    // Map UV from [0,1] to [-1,1] centered
    vec2 p = (fragTexCoord - 0.5) * 2.0;

    // Polar fold before turbulence cascade
    if (polarFold) {
        p = doPolarFold(p, polarFoldSegments);
    }

    for (int i = 0; i < octaves; i++) {
        float freq = pow(2.0, float(i));
        float amp = 1.0 / freq;

        // Animated sine-based shifts
        p.x += sin(p.y * freq + time) * amp * strength;
        p.y += sin(p.x * freq + time * 1.3) * amp * strength;

        // Rotation per octave
        float angle = float(i) * octaveRotation + time * 0.1;
        float c = cos(angle);
        float s = sin(angle);
        p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);

        // Sample at this depth
        vec2 sampleUV = 0.5 + uvScale * sin(p * 0.3);

        // Depth-based weight: higher frequencies contribute less
        float weight = amp;
        colorAccum += texture(texture0, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    finalColor = vec4(colorAccum / weightAccum, 1.0);
}
