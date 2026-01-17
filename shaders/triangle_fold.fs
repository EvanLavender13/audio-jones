#version 330

// Triangle Fold: Sierpinski-style UV transform creating 3-fold/6-fold gasket patterns

in vec2 fragTexCoord;

uniform sampler2D texture0;

uniform int iterations;       // 1-8
uniform float scale;          // Expansion per iteration (1.5-3.0)
uniform vec2 triangleOffset;  // Translation after fold
uniform float rotation;       // Global rotation (accumulated)
uniform float twistAngle;     // Per-iteration rotation (accumulated)

out vec4 finalColor;

const float SQRT3 = 1.7320508;

vec2 rotate2d(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

vec2 mirror(vec2 x)
{
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

vec2 triangleFold(vec2 p)
{
    // Fold 1: Reflect across Y axis (creates left-right symmetry)
    p.x = abs(p.x);

    // Fold 2: Reflect across 60-degree line (y = sqrt(3) * x)
    // Normal to 60-degree line: n = normalize(sqrt(3), -1) = (sqrt(3)/2, -0.5)
    vec2 n = vec2(SQRT3, -1.0) * 0.5;
    float d = dot(p, n);
    if (d > 0.0) {
        p -= 2.0 * d * n;
    }

    return p;
}

void main()
{
    vec2 uv = fragTexCoord - 0.5;
    vec2 p = uv * 2.0;

    // Global rotation
    p = rotate2d(p, rotation);

    // Triangle fold iteration
    for (int i = 0; i < iterations; i++) {
        p = triangleFold(p);

        // Scale and translate
        p = p * scale - triangleOffset;

        // Per-iteration rotation
        p = rotate2d(p, twistAngle);
    }

    // Sample texture at transformed position
    vec2 newUV = mirror(p * 0.5 + 0.5);
    finalColor = texture(texture0, newUV);
}
