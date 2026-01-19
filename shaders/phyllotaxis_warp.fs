#version 330

// Phyllotaxis Warp: UV displacement along sunflower spiral arms
// Analytical arm distance creates swirling galaxy-arm or water-spiral distortion

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float scale;
uniform float divergenceAngle;
uniform float warpStrength;
uniform float warpFalloff;
uniform float tangentIntensity;
uniform float radialIntensity;
uniform float spinOffset;

out vec4 finalColor;

float getArmDistance(vec2 uv, float scl, float divAngle, float spin)
{
    vec2 centered = uv - 0.5;
    float r = length(centered);
    float theta = atan(centered.y, centered.x) - spin;

    if (r < 0.001) { return 0.0; }

    float estimatedN = (r / scl) * (r / scl);
    float expectedTheta = estimatedN * divAngle;
    float armPhase = (theta - expectedTheta) / divAngle;
    float distToArm = (armPhase - round(armPhase)) * divAngle;

    return distToArm;
}

vec2 phyllotaxisWarp(vec2 uv, float scl, float divAngle, float spin,
                     float tangentStr, float radialStr, float falloff)
{
    vec2 centered = uv - 0.5;
    float r = length(centered);
    if (r < 0.001) { return uv; }

    float distToArm = getArmDistance(uv, scl, divAngle, spin);
    float displacement = distToArm * exp(-abs(distToArm) * falloff);

    vec2 tangent = vec2(-centered.y, centered.x) / r;
    vec2 radial = centered / r;

    vec2 offset = tangent * displacement * tangentStr * r
                + radial * displacement * radialStr;

    return uv + offset;
}

void main()
{
    vec2 warped = phyllotaxisWarp(fragTexCoord, scale, divergenceAngle, spinOffset,
                                   tangentIntensity * warpStrength,
                                   radialIntensity * warpStrength,
                                   warpFalloff);

    finalColor = texture(texture0, warped);
}
