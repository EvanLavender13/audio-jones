#version 330

// Mobius Transform with animated log-polar spiral
// Based on https://www.shadertoy.com/view/4sGXDK

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 point1;
uniform vec2 point2;
uniform float time;
uniform float spiralTightness;
uniform float zoomFactor;

out vec4 finalColor;

void main()
{
    vec2 U = fragTexCoord - 0.5;

    vec2 z1 = point1 - 0.5;
    vec2 z2 = point2 - 0.5;

    // Mobius transform (pole at point2)
    vec2 z = U - z1;
    U -= z2;
    float d = dot(U, U);
    if (d < 0.0001) {
        finalColor = vec4(0.0);
        return;
    }
    U = vec2(z.x * U.x + z.y * U.y, z.y * U.x - z.x * U.y) / d;

    float len = length(U);
    if (len < 0.0001) {
        finalColor = vec4(0.0);
        return;
    }

    // Log-polar spiral with animation
    U = log(len) * vec2(spiralTightness, zoomFactor) + time * 0.25
      + atan(U.y, U.x) / 6.283 * vec2(5.0, 1.0);

    finalColor = texture(texture0, fract(U));
}
