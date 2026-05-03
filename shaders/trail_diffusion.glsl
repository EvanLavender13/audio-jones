#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 1) readonly uniform image2D inputMap;
layout(rgba32f, binding = 2) writeonly uniform image2D outputMap;

uniform vec2 resolution;
uniform int diffusionScale;
uniform float decayFactor;  // Pre-computed: exp(-0.693147 * dt / halfLife)
uniform int applyDecay;     // 1 for V-pass, 0 for H-pass
uniform int direction;      // 0 = horizontal, 1 = vertical

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    if (coord.x >= int(resolution.x) || coord.y >= int(resolution.y)) {
        return;
    }

    vec4 result;
    if (diffusionScale == 0) {
        result = imageLoad(inputMap, coord);
    } else {
        // Dense unit-stride Gaussian: sigma = diffusionScale, radius = ceil(3 sigma)
        ivec2 offset = (direction == 0) ? ivec2(1, 0) : ivec2(0, 1);
        ivec2 res = ivec2(resolution);

        float sigma = float(diffusionScale);
        float twoSigma2 = 2.0 * sigma * sigma;
        int radius = int(ceil(3.0 * sigma));

        vec4 sum = vec4(0.0);
        float wsum = 0.0;
        for (int i = -radius; i <= radius; i++) {
            float w = exp(-float(i * i) / twoSigma2);
            ivec2 s = ivec2(mod(vec2(coord + i * offset), vec2(res)));
            sum += imageLoad(inputMap, s) * w;
            wsum += w;
        }
        result = sum / wsum;
    }

    if (applyDecay == 1) {
        result *= decayFactor;
    }

    imageStore(outputMap, coord, result);
}
