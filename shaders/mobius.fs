#version 330

// Mobius Transform: conformal UV warp via complex-plane fractional linear transformation
// Operates in log-polar space around midpoint between fixed points

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform vec2 point1;
uniform vec2 point2;
uniform float rho;
uniform float alpha;

out vec4 finalColor;

// Complex multiplication: (a + bi)(c + di) = (ac - bd) + (ad + bc)i
vec2 cmul(vec2 a, vec2 b)
{
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

// Complex division: (a + bi)/(c + di)
vec2 cdiv(vec2 a, vec2 b)
{
    return vec2(a.x * b.x + a.y * b.y, -a.x * b.y + a.y * b.x) / dot(b, b);
}

// Hyperbolic transform: scale in log-polar space
vec2 trans_hyperbolic(vec2 p, float r)
{
    float len = length(p);
    if (len < 0.0001) return p;
    float logR = log(len) - r;
    return normalize(p) * exp(logR);
}

// Elliptic transform: rotation around origin
vec2 trans_elliptic(vec2 p, float a)
{
    float c = cos(a);
    float s = sin(a);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

// Loxodromic transform: combines hyperbolic scaling and elliptic rotation
vec2 trans_loxodromic(vec2 p, float r, float a)
{
    p = trans_hyperbolic(p, r);
    p = trans_elliptic(p, a);
    return p;
}

void main()
{
    vec2 uv = fragTexCoord;

    // Center UV to [-0.5, 0.5] range
    vec2 centered = uv - 0.5;

    // Compute midpoint between fixed points
    vec2 mid = (point1 + point2) * 0.5 - 0.5;

    // Translate to midpoint-centered space
    vec2 p = centered - mid;

    // Apply animated loxodromic transform
    float animRho = time * rho;
    float animAlpha = time * alpha;
    p = trans_loxodromic(p, animRho, animAlpha);

    // Translate back
    p = p + mid;

    // Convert back to UV space
    vec2 warpedUV = p + 0.5;

    finalColor = texture(texture0, warpedUV);
}
