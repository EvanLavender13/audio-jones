#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int splatCount;        // 4-16, default 11
uniform float splatSizeMin;    // 0.01-0.1, default 0.018
uniform float splatSizeMax;    // 0.05-0.25, default 0.1
uniform float strokeFreq;      // 400-2000, default 1200
uniform float strokeOpacity;   // 0-1, default 0.7
uniform float outlineStrength; // 0-1, default 1.0
uniform float edgeStrength;    // 0-8, default 4.0
uniform float edgeMaxDarken;   // 0-0.3, default 0.13
uniform float grainScale;      // 100-800, default 400
uniform float grainAmount;     // 0-0.2, default 0.1
uniform float exposure;        // 0.5-2.0, default 1.28

float seed;

float rand() {
    return fract(sin(seed++) * 43758.545);
}

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
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

// Sample input texture from aspect-corrected centered coordinates
vec3 sampleInput(vec2 p) {
    float aspect = resolution.x / resolution.y;
    vec2 uv = p / vec2(aspect, 1.0) + 0.5;
    return texture(texture0, clamp(uv, 0.0, 1.0)).rgb * 1.05;
}

void main() {
    float aspect = resolution.x / resolution.y;

    // Centered, aspect-corrected coordinates (matches original space)
    vec2 p = (fragTexCoord - 0.5) * vec2(aspect, 1.0);

    // Start from dark canvas — splats paint the image on top
    vec3 col = vec3(0.133, 0.133, 0.167);

    // Two-pass splat rendering: large splats first, then small
    for (int pass = 0; pass < 2; ++pass) {
        float maxr = mix(splatSizeMin, splatSizeMax, 1.0 - float(pass));

        ivec2 cell = ivec2(floor(p / maxr));

        for (int i = -1; i <= 1; ++i) {
            for (int j0 = -1; j0 <= 1; ++j0) {
                // Staggered grid: alternate rows flip neighbor order
                int j = ((cell.x + i) & 1) == 1 ? -j0 : j0;
                vec2 u = vec2(cell) + vec2(float(i), float(j));

                // Seed RNG from cell position and pass
                seed = u.x * 881.0 + u.y * 927.0 + float(pass) * 1801.0;

                for (int k = 0; k < splatCount; ++k) {
                    vec2 splatPos = (u + vec2(rand(), rand())) * maxr;
                    vec2 delta = p - splatPos;
                    vec3 splatColor = sampleInput(splatPos);
                    float brightness = dot(splatColor, vec3(1.0 / 3.0));
                    float r = mix(0.25, 0.99, pow(rand(), 4.0)) * maxr;
                    float ang = rand() * 6.283185;
                    float d = length(delta);
                    vec2 rotated = mat2(cos(ang), sin(ang), -sin(ang), cos(ang)) * delta;

                    // Occasional desaturation toward bright grey
                    splatColor = mix(splatColor, vec3(brightness) * 1.5, pow(rand(), 16.0));

                    // Per-splat random opacity (0.3-1.2 range) and feather width
                    float opacity = mix(0.1, 0.4, rand()) * 3.0 * strokeOpacity;
                    float feather = mix(0.001, 0.004, rand());

                    // Stroke hatching modulates fill
                    float stroke = mix(0.8, 1.0, cos(rotated.x * strokeFreq) * 0.5 + 0.5);
                    float fill = clamp((r - d) / feather, 0.0, 1.0);
                    col = mix(col, splatColor, opacity * pow(brightness, 0.8) * stroke * fill);

                    // Circle outline with per-splat random strength
                    float outlineAlpha = mix(0.14, 0.3, pow(rand(), 16.0)) / 4.0;
                    float outline = clamp(1.0 - abs(r - d) / 0.002, 0.0, 1.0);
                    col = mix(col, splatColor * 0.5, outlineStrength * outlineAlpha * outline);
                }
            }
        }
    }

    // Edge darkening with jitter for hand-drawn feel
    vec2 eps = vec2(1.0 / resolution.x, 1.0 / resolution.y);
    vec2 jittered = fragTexCoord + (valueNoise(fragTexCoord * 18.0) - 0.5) * 0.01;

    float c0 = dot(texture(texture0, jittered).rgb, vec3(1.0 / 3.0));
    float c1 = dot(texture(texture0, jittered + vec2(eps.x, 0.0)).rgb, vec3(1.0 / 3.0));
    float c2 = dot(texture(texture0, jittered + vec2(0.0, eps.y)).rgb, vec3(1.0 / 3.0));

    float edge = max(abs(c1 - c0), abs(c2 - c0));
    col *= mix(0.1, 1.0, 1.0 - clamp(edge * edgeStrength, 0.0, edgeMaxDarken));

    // Contrast boost + paper grain + gamma
    col = (col - 0.5) * 1.1 + 0.5;
    float grain = mix(1.0 - grainAmount, 1.0, valueNoise(fragTexCoord * grainScale));
    col = sqrt(max(col * grain, 0.0)) * exposure;

    finalColor = vec4(col, 1.0);
}
