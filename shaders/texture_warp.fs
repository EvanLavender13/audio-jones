#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float strength;       // Warp magnitude per iteration
uniform int iterations;       // Cascade depth

void main()
{
    vec2 warpedUV = fragTexCoord;

    for (int i = 0; i < iterations; i++) {
        vec3 sample = texture(texture0, warpedUV).rgb;
        vec2 offset = (sample.rg - 0.5) * 2.0;
        warpedUV += offset * strength;
    }

    finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
}
