#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform int mode;           // 0=radial, 1=spiral
uniform int samples;        // Sample count (4-16)
uniform float streakLength; // Maximum streak distance in UV units (0.1-1.0)
uniform float spiralTwist;  // Additional angle per sample (radians)
uniform float spiralTurns;  // Base rotation offset (radians)
uniform vec2 focalOffset;   // Lissajous center offset (UV units)

void main()
{
    vec2 center = vec2(0.5) + focalOffset;
    vec2 toCenter = center - fragTexCoord;
    float dist = length(toCenter);

    // Base direction: normalized vector toward center
    vec2 dir = (dist > 0.001) ? normalize(toCenter) : vec2(0.0, 1.0);

    // Apply base rotation offset (spiralTurns)
    if (spiralTurns != 0.0) {
        float cosT = cos(spiralTurns);
        float sinT = sin(spiralTurns);
        dir = vec2(dir.x * cosT - dir.y * sinT, dir.x * sinT + dir.y * cosT);
    }

    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;
    float sampleCount = float(samples);

    for (int i = 0; i < samples; i++) {
        // t: normalized position along streak [0, 1]
        float t = float(i) / (sampleCount - 1.0);

        // Sample offset distance: scale by t and streakLength
        float offsetDist = t * streakLength * dist;

        // Sample direction: apply spiral twist per sample in spiral mode
        vec2 sampleDir = dir;
        if (mode == 1 && spiralTwist != 0.0) {
            float twistAngle = t * spiralTwist;
            float cosA = cos(twistAngle);
            float sinA = sin(twistAngle);
            sampleDir = vec2(dir.x * cosA - dir.y * sinA, dir.x * sinA + dir.y * cosA);
        }

        // Sample UV: move toward center along direction
        vec2 sampleUV = fragTexCoord + sampleDir * offsetDist;

        // Bounds check
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            continue;
        }

        // Edge fade: softens samples near texture boundaries
        float edgeDist = min(min(sampleUV.x, 1.0 - sampleUV.x), min(sampleUV.y, 1.0 - sampleUV.y));
        float edgeFade = smoothstep(0.0, 0.05, edgeDist);

        // Weight: inner samples (closer to pixel) contribute more
        // Gaussian-like falloff: higher weight at t=0, lower at t=1
        float weight = exp(-2.0 * t * t) * edgeFade;

        // Accumulate
        vec3 sampleColor = texture(texture0, sampleUV).rgb;
        colorAccum += sampleColor * weight;
        weightAccum += weight;
    }

    // Normalize and output
    if (weightAccum > 0.0) {
        finalColor = vec4(colorAccum / weightAccum, 1.0);
    } else {
        finalColor = texture(texture0, fragTexCoord);
    }
}
