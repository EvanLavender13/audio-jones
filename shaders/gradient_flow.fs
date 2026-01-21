#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float strength;      // Displacement per iteration
uniform int iterations;      // Cascade depth
uniform float flowAngle;     // Rotation between tangent (0) and gradient (PI/2)
uniform float edgeWeight;    // Blend between uniform (0) and edge-scaled (1) displacement
uniform int smoothRadius;    // Structure tensor window half-size (2 = 5x5 window)

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

vec2 computeSobelGradient(vec2 uv, vec2 texel) {
    // Sample 3x3 neighborhood (luminance)
    float tl = luminance(texture(texture0, uv + vec2(-texel.x, -texel.y)).rgb);
    float t  = luminance(texture(texture0, uv + vec2(    0.0, -texel.y)).rgb);
    float tr = luminance(texture(texture0, uv + vec2( texel.x, -texel.y)).rgb);
    float l  = luminance(texture(texture0, uv + vec2(-texel.x,     0.0)).rgb);
    float r  = luminance(texture(texture0, uv + vec2( texel.x,     0.0)).rgb);
    float bl = luminance(texture(texture0, uv + vec2(-texel.x,  texel.y)).rgb);
    float b  = luminance(texture(texture0, uv + vec2(    0.0,  texel.y)).rgb);
    float br = luminance(texture(texture0, uv + vec2( texel.x,  texel.y)).rgb);

    // Sobel gradients
    float gx = (tr + 2.0*r + br) - (tl + 2.0*l + bl);
    float gy = (tl + 2.0*t + tr) - (bl + 2.0*b + br);

    return vec2(gx, gy);
}

vec2 computeFlowDirection(vec2 uv, vec2 texel) {
    float J11 = 0.0, J22 = 0.0, J12 = 0.0;

    for (int y = -smoothRadius; y <= smoothRadius; y++) {
        for (int x = -smoothRadius; x <= smoothRadius; x++) {
            vec2 sampleUV = uv + vec2(float(x), float(y)) * texel;

            float lum_l = luminance(texture(texture0, sampleUV + vec2(-texel.x, 0.0)).rgb);
            float lum_r = luminance(texture(texture0, sampleUV + vec2( texel.x, 0.0)).rgb);
            float lum_t = luminance(texture(texture0, sampleUV + vec2(0.0, -texel.y)).rgb);
            float lum_b = luminance(texture(texture0, sampleUV + vec2(0.0,  texel.y)).rgb);

            float Ix = (lum_r - lum_l) * 0.5;
            float Iy = (lum_b - lum_t) * 0.5;

            J11 += Ix * Ix;
            J22 += Iy * Iy;
            J12 += Ix * Iy;
        }
    }

    float angle = 0.5 * atan(2.0 * J12, J22 - J11);
    vec2 tangent = vec2(cos(angle), sin(angle));

    float s = sin(flowAngle);
    float c = cos(flowAngle);
    return vec2(c * tangent.x - s * tangent.y, s * tangent.x + c * tangent.y);
}

void main()
{
    vec2 texel = 1.0 / resolution;
    vec2 warpedUV = fragTexCoord;

    for (int i = 0; i < iterations; i++) {
        vec2 flow = computeFlowDirection(warpedUV, texel);

        // Compute edge magnitude for weighting
        vec2 gradient = computeSobelGradient(warpedUV, texel);
        float gradMag = length(gradient);

        // Blend between uniform and edge-weighted displacement
        float displacement = strength * mix(1.0, gradMag, edgeWeight);

        warpedUV += flow * displacement;
    }

    finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
}
