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
uniform float brightness;

void main() {
    vec2 uv = fragTexCoord;
    float dx = uv.x - 0.5;
    float d = abs(dx);
    float signDx = sign(dx);

    // Outward push: linear base speed + perspective acceleration with distance
    float shift = speedDt * (1.0 + d * perspective);
    float sampleD = max(0.0, d - shift);

    vec2 sampleUV = vec2(0.5 + signDx * sampleD, uv.y);
    vec3 old = texture(texture0, sampleUV).rgb;

    // Stamp fresh slit at center
    float slitWidthUV = slitWidth / resolution.x;
    float slitMask = smoothstep(slitWidthUV, 0.0, d);

    // Map output position into source band: left half feeds left, right half feeds right
    float slitU = slitPosition + dx;
    vec3 slitColor = texture(sceneTexture, vec2(slitU, uv.y)).rgb * brightness;

    finalColor = vec4(mix(old, slitColor, slitMask), 1.0);
}
