#version 330

// Tunnel Effect with multi-depth accumulation
// Polar coordinate remapping creates infinite tunnel with texture undulation

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform float speed;
uniform float rotation;      // CPU-accumulated rotation angle (radians)
uniform float twist;
uniform int layers;
uniform float depthSpacing;
uniform float windingAmplitude;
uniform float windingFreqX;
uniform float windingFreqY;
uniform vec2 focalOffset;
uniform float windingPhaseX;
uniform float windingPhaseY;

const float TAU = 6.28318530718;

out vec4 finalColor;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    for (int i = 0; i < layers; i++) {
        float depthOffset = float(i) * depthSpacing;

        // Compute depth value for this layer (base depth from screen distance)
        vec2 baseCentered = fragTexCoord - 0.5 - focalOffset;
        float baseDist = length(baseCentered);
        float depth = 1.0 / (baseDist + 0.1) + time * speed + depthOffset;

        // Winding: offset the tunnel center as a function of depth
        vec2 windingOffset = windingAmplitude * vec2(
            sin(depth * windingFreqX + windingPhaseX),
            cos(depth * windingFreqY + windingPhaseY)
        );

        // Recompute polar coords from depth-offset center
        vec2 centered = fragTexCoord - 0.5 - focalOffset - windingOffset;
        float dist = length(centered);
        float angle = atan(centered.y, centered.x);

        // Texture coordinates
        float texY = 1.0 / (dist + 0.1) + time * speed + depthOffset;
        float texX = angle / TAU + rotation;

        // Depth-dependent spiral twist
        texX += texY * twist;

        vec2 tunnelUV = vec2(texX, texY);

        // Depth-based weight (further layers contribute less)
        float weight = 1.0 / float(i + 1);
        colorAccum += texture(texture0, fract(tunnelUV)).rgb * weight;
        weightAccum += weight;
    }

    finalColor = vec4(colorAccum / weightAccum, 1.0);
}
