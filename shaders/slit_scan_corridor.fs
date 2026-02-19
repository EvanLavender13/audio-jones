#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // previous accumulation (ping-pong read)
uniform sampler2D sceneTexture;  // current scene for slit sampling
uniform vec2 resolution;
uniform float slitPosition;      // 0-1 horizontal UV
uniform float speedDt;           // speed * deltaTime (precomputed on CPU)
uniform float perspective;
uniform float slitWidth;
uniform float decayFactor;
uniform float brightness;

void main() {
    vec2 uv = fragTexCoord;
    float dx = uv.x - 0.5;
    float d = abs(dx);

    // Perspective-accelerated inward sample
    // Near center (d~0): sampleD ~ d (barely moves)
    // Near edges (d~0.5): sampleD << d (rushes past)
    float sampleD = d / (1.0 + speedDt * d * perspective);
    float signDx = sign(dx);
    // Handle dx == 0: sign(0) = 0, but sampleD = 0 too so sampleU = 0.5
    vec2 sampleUV = vec2(0.5 + signDx * sampleD, uv.y);

    // Sample old accumulation with decay
    vec4 old = texture(texture0, sampleUV) * decayFactor;

    // Stamp fresh slit at center
    float halfPixel = 0.5 / resolution.x;
    float slitMask = smoothstep(slitWidth, 0.0, d);

    // Left of center: left pixel of slit; right of center: right pixel
    float slitU = (dx < 0.0)
        ? (slitPosition - halfPixel)
        : (slitPosition + halfPixel);
    vec4 slitColor = texture(sceneTexture, vec2(slitU, uv.y)) * brightness;

    // Vertical vignette (top/bottom fade to black — no floor/ceiling)
    float vy = abs(uv.y - 0.5) * 2.0;
    slitColor *= smoothstep(1.0, 0.7, vy);

    finalColor = mix(old, slitColor, slitMask);
}
