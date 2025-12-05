#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float halfLife;   // Trail persistence in seconds (e.g., 0.5 = trails last ~0.5 sec)
uniform float deltaTime;  // Time between frames in seconds

out vec4 finalColor;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);

    // Framerate-independent exponential decay using half-life formula
    float safeHalfLife = max(halfLife, 0.001);
    float decayMultiplier = exp(-0.693147 * deltaTime / safeHalfLife);

    vec3 fadedColor = texColor.rgb * decayMultiplier;

    // CRITICAL: Subtract a minimum amount to overcome 8-bit quantization
    // Without this, values like 48*0.99=47.52 round back to 48 and never fade
    // Subtract ~2/255 per frame to guarantee progress toward black
    fadedColor = max(fadedColor - vec3(0.008), vec3(0.0));

    finalColor = vec4(fadedColor, texColor.a);
}
