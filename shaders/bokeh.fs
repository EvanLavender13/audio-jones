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

out vec4 finalColor;

#define GOLDEN_ANGLE 2.39996323
#define HALF_PI 1.5707963

void main()
{
    vec2 uv = fragTexCoord;
    float aspect = resolution.y / resolution.x;

    vec3 acc = vec3(0.0);
    vec3 div = vec3(0.0);

    for (int i = 0; i < iterations; i++)
    {
        // Golden angle spiral direction
        vec2 dir = cos(float(i) * GOLDEN_ANGLE + vec2(0.0, HALF_PI));

        // Square root distributes samples uniformly across disc area
        float r = sqrt(float(i) / float(iterations)) * radius;

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
