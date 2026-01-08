#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float powerMapN;      // Power exponent
uniform float rotation;       // CPU-accumulated rotation (radians)
uniform vec2 focalOffset;     // Lissajous center offset

// 2D rotation helper
vec2 rotate2d(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

void main()
{
    vec2 uv = fragTexCoord - 0.5 - focalOffset;

    // Apply rotation
    uv = rotate2d(uv, rotation);

    // Complex power: z^n in polar form
    // r^n, angle*n
    float r = length(uv);
    float a = atan(uv.y, uv.x);

    float newR = pow(r + 0.001, powerMapN);
    float newAngle = a * powerMapN;

    vec2 newUV = vec2(cos(newAngle), sin(newAngle)) * newR * 0.5 + 0.5;

    finalColor = texture(texture0, clamp(newUV, 0.0, 1.0));
}
