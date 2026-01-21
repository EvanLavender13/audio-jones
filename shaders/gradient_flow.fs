#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float strength;      // Displacement per iteration
uniform int iterations;      // Cascade depth
uniform float edgeWeight;    // Blend between uniform (0) and edge-scaled (1) displacement
uniform int randomDirection; // Randomize tangent direction per pixel

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 computeSobelGradient(vec2 uv, vec2 texel) {
    float tl = luminance(texture(texture0, uv + vec2(-texel.x, -texel.y)).rgb);
    float t  = luminance(texture(texture0, uv + vec2(    0.0, -texel.y)).rgb);
    float tr = luminance(texture(texture0, uv + vec2( texel.x, -texel.y)).rgb);
    float l  = luminance(texture(texture0, uv + vec2(-texel.x,     0.0)).rgb);
    float r  = luminance(texture(texture0, uv + vec2( texel.x,     0.0)).rgb);
    float bl = luminance(texture(texture0, uv + vec2(-texel.x,  texel.y)).rgb);
    float b  = luminance(texture(texture0, uv + vec2(    0.0,  texel.y)).rgb);
    float br = luminance(texture(texture0, uv + vec2( texel.x,  texel.y)).rgb);

    float gx = (tr + 2.0*r + br) - (tl + 2.0*l + bl);
    float gy = (tl + 2.0*t + tr) - (bl + 2.0*b + br);

    return vec2(gx, gy);
}

// Average gradients over 3x3 area for smoother flow
vec2 computeBlurredGradient(vec2 uv, vec2 texel) {
    vec2 sum = vec2(0.0);
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            sum += computeSobelGradient(uv + vec2(float(x), float(y)) * texel, texel);
        }
    }
    return sum / 9.0;
}

void main()
{
    vec2 texel = 1.0 / resolution;
    vec2 warpedUV = fragTexCoord;

    for (int i = 0; i < iterations; i++) {
        vec2 gradient = computeBlurredGradient(warpedUV, texel);
        float gradMag = length(gradient);

        // Tangent = perpendicular to gradient (flows along edges)
        float dirSign = 1.0;
        if (randomDirection != 0) {
            dirSign = hash(warpedUV * resolution) < 0.5 ? -1.0 : 1.0;
        }
        vec2 tangent = vec2(-gradient.y, gradient.x) * dirSign;

        // Normalize to get direction only
        vec2 flow = gradMag > 0.001 ? normalize(tangent) : vec2(0.0);

        // Blend between uniform and edge-weighted displacement
        float displacement = strength * mix(1.0, gradMag, edgeWeight);

        warpedUV += flow * displacement;
    }

    finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
}
