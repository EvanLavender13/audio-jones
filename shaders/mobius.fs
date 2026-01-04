#version 330

// Möbius transformation post-process effect
// Applies conformal UV warp: w = (az + b) / (cz + d)

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

// Complex parameters a, b, c, d packed as vec2
uniform vec2 mobiusA;  // (aReal, aImag)
uniform vec2 mobiusB;  // (bReal, bImag)
uniform vec2 mobiusC;  // (cReal, cImag)
uniform vec2 mobiusD;  // (dReal, dImag)

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
    // Map UV from [0,1] to [-1,1] centered
    vec2 z = (fragTexCoord - 0.5) * 2.0;

    // Apply Möbius transformation
    vec2 w = mobius(z, mobiusA, mobiusB, mobiusC, mobiusD);

    // Map back to [0,1] UV space
    vec2 warpedUV = w * 0.5 + 0.5;

    // Sample with clamping for out-of-bounds
    finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
}
