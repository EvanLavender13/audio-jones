#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float strength;      // Displacement per iteration
uniform int iterations;      // Cascade depth
uniform float flowAngle;     // Rotation between tangent (0) and gradient (PI/2)
uniform int edgeWeighted;    // Scale displacement by gradient magnitude

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

void main()
{
    vec2 texel = 1.0 / resolution;
    vec2 warpedUV = fragTexCoord;

    float s = sin(flowAngle);
    float c = cos(flowAngle);

    for (int i = 0; i < iterations; i++) {
        vec2 gradient = computeSobelGradient(warpedUV, texel);
        float gradMag = length(gradient);

        // Perpendicular to gradient = tangent to edges
        vec2 tangent = vec2(-gradient.y, gradient.x);

        // flowAngle rotates between tangent (0) and gradient (PI/2)
        vec2 flow = vec2(c * tangent.x + s * gradient.x,
                         c * tangent.y + s * gradient.y);

        // Normalize to get direction only
        flow = gradMag > 0.001 ? normalize(flow) : vec2(0.0);

        // Base displacement, optionally scaled by edge strength
        float displacement = strength;
        if (edgeWeighted != 0) {
            displacement *= gradMag;
        }

        warpedUV += flow * displacement;
    }

    finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
}
