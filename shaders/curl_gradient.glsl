#version 430

// Precompute density gradient for curl flow.
// Samples density at +/- gradientRadius in X/Y, computes central differences.

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D trailMap;
layout(binding = 1) uniform sampler2D accumMap;
layout(rgba16f, binding = 2) writeonly uniform image2D gradientOut;

uniform vec2 resolution;
uniform float gradientRadius;
uniform float accumSenseBlend;

const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

float sampleDensity(vec2 uv)
{
    vec4 trail = texture(trailMap, uv);
    vec4 accum = texture(accumMap, uv);
    vec4 blended = mix(trail, accum, accumSenseBlend);
    return dot(blended.rgb, LUMA_WEIGHTS);
}

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    if (coord.x >= int(resolution.x) || coord.y >= int(resolution.y)) {
        return;
    }

    vec2 texelSize = 1.0 / resolution;
    vec2 uv = (vec2(coord) + 0.5) * texelSize;

    // Sample density at +/- gradientRadius in X and Y
    vec2 offsetX = vec2(gradientRadius * texelSize.x, 0.0);
    vec2 offsetY = vec2(0.0, gradientRadius * texelSize.y);

    float densityPosX = sampleDensity(uv + offsetX);
    float densityNegX = sampleDensity(uv - offsetX);
    float densityPosY = sampleDensity(uv + offsetY);
    float densityNegY = sampleDensity(uv - offsetY);

    // Central differences for gradient
    float gradX = (densityPosX - densityNegX) * 0.5;
    float gradY = (densityPosY - densityNegY) * 0.5;

    // Sample center density for ramp calculation in agent shader
    float density = sampleDensity(uv);

    imageStore(gradientOut, coord, vec4(gradX, gradY, density, 0.0));
}
