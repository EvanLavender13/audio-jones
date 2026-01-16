#version 330

// Bloom downsample: Dual Kawase pattern
// Center (4x weight) + four diagonals (1x each)

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 halfpixel;

out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    vec3 sum = texture(texture0, uv).rgb * 4.0;
    sum += texture(texture0, uv + vec2(-halfpixel.x, -halfpixel.y)).rgb;
    sum += texture(texture0, uv + vec2( halfpixel.x, -halfpixel.y)).rgb;
    sum += texture(texture0, uv + vec2(-halfpixel.x,  halfpixel.y)).rgb;
    sum += texture(texture0, uv + vec2( halfpixel.x,  halfpixel.y)).rgb;
    finalColor = vec4(sum / 8.0, 1.0);
}
