// Based on "notebook drawings" by flockaroo (Florian Berger)
// https://www.shadertoy.com/view/XtVGD1
// License: CC BY-NC-SA 3.0 Unported
// Modified: Adapted for AudioJones pipeline with configurable parameters

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // Input scene
uniform sampler2D texture1;   // Shared noise texture (1024x1024 RGBA)
uniform vec2 resolution;
uniform int angleCount;
uniform int sampleCount;
uniform float gradientEps;
uniform float desaturation;
uniform float toneCap;
uniform float noiseInfluence;
uniform float colorStrength;
uniform float paperStrength;
uniform float vignetteStrength;
uniform float wobbleTime;
uniform float wobbleAmount;

#define PI2 6.28318530718

// Sample noise texture with resolution-scaled UV
vec4 getRand(vec2 pos) {
    vec2 noiseRes = vec2(textureSize(texture1, 0));
    return textureLod(texture1, pos / noiseRes / resolution.y * 1080.0, 0.0);
}

// Sample input with desaturation and tonal clamping
vec4 getCol(vec2 pos) {
    vec2 uv = pos / resolution;
    vec4 c1 = texture(texture0, uv);
    // Edge fade to white (prevents wobble from sampling outside bounds)
    vec4 e = smoothstep(vec4(-0.05), vec4(0.0), vec4(uv, vec2(1.0) - uv));
    c1 = mix(vec4(1.0, 1.0, 1.0, 0.0), c1, e.x * e.y * e.z * e.w);
    // Desaturate: extract green-dominant brightness, mix toward gray
    float d = clamp(dot(c1.xyz, vec3(-0.5, 1.0, -0.5)), 0.0, 1.0);
    vec4 c2 = vec4(0.7);
    return min(mix(c1, c2, desaturation * d), toneCap);
}

// Halftone threshold: smoothstep around noise-dithered brightness
vec4 getColHT(vec2 pos) {
    return smoothstep(0.95, 1.05, getCol(pos) * 0.8 + 0.2 + getRand(pos * 0.7));
}

// Luminance from desaturated color
float getVal(vec2 pos) {
    vec4 c = getCol(pos);
    return dot(c.xyz, vec3(0.333));
}

// Central-difference gradient
vec2 getGrad(vec2 pos, float eps) {
    vec2 d = vec2(eps, 0.0);
    return vec2(
        getVal(pos + d.xy) - getVal(pos - d.xy),
        getVal(pos + d.yx) - getVal(pos - d.yx)
    ) / eps / 2.0;
}

void main() {
    float scale = resolution.y / 400.0;
    vec2 pos = fragTexCoord * resolution
             + wobbleAmount * sin(wobbleTime * vec2(1.0, 1.7)) * scale;

    float col = 0.0;        // Darkness accumulator
    vec3 col2 = vec3(0.0);  // Tinted color accumulator
    float sum = 0.0;

    for (int i = 0; i < angleCount; i++) {
        float ang = PI2 / float(angleCount) * (float(i) + 0.8);
        vec2 v = vec2(cos(ang), sin(ang));

        for (int j = 0; j < sampleCount; j++) {
            // Perpendicular offset (stroke width)
            vec2 dpos = v.yx * vec2(1.0, -1.0) * float(j) * scale;
            // Parallel offset (curved stroke path, j^2 taper)
            vec2 dpos2 = v.xy * float(j * j) / float(sampleCount) * 0.5 * scale;

            for (float s = -1.0; s <= 1.0; s += 2.0) {
                vec2 pos2 = pos + s * dpos + dpos2;
                // Color sample at perpendicular offset, 2x distance
                vec2 pos3 = pos + (s * dpos + dpos2).yx * vec2(1.0, -1.0) * 2.0;
                vec2 g = getGrad(pos2, gradientEps);

                // Stroke: gradient aligned with sample direction
                float fact = dot(g, v)
                           - 0.5 * abs(dot(g, v.yx * vec2(1.0, -1.0)));
                // Color direction factor
                float fact2 = dot(normalize(g + vec2(0.0001)),
                                  v.yx * vec2(1.0, -1.0));

                fact = clamp(fact, 0.0, 0.05);
                fact2 = abs(fact2);

                // Fixed linear falloff
                fact *= 1.0 - float(j) / float(sampleCount);

                col += fact;
                col2 += fact2 * getColHT(pos3).xyz;
                sum += fact2;
            }
        }
    }

    // Normalize
    col /= float(sampleCount * angleCount) * 0.75 / sqrt(resolution.y);
    col2 /= sum;

    // Noise-modulated stroke intensity
    float noiseVal = getRand(pos * 0.7).x;
    col *= mix(1.0, 0.6 + 0.8 * noiseVal, noiseInfluence);

    // Cubic contrast curve
    col = 1.0 - col;
    col *= col * col;

    // Paper grid texture
    vec2 sp = sin(pos.xy * 0.1 / sqrt(scale));
    vec3 karo = vec3(1.0);
    karo -= paperStrength * 0.5 * vec3(0.25, 0.1, 0.1)
          * dot(exp(-sp * sp * 80.0), vec2(1.0));

    // Vignette
    float r = length(pos - resolution.xy * 0.5) / resolution.x;
    float vign = 1.0 - r * r * r * vignetteStrength;

    // Final: blend monochrome vs tinted strokes
    vec3 result = vec3(col) * mix(vec3(1.0), col2, colorStrength) * karo * vign;

    finalColor = vec4(result, 1.0);
}
