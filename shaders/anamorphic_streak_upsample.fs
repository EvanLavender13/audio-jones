#version 330

// Anamorphic streak upsample: 3 horizontal taps with stretch-controlled lerp
// Blends wider mip into narrower mip — higher stretch favors wider blur

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // Wider mip being upsampled
uniform sampler2D highResTex; // Current narrower mip (from down chain)
uniform float texelSize;
uniform float stretch;

void main() {
    vec2 uv = fragTexCoord;

    vec3 upsampled = texture(texture0, uv + vec2(-1.5 * texelSize, 0.0)).rgb * 0.25;
    upsampled += texture(texture0, uv).rgb * 0.5;
    upsampled += texture(texture0, uv + vec2( 1.5 * texelSize, 0.0)).rgb * 0.25;

    vec3 highRes = texture(highResTex, uv).rgb;

    finalColor = vec4(mix(highRes, upsampled, stretch), 1.0);
}
