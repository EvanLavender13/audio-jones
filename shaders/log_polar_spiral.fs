#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float speed;        // Animation speed (0.1-2.0)
uniform float zoomDepth;    // Zoom range in powers of 2 (1.0-5.0)
uniform vec2 focalOffset;   // Lissajous center offset (UV units)
uniform int layers;         // Layer count (2-8)
uniform float spiralTwist;  // Angle varies with log-radius (radians)
uniform float spiralTurns;  // Rotation per zoom cycle (turns)

const float TWO_PI = 6.28318530718;

vec2 toLogPolar(vec2 p) {
    return vec2(log(length(p) + 0.001), atan(p.y, p.x));
}

vec2 fromLogPolar(vec2 lp) {
    return exp(lp.x) * vec2(cos(lp.y), sin(lp.y));
}

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;
    float L = float(layers);

    // Transform relative to center with Lissajous offset
    vec2 center = vec2(0.5) + focalOffset;
    vec2 centered = fragTexCoord - center;

    for (int i = 0; i < layers; i++) {
        // Phase: where this layer sits in zoom cycle [0,1)
        float phase = fract((float(i) - time * speed) / L);

        // Alpha: cosine fade (zero at phase boundaries, peak at 0.5)
        float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5;

        // Early-out on negligible contribution
        if (alpha < 0.001) continue;

        // Convert to log-polar coordinates
        vec2 lp = toLogPolar(centered);

        // Scale shift (zoom) based on layer phase
        lp.x += phase * zoomDepth;

        // Spiral twist: angle varies with log-radius
        lp.y += lp.x * spiralTwist;

        // Per-layer rotation based on phase
        lp.y += phase * spiralTurns * TWO_PI;

        // Convert back to Cartesian
        vec2 warped = fromLogPolar(lp);
        vec2 uv = warped + center;

        // Bounds check: discard samples outside texture
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            continue;
        }

        // Edge softening: fade near boundaries
        float edgeDist = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
        float edgeFade = smoothstep(0.0, 0.1, edgeDist);

        // Sample and accumulate
        vec3 sampleColor = texture(texture0, uv).rgb;
        float weight = alpha * edgeFade;
        colorAccum += sampleColor * weight;
        weightAccum += weight;
    }

    // Normalize and output
    if (weightAccum > 0.0) {
        finalColor = vec4(colorAccum / weightAccum, 1.0);
    } else {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
