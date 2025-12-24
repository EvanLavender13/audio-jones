#version 330

// Injection blend pass: mix waveform into feedback at low opacity
// Waveform becomes subtle seed rather than dominant foreground

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // Feedback texture (processed accumulation)
uniform sampler2D injectionTex;   // Waveform injection texture
uniform float injectionOpacity;   // Blend amount (0 = no waveform, 1 = full waveform)

void main()
{
    vec4 feedback = texture(texture0, fragTexCoord);
    vec4 injection = texture(injectionTex, fragTexCoord);

    // Linear blend: feedback * (1-opacity) + injection * opacity
    vec3 blended = mix(feedback.rgb, injection.rgb, injectionOpacity);

    finalColor = vec4(blended, 1.0);
}
