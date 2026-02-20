#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // previous accumulation (ping-pong read)
uniform sampler2D sceneTexture;  // current scene for slit sampling
uniform vec2 resolution;
uniform float slitPosition;      // 0-1 horizontal UV
uniform float speedDt;           // speed * deltaTime (precomputed on CPU)
uniform float pushAccel;
uniform float slitWidth;
uniform float brightness;

void main() {
    vec2 uv = fragTexCoord;
    float dx = uv.x - 0.5;
    float d = abs(dx);
    float signDx = sign(dx);

    // Outward push: linear base speed + perspective acceleration with distance
    // Min offset keeps left/right sides from reading across the center boundary
    float halfPixel = 0.5 / resolution.x;
    float shift = speedDt * (1.0 + d * pushAccel);
    float sampleD = max(halfPixel, d - shift);

    vec2 sampleUV = vec2(0.5 + signDx * sampleD, uv.y);
    vec3 old = texture(texture0, sampleUV).rgb;

    // Narrow injection zone (fixed ~2px), independent of slit width
    float injectionWidth = 2.0 / resolution.x;
    float slitMask = smoothstep(injectionWidth, 0.0, d);

    // Smoothly map injection zone to source band: left edge → right edge
    float halfSlit = slitWidth * 0.5;
    float t = clamp(dx / max(injectionWidth, 0.001), -1.0, 1.0);
    float slitU = fract(slitPosition + t * halfSlit);
    vec3 slitColor = texture(sceneTexture, vec2(slitU, uv.y)).rgb * brightness;

    finalColor = vec4(mix(old, slitColor, slitMask), 1.0);
}
