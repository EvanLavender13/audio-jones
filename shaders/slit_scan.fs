#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D sceneTexture;
uniform vec2 resolution;
uniform float slitPosition;
uniform float speedDt;
uniform float pushAccel;
uniform float slitWidth;
uniform float brightness;
uniform float center;

void main() {
    vec2 uv = fragTexCoord;
    float dx = uv.x - center;
    float d = abs(dx);
    float signDx = sign(dx);

    float halfPixel = 0.5 / resolution.x;
    float shift = speedDt * (1.0 + d * pushAccel);
    float sampleD = max(halfPixel, d - shift);

    vec2 sampleUV = vec2(center + signDx * sampleD, uv.y);
    vec3 old = texture(texture0, sampleUV).rgb;

    float injectionWidth = 2.0 / resolution.x;
    float slitMask = smoothstep(injectionWidth, 0.0, d);

    float halfSlit = slitWidth * 0.5;
    float t = clamp(dx / max(injectionWidth, 0.001), -1.0, 1.0);
    float slitU = fract(slitPosition + t * halfSlit);
    vec3 slitColor = texture(sceneTexture, vec2(slitU, uv.y)).rgb * brightness;

    finalColor = vec4(mix(old, slitColor, slitMask), 1.0);
}
