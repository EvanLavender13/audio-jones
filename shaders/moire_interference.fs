#version 330

// Moiré Interference: Multi-sample UV transform with rotated/scaled copies
// Small rotation/scale differences produce large-scale wave interference patterns

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float rotationAngle;   // Angle between layers (radians)
uniform float scaleDiff;       // Scale ratio between layers
uniform int layers;            // Number of overlaid samples (2-4)
uniform int blendMode;         // 0=multiply, 1=min, 2=average, 3=difference
uniform float centerX;         // Rotation/scale center X
uniform float centerY;         // Rotation/scale center Y
uniform float rotationAccum;   // CPU-accumulated rotation offset

out vec4 finalColor;

mat2 rotate2d(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

void main()
{
    vec2 center = vec2(centerX, centerY);
    vec2 centered = fragTexCoord - center;

    vec4 result = texture(texture0, fragTexCoord);

    for (int i = 1; i < layers; i++) {
        float layerAngle = (rotationAngle + rotationAccum) * float(i);
        float layerScale = 1.0 + (scaleDiff - 1.0) * float(i);

        vec2 rotated = rotate2d(layerAngle) * centered;
        vec2 scaled = rotated * layerScale + center;

        // Mirror repeat for edge handling
        scaled = 1.0 - abs(mod(scaled, 2.0) - 1.0);

        vec4 samp = texture(texture0, scaled);

        // Blend based on mode
        if (blendMode == 0) {
            // Multiply
            result *= samp;
        } else if (blendMode == 1) {
            // Min
            result = min(result, samp);
        } else if (blendMode == 2) {
            // Average
            result = (result + samp) / 2.0;
        } else {
            // Difference
            result = abs(result - samp);
        }
    }

    // Normalize multiply mode to prevent excessive darkening
    if (blendMode == 0) {
        result = pow(result, vec4(1.0 / float(layers)));
    }

    finalColor = result;
}
