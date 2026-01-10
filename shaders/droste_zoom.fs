#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float scale;
uniform float spiralAngle;
uniform float twist;
uniform float innerRadius;
uniform int branches;

const float TWO_PI = 6.28318530718;

void main()
{
    vec2 uv = fragTexCoord - 0.5;
    float r = max(length(uv), 0.001);
    float theta = atan(uv.y, uv.x);

    float s = log(scale);
    float a = s / TWO_PI;

    // Log-polar coordinates
    float logR = log(r);

    // Apply branches
    float workTheta = theta * float(max(branches, 1));

    // Conformal rotation in log-polar space
    float zx = logR - a * workTheta;
    float zy = a * logR + workTheta + spiralAngle + twist * logR;

    // Droste tiling: mod in log-radius, animate
    // Use floor-based mod for proper positive result
    zx = zx - time;
    zx = zx - s * floor(zx / s);

    // Convert back - but keep output in same scale as input
    // exp(zx) is in [1, scale], divide by scale to get [1/scale, 1]
    // Then multiply by original max radius to stay in bounds
    float newR = exp(zx) / scale * 0.5;

    vec2 result = newR * vec2(cos(zy), sin(zy));

    // Undo branches
    if (branches > 1) {
        float outTheta = atan(result.y, result.x) / float(branches);
        float outR = length(result);
        result = outR * vec2(cos(outTheta), sin(outTheta));
    }

    vec2 sampleUV = result + 0.5;
    vec4 color = texture(texture0, sampleUV);

    // Inner radius mask
    float fade = (innerRadius > 0.0) ? smoothstep(0.0, innerRadius, length(uv)) : 1.0;

    finalColor = vec4(color.rgb * fade, 1.0);
}
