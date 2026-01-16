#version 330

// Bloom prefilter: Soft threshold extraction with knee smoothing
// Isolates bright pixels for subsequent blur passes

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float threshold;
uniform float knee;

out vec4 finalColor;

vec3 SoftThreshold(vec3 color)
{
    float brightness = max(color.r, max(color.g, color.b));
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);
    return color * contribution;
}

void main()
{
    vec3 color = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(SoftThreshold(color), 1.0);
}
