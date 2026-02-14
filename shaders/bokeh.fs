#version 330

// Bokeh: Golden-angle Vogel disc blur with brightness weighting
// Bright spots bloom into soft circular highlights

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float radius;
uniform int iterations;
uniform float brightnessPower;
uniform int shape;
uniform float shapeAngle;
uniform int starPoints;
uniform float starInnerRadius;

out vec4 finalColor;

#define PI 3.141593
#define GOLDEN_ANGLE 2.39996323
#define HALF_PI 1.5707963

// Regular N-gon boundary: inscribed-circle-normalized
// Vertices at sector boundaries, edges at sector midpoints
float ngonRadius(float theta, int n, float rotation) {
    float halfAngle = PI / float(n);
    float sector = mod(theta + rotation, 2.0 * halfAngle) - halfAngle;
    return cos(halfAngle) / cos(sector);
}

// Star: linear interpolation between outer tips and inner valleys
// Tips at sector=0 (radius=1.0), valleys at sector=halfAngle (radius=innerRatio)
float starRadius(float theta, int n, float innerRatio, float rotation) {
    float halfAngle = PI / float(n);
    float sector = mod(theta + rotation, 2.0 * halfAngle);
    float t = abs(sector - halfAngle) / halfAngle;
    return mix(innerRatio, 1.0, t);
}

void main()
{
    vec2 uv = fragTexCoord;
    float aspect = resolution.y / resolution.x;

    vec3 acc = vec3(0.0);
    vec3 div = vec3(0.0);

    for (int i = 0; i < iterations; i++)
    {
        // Golden angle spiral direction
        float theta = float(i) * GOLDEN_ANGLE;
        vec2 dir = cos(theta + vec2(0.0, HALF_PI));

        // Square root distributes samples uniformly across disc area
        float r = sqrt(float(i) / float(iterations)) * radius;

        // shape: 0=Disc (no-op), 1=Box, 2=Hex, 3=Star
        if (shape == 1)      r *= ngonRadius(theta, 4, shapeAngle);
        else if (shape == 2) r *= ngonRadius(theta, 6, shapeAngle);
        else if (shape == 3) r *= starRadius(theta, starPoints, starInnerRadius, shapeAngle);

        // Aspect-corrected offset
        vec2 offset = dir * r;
        offset.x *= aspect;

        vec3 col = texture(texture0, uv + offset).rgb;

        // Brightness weighting - higher power = more "pop"
        vec3 weight = pow(col, vec3(brightnessPower));

        acc += col * weight;
        div += weight;
    }

    vec3 result = acc / max(div, vec3(0.001));
    float alpha = texture(texture0, uv).a;
    finalColor = vec4(result, alpha);
}
