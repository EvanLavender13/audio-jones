#version 330

// Density Wave Spiral: Lin-Shu density wave theory UV warp
// Concentric ellipse rings with differential rotation create spiral arm structure

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform vec2 center;         // Galaxy center offset from screen center
uniform vec2 aspect;         // Ellipse eccentricity (x, y)
uniform float tightness;     // Arm winding (radians), negative = trailing
uniform float rotationAccum; // CPU-accumulated rotation
uniform float thickness;     // Ring edge softness
uniform int ringCount;       // Number of concentric rings
uniform float falloff;       // Brightness decay exponent

out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord - 0.5 - center;
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < ringCount; i++) {
        float l = 0.1 + float(i) * (3.0 / float(ringCount));
        float n = float(i) + 1.0;

        // Apply tilt based on radius (creates spiral arms)
        float angle = tightness + tightness * l;
        float c = cos(angle), s = sin(angle);
        vec2 V = mat2(c, -s, s, c) * uv;
        V /= aspect;

        // Calculate ring mask via ellipse distance
        float d = dot(V, V);
        float ring = smoothstep(thickness, 0.0, abs(sqrt(d) - l));

        // Sample texture with differential rotation (inner rings faster)
        float va = rotationAccum / l;
        float c2 = cos(va + n), s2 = sin(va + n);
        vec2 sampleUV = mat2(c2, -s2, s2, c2) * (0.5 * V / l);
        sampleUV = sampleUV * 0.5 + 0.5;

        // Mirror repeat for smooth boundaries
        sampleUV = 1.0 - abs(mod(sampleUV, 2.0) - 1.0);

        vec4 texColor = texture(texture0, sampleUV);

        // Accumulate with distance falloff
        float weight = ring / pow(l, falloff);
        result += weight * texColor;
        totalWeight += weight;
    }

    // Normalize by total coverage
    float coverage = clamp(totalWeight, 0.0, 1.0);
    vec4 spiral = (totalWeight > 0.001) ? result / totalWeight : vec4(0.0);

    // Dark space between rings - no original texture
    vec4 space = vec4(0.0, 0.0, 0.0, 1.0);

    // Blend: dark space in gaps, spiral on rings
    finalColor = mix(space, spiral, coverage);
}
