#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float strength;
uniform float granulation;
uniform float bleedStrength;

float getLuminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void sobelEdge(vec2 uv, vec2 texel, out float edge, out vec2 gradient)
{
    float n[9];
    n[0] = getLuminance(texture(texture0, uv + vec2(-texel.x, -texel.y)).rgb);
    n[1] = getLuminance(texture(texture0, uv + vec2(    0.0, -texel.y)).rgb);
    n[2] = getLuminance(texture(texture0, uv + vec2( texel.x, -texel.y)).rgb);
    n[3] = getLuminance(texture(texture0, uv + vec2(-texel.x,     0.0)).rgb);
    n[4] = getLuminance(texture(texture0, uv).rgb);
    n[5] = getLuminance(texture(texture0, uv + vec2( texel.x,     0.0)).rgb);
    n[6] = getLuminance(texture(texture0, uv + vec2(-texel.x,  texel.y)).rgb);
    n[7] = getLuminance(texture(texture0, uv + vec2(    0.0,  texel.y)).rgb);
    n[8] = getLuminance(texture(texture0, uv + vec2( texel.x,  texel.y)).rgb);

    float sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    float sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    edge = sqrt(sobelH * sobelH + sobelV * sobelV);
    gradient = vec2(sobelH, sobelV);
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbmNoise(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;
    vec3 color = texture(texture0, uv).rgb;

    float edge;
    vec2 gradient;
    sobelEdge(uv, texel, edge, gradient);

    color *= 1.0 - edge * strength;

    float paper = fbmNoise(uv * 20.0);
    color *= mix(1.0, paper, granulation * (1.0 - edge));

    vec2 bleedDir = normalize(gradient + 0.001);
    vec3 bleed = vec3(0.0);
    for (int i = -2; i <= 2; i++) {
        bleed += texture(texture0, uv + bleedDir * float(i) * 5.0 * texel).rgb;
    }
    bleed /= 5.0;
    color = mix(color, bleed, edge * bleedStrength);

    finalColor = vec4(color, texture(texture0, uv).a);
}
