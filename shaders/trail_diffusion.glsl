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
        // 5-tap Gaussian with wrapping (mod() handles negative coords correctly)
        ivec2 offset = (direction == 0) ? ivec2(1, 0) : ivec2(0, 1);
        vec2 res = resolution;

        ivec2 s0 = ivec2(mod(vec2(coord - 2 * offset * diffusionScale), res));
        ivec2 s1 = ivec2(mod(vec2(coord - 1 * offset * diffusionScale), res));
        ivec2 s3 = ivec2(mod(vec2(coord + 1 * offset * diffusionScale), res));
        ivec2 s4 = ivec2(mod(vec2(coord + 2 * offset * diffusionScale), res));

        result = imageLoad(inputMap, s0) * 0.0625
               + imageLoad(inputMap, s1) * 0.25
               + imageLoad(inputMap, coord) * 0.375
               + imageLoad(inputMap, s3) * 0.25
               + imageLoad(inputMap, s4) * 0.0625;
    }

    if (applyDecay == 1) {
        result *= decayFactor;
    }

    imageStore(outputMap, coord, result);
}
