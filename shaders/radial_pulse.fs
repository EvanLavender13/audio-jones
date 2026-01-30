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
uniform int octaves;
uniform bool depthBlend;

out vec4 finalColor;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 delta = fragTexCoord - vec2(0.5);
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);

    vec2 radialDir = (radius > 0.0001) ? normalize(delta) : vec2(1.0, 0.0);
    vec2 tangentDir = vec2(-radialDir.y, radialDir.x);

    vec2 totalDisp = vec2(0.0);

    for (int i = 0; i < octaves; i++) {
        float freq = radialFreq * pow(2.0, float(i));
        float amp = 1.0 / pow(2.0, float(i));

        float spiralPhase = phase + (radius * 2.0) * spiralTwist;
        float angularWave = sin(angle * float(segments) + spiralPhase);
        float petalMod = 1.0 + petalAmp * angularWave;

        float radialDisp = sin(radius * freq + phase) * radialAmp * amp * petalMod;
        float tangentDisp = angularWave * angularAmp * amp * radius;

        totalDisp += radialDir * radialDisp + tangentDir * tangentDisp;

        if (depthBlend) {
            vec2 uv = fragTexCoord + totalDisp;
            float weight = amp;
            colorAccum += texture(texture0, uv).rgb * weight;
            weightAccum += weight;
        }
    }

    if (depthBlend) {
        finalColor = vec4(colorAccum / weightAccum, 1.0);
    } else {
        vec2 uv = fragTexCoord + totalDisp;
        finalColor = texture(texture0, uv);
    }
}
