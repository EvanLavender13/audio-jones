#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int splatCount;        // 4-16, default 8
uniform float splatSizeMin;    // 0.01-0.1, default 0.02
uniform float splatSizeMax;    // 0.05-0.25, default 0.12
uniform float strokeFreq;      // 400-2000, default 800
uniform float strokeOpacity;   // 0-1, default 0.7
uniform float outlineStrength; // 0-0.5, default 0.15
uniform float edgeStrength;    // 0-8, default 4.0
uniform float edgeMaxDarken;   // 0-0.3, default 0.15
uniform float grainScale;      // 100-800, default 400
uniform float grainAmount;     // 0-0.2, default 0.1
uniform float exposure;        // 0.5-2.0, default 1.0

float seed;

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float rand() {
    seed += 1.0;
    return hash(vec2(seed, seed * 1.7));
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main() {
    vec2 uv = fragTexCoord;
    vec3 col = texture(texture0, uv).rgb;

    // Two-pass splat rendering
    for (int pass = 0; pass < 2; ++pass) {
        float maxr = mix(splatSizeMin, splatSizeMax, 1.0 - float(pass));
        ivec2 cell = ivec2(floor(uv / maxr));

        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                for (int k = 0; k < splatCount; ++k) {
                    // Seed RNG from cell, neighbor offset, splat index, pass
                    seed = float(cell.x + i) * 131.0 + float(k) * 17.0
                         + float(cell.y + j) * 97.0 + float(pass) * 53.0;

                    vec2 splatPos = (vec2(cell + ivec2(i, j)) + vec2(rand(), rand())) * maxr;
                    vec3 splatColor = texture(texture0, splatPos).rgb;
                    float r = mix(0.25, 0.99, pow(rand(), 4.0)) * maxr;
                    float d = length(uv - splatPos);

                    // Random rotation for stroke direction
                    float ang = rand() * 6.283185;
                    vec2 rotated = mat2(cos(ang), -sin(ang), sin(ang), cos(ang)) * (uv - splatPos);

                    // Internal stroke hatching
                    float stroke = cos(rotated.x * strokeFreq) * 0.5 + 0.5;

                    // Soft circle with feathered edge
                    float feather = maxr * 0.1;
                    float fill = clamp((r - d) / feather, 0.0, 1.0);

                    // Brightness-weighted opacity
                    float brightness = dot(splatColor, vec3(0.299, 0.587, 0.114));
                    float opacity = pow(brightness, 0.8) * strokeOpacity;

                    col = mix(col, splatColor, opacity * stroke * fill);

                    // Circle outline
                    float outline = clamp(1.0 - abs(r - d) / 0.002, 0.0, 1.0);
                    col = mix(col, splatColor * 0.5, outlineStrength * outline);
                }
            }
        }
    }

    // Edge darkening (resolution-dependent epsilon)
    vec2 eps = vec2(1.0 / resolution.x, 0.0);
    vec2 jittered = uv + (valueNoise(uv * 18.0) - 0.5) * 0.01;

    float c0 = dot(texture(texture0, jittered).rgb, vec3(0.299, 0.587, 0.114));
    float c1 = dot(texture(texture0, jittered + eps.xy).rgb, vec3(0.299, 0.587, 0.114));
    float c2 = dot(texture(texture0, jittered + eps.yx).rgb, vec3(0.299, 0.587, 0.114));

    float edge = max(abs(c1 - c0), abs(c2 - c0));
    col *= mix(0.1, 1.0, 1.0 - clamp(edge * edgeStrength, 0.0, edgeMaxDarken));

    // Paper grain
    float grain = mix(1.0 - grainAmount, 1.0, valueNoise(uv * grainScale));
    col = sqrt(col * grain) * exposure;

    finalColor = vec4(col, 1.0);
}
