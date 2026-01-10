#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float scale;            // Ratio between recursive copies (1.5 to 10.0)
uniform float spiralAngle;      // Additional rotation per cycle (radians)
uniform float twist;            // Radius-dependent twist (-1.0 to 1.0)
uniform float innerRadius;      // Mask center singularity (0.0 to 0.5)
uniform int branches;           // Number of spiral arms (1 to 8)

const float TWO_PI = 6.28318530718;

void main()
{
    vec2 uv = fragTexCoord - 0.5;

    // Polar coordinates
    float r = length(uv);
    float theta = atan(uv.y, uv.x);

    // Clamp minimum radius to avoid log(0) singularity
    r = max(r, 0.001);

    // Log of scale ratio for conformal spiral mapping
    float s = log(scale);
    float a = s / TWO_PI;

    // Multi-arm spiral: multiply angle by branch count
    float branchTheta = theta;
    if (branches > 1) {
        branchTheta = theta * float(branches);
    }

    // Conformal spiral mapping: rotate log-polar space
    // mat2(1, -a, a, 1) rotates log-polar rectangle so diagonal aligns with vertical
    vec2 z;
    z.x = log(r) + a * branchTheta;
    z.y = branchTheta - a * log(r);

    // Additional spiral rotation
    z.y += spiralAngle;

    // Radius-dependent twist beyond natural alpha
    z.y += twist * z.x;

    // Droste: mod log-radius, animate
    z.x = mod(z.x - time, s);

    // Convert back to Cartesian
    float expX = exp(z.x);
    vec2 result = expX * vec2(cos(z.y), sin(z.y));

    // Undo branch multiplication in output
    if (branches > 1) {
        float outTheta = atan(result.y, result.x) / float(branches);
        float outR = length(result);
        result = outR * vec2(cos(outTheta), sin(outTheta));
    }

    // Final UV
    vec2 sampleUV = result + 0.5;

    // Sample texture
    vec4 color = texture(texture0, sampleUV);

    // Inner radius fade: smoothly mask center singularity
    float originalR = length(fragTexCoord - 0.5);
    if (innerRadius > 0.0) {
        float fade = smoothstep(0.0, innerRadius, originalR);
        color.rgb *= fade;
    }

    finalColor = color;
}
