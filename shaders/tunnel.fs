#version 330

// Tunnel Effect with multi-depth accumulation
// Polar coordinate remapping creates infinite tunnel illusion with Lissajous winding

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform float speed;
uniform float rotationSpeed;
uniform float twist;
uniform int layers;
uniform float depthSpacing;
uniform float windingAmplitude;
uniform float windingFreqX;
uniform float windingFreqY;
uniform vec2 focalOffset;

const float TAU = 6.28318530718;

out vec4 finalColor;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    // UV to polar (centered at focalOffset)
    vec2 centered = fragTexCoord - 0.5 - focalOffset;
    float dist = length(centered);
    float angle = atan(centered.y, centered.x);

    for (int i = 0; i < layers; i++) {
        float depthOffset = float(i) * depthSpacing;

        // 1/dist creates depth illusion (closer to center = further away)
        float texY = 1.0 / (dist + 0.1) + time * speed + depthOffset;
        float texX = angle / TAU + time * rotationSpeed;

        // Depth-dependent spiral twist
        texX += texY * twist;

        // Lissajous winding offset
        texX += windingAmplitude * sin(texY * windingFreqX + time);
        texY += windingAmplitude * cos(texY * windingFreqY + time);

        vec2 tunnelUV = vec2(texX, texY);

        // Depth-based weight (further layers contribute less)
        float weight = 1.0 / float(i + 1);
        colorAccum += texture(texture0, fract(tunnelUV)).rgb * weight;
        weightAccum += weight;
    }

    finalColor = vec4(colorAccum / weightAccum, 1.0);
}
