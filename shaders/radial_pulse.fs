#version 330

// Radial Pulse: polar-native sine distortion
// Creates concentric rings, petal symmetry, and animated spirals

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float radialFreq;
uniform float radialAmp;
uniform int segments;
uniform float angularAmp;
uniform float phase;
uniform float spiralTwist;

out vec4 finalColor;

void main()
{
    vec2 delta = fragTexCoord - vec2(0.5);
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);

    // Direction vectors
    vec2 radialDir = (radius > 0.0001) ? normalize(delta) : vec2(1.0, 0.0);
    vec2 tangentDir = vec2(-radialDir.y, radialDir.x);

    // Radial displacement (concentric rings)
    float radialDisp = sin(radius * radialFreq + phase) * radialAmp;

    // Angular displacement (petals) with spiral twist
    float spiralPhase = phase + radius * spiralTwist;
    float tangentDisp = sin(angle * float(segments) + spiralPhase) * angularAmp * radius;

    vec2 uv = fragTexCoord + radialDir * radialDisp + tangentDir * tangentDisp;

    finalColor = texture(texture0, uv);
}
