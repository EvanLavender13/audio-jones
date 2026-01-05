#version 330

// Iterated Möbius with depth accumulation
// Applies animated conformal transforms iteratively and blends samples at each depth

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform int iterations;
uniform float poleMagnitude;
uniform float rotation;      // CPU-accumulated rotation angle (radians)
uniform float uvScale;

out vec4 finalColor;

// Complex multiplication: (a.x + a.y*i) * (b.x + b.y*i)
vec2 cmul(vec2 a, vec2 b)
{
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

// Complex division: a / b with singularity protection
vec2 cdiv(vec2 a, vec2 b)
{
    float denom = max(dot(b, b), 0.0001);
    return vec2(a.x * b.x + a.y * b.y, a.y * b.x - a.x * b.y) / denom;
}

// Möbius transform: w = (az + b) / (cz + d)
vec2 mobius(vec2 z, vec2 a, vec2 b, vec2 c, vec2 d)
{
    vec2 numerator = cmul(a, z) + b;
    vec2 denominator = cmul(c, z) + d;
    return cdiv(numerator, denominator);
}

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    // Map UV from [0,1] to [-1,1] centered
    vec2 z = (fragTexCoord - 0.5) * 2.0;

    for (int i = 0; i < iterations; i++) {
        // Animate Möbius parameters per iteration
        float t = time + float(i) * 0.5;

        // a: rotating complex number (controls scale/rotation)
        // rotation is CPU-accumulated for smooth speed changes, scaled down for subtle effect
        float angle = rotation * 0.1 + float(i) * 0.5;
        vec2 a = vec2(cos(angle), sin(angle));

        // b: no translation
        vec2 b = vec2(0.0);

        // c: animated pole position (controls lensing/distortion)
        vec2 c = poleMagnitude * vec2(sin(t * 0.5), cos(t * 0.7));

        // d: identity denominator
        vec2 d = vec2(1.0, 0.0);

        z = mobius(z, a, b, c, d);

        // Smooth remap to UV space using sin() to avoid hard boundaries
        vec2 sampleUV = 0.5 + uvScale * sin(z * 0.5);

        // Depth-based weight: earlier iterations contribute less
        float weight = 1.0 / float(i + 2);
        colorAccum += texture(texture0, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    finalColor = vec4(colorAccum / weightAccum, 1.0);
}
