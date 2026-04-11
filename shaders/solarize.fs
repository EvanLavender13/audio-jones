// Suricrasia Online - Interpolatable Colour Inversion
// https://suricrasia.online/blog/interpolatable-colour-inversion/
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float amount;
uniform float threshold;

vec3 solar_invert(vec3 color, float x) {
    float st = 1.0 - step(0.5, x);
    return abs((color - st) * (2.0 * x + 4.0 * st - 3.0) + 1.0);
}

void main() {
    vec2 uv = fragTexCoord;
    vec4 color = texture(texture0, uv);

    vec3 shifted = clamp(color.rgb + (threshold - 0.5) * 2.0, 0.0, 1.0);
    vec3 result = solar_invert(shifted, amount);

    finalColor = vec4(result, color.a);
}
