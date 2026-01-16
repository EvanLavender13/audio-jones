#version 330

// Bloom upsample: Dual Kawase pattern
// Four edge samples (1x) + four diagonal corners (2x)

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 halfpixel;

out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    vec3 sum = vec3(0.0);

    sum += texture(texture0, uv + vec2(-halfpixel.x * 2.0, 0.0)).rgb;
    sum += texture(texture0, uv + vec2( halfpixel.x * 2.0, 0.0)).rgb;
    sum += texture(texture0, uv + vec2(0.0, -halfpixel.y * 2.0)).rgb;
    sum += texture(texture0, uv + vec2(0.0,  halfpixel.y * 2.0)).rgb;

    sum += texture(texture0, uv + vec2(-halfpixel.x,  halfpixel.y)).rgb * 2.0;
    sum += texture(texture0, uv + vec2( halfpixel.x,  halfpixel.y)).rgb * 2.0;
    sum += texture(texture0, uv + vec2(-halfpixel.x, -halfpixel.y)).rgb * 2.0;
    sum += texture(texture0, uv + vec2( halfpixel.x, -halfpixel.y)).rgb * 2.0;

    finalColor = vec4(sum / 12.0, 1.0);
}
