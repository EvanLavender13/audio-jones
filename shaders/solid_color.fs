// shaders/solid_color.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D colorLUT;

void main() {
    // Sample color from LUT at midpoint — uniform color across screen
    vec3 color = texture(colorLUT, vec2(0.5, 0.5)).rgb;
    finalColor = vec4(color, 1.0);
}
