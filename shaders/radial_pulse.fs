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
uniform float petalAmp;
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

    // Spiral phase for animation
    float spiralPhase = phase + radius * spiralTwist;
    float angularWave = sin(angle * float(segments) + spiralPhase);

    // Petal modulation: angular wave scales radial displacement
    float petalMod = 1.0 + petalAmp * angularWave;

    // Radial displacement (concentric rings) with petal modulation
    float radialDisp = sin(radius * radialFreq + phase) * radialAmp * petalMod;

    // Tangential displacement (swirl)
    float tangentDisp = angularWave * angularAmp * radius;

    vec2 uv = fragTexCoord + radialDir * radialDisp + tangentDir * tangentDisp;

    finalColor = texture(texture0, uv);
}
