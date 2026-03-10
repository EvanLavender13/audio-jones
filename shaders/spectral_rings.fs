// Spectral Rings — dense concentric rings with FFT-reactive brightness
// Based on "Rings [324 chars]" by XorDev (shadertoy.com/view/sstyDX), CC BY-NC-SA 3.0
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D noiseTex;
uniform sampler2D gradientLUT;
uniform float time;
uniform float noiseScale;

void main() {
    vec2 r = resolution;
    vec2 I = fragTexCoord * r;
    vec2 i = vec2(1.0);
    vec2 d = vec2((I.x - 2.0 * I.y + r.y * 0.2) / 2000.0);
    vec2 p, l;
    float ns = noiseScale;

    vec4 O = vec4(0.0);

    for (; i.x < 16.0; i += 1.0 / i) {
        l = length(p = (I + d * i) * mat2(2, 1, -2, 4) / r.y - vec2(-0.1, 0.6)) / vec2(3, 8);
        d *= -mat2(73, 67, -67, 73) / 99.0;

        // Sample noise as gradient LUT index
        float nR = texture(noiseTex, l * ns).r;
        float nC = texture(noiseTex, p * mat2(cos(time * 0.1 + vec4(0, 33, 11, 0))) * ns).r;
        vec4 radial = texture(gradientLUT, vec2(nR, 0.5));
        vec4 cart = texture(gradientLUT, vec2(nC, 0.5));

        O += pow(radial * cart / l.x * 0.4, vec4(5, 8, 9, 0));
    }

    finalColor = vec4(pow(O, vec4(0.2)).rgb, 1.0);
}
