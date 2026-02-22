// Scrawl: IFS fractal fold with thick marker strokes and gradient LUT coloring.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform int iterations;
uniform float foldOffset;
uniform float tileScale;
uniform float zoom;
uniform float warpFreq;
uniform float warpAmp;
uniform float thickness;
uniform float glowIntensity;
uniform float scrollAccum;
uniform float evolveAccum;
uniform float rotationAccum;
uniform sampler2D gradientLUT;

void main() {
    // UV: centered, aspect-corrected (matches reference layout)
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= resolution.x / resolution.y;

    // Smooth rotation (replaces reference's discrete 45-degree snaps)
    float cr = cos(rotationAccum), sr = sin(rotationAccum);
    uv *= mat2(cr, -sr, sr, cr);

    // Save pre-fold position for drawing mask
    vec2 p2 = uv;

    // Breathing zoom (reference: p *= 0.3 + asin(0.9*sin(iTime*0.5))*0.2)
    uv *= zoom + asin(0.9 * sin(evolveAccum)) * 0.2;

    // Horizontal scroll
    uv.x += scrollAccum;

    // Tile into [0,1] — NOT centered, matches reference fract(p*0.5)
    uv = fract(uv * tileScale);

    // IFS fold loop
    float m = 1000.0;
    int winIt = 0;

    for (int i = 0; i < iterations; i++) {
        uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;

        float l = abs(uv.x + asin(0.9 * sin(length(uv) * warpFreq)) * warpAmp);
        if (l < m) {
            m = l;
            winIt = i;
        }
    }

    // Stroke rendering (reference: smoothstep(.015, .01, m*.5))
    float stroke = smoothstep(thickness * 1.5, thickness, m * 0.5);

    // Drawing mask — progressive reveal creates growing-line effect
    stroke *= step(p2.x + float(winIt) * 0.1 - 0.8 + sin(uv.y * 2.0) * 0.1, 0.0);

    // Color from gradient LUT by iteration depth
    vec3 layerColor = texture(gradientLUT, vec2((float(winIt) + 0.5) / float(iterations), 0.5)).rgb;
    vec3 color = stroke * glowIntensity * layerColor;

    finalColor = vec4(color, 1.0);
}
