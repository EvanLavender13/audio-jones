#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float cellSize;
uniform float trailLength;
uniform int fallerCount;
uniform float overlayIntensity;
uniform float refreshRate;
uniform float leadBrightness;
uniform float time;
uniform int sampleMode;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec4 hash4(vec2 p) {
    return fract(sin(vec4(
        dot(p, vec2(127.1, 311.7)),
        dot(p, vec2(269.5, 183.3)),
        dot(p, vec2(419.2, 371.9)),
        dot(p, vec2(347.1, 247.5))
    )) * 43758.5453);
}

float rune_line(vec2 p, vec2 a, vec2 b) {
    p -= a; b -= a;
    float h = clamp(dot(p, b) / dot(b, b), 0.0, 1.0);
    return length(p - b * h);
}

float rune(vec2 uv, vec2 seed, float highlight) {
    float d = 1e5;
    for (int i = 0; i < 4; i++) {
        vec4 pos = hash4(seed);
        seed += 1.0;
        if (i == 0) pos.y = 0.0;
        if (i == 1) pos.x = 0.999;
        if (i == 2) pos.x = 0.0;
        if (i == 3) pos.y = 0.999;
        vec4 snaps = vec4(2, 3, 2, 3);
        pos = (floor(pos * snaps) + 0.5) / snaps;
        if (pos.xy != pos.zw)
            d = min(d, rune_line(uv, pos.xy, pos.zw + 0.001));
    }
    return smoothstep(0.1, 0.0, d) + highlight * smoothstep(0.4, 0.0, d);
}

void main() {
    vec4 original = texture(texture0, fragTexCoord);

    // Grid division
    vec2 cellSizeUV = vec2(cellSize) / resolution;
    vec2 cellID = floor(fragTexCoord / cellSizeUV);
    vec2 localUV = fract(fragTexCoord / cellSizeUV);
    float col = cellID.x;
    float row = cellID.y;

    // Rain trail mask
    float brightness = 0.0;
    float charDist = trailLength;
    float numRows = resolution.y / cellSize;
    for (int i = 0; i < fallerCount; i++) {
        float fSpeed = hash(vec2(col, float(i))) * 0.7 + 0.3;
        float fOffset = hash(vec2(col, float(i) + 100.0));
        float headPos = fract(-time * fSpeed + fOffset) * (numRows + trailLength);
        float dist = headPos - row;
        if (dist > 0.0 && dist < trailLength) {
            float t = dist / trailLength;
            float fade = 1.0 - t;
            brightness += fade;
            if (dist < 1.0) brightness += leadBrightness;
            charDist = min(charDist, dist);
        }
    }
    brightness = clamp(brightness, 0.0, 1.0 + leadBrightness);

    // Character animation
    float timeFactor;
    if (charDist < 1.0) {
        timeFactor = floor(time * refreshRate * 5.0);
    } else {
        float baseRate = hash(vec2(col, 2.0));
        float charRate = pow(hash(vec2(charDist, col)), 4.0);
        timeFactor = floor(time * refreshRate * (baseRate + charRate * 4.0));
    }
    vec2 charSeed = vec2(col + timeFactor, row);

    // Color
    vec3 rainColor = mix(vec3(0.0, 0.2, 0.0), vec3(0.67, 1.0, 0.82), clamp(brightness, 0.0, 1.0));

    // Character mask
    float charMask = rune(localUV, charSeed, clamp(brightness - 1.0, 0.0, 1.0));

    // Compositing
    float rainAlpha = charMask * clamp(brightness, 0.0, 1.0) * overlayIntensity;

    vec3 result;
    if (sampleMode != 0) {
        // Glyphs take color from source texture, gaps go black
        result = original.rgb * rainAlpha;
    } else {
        result = mix(original.rgb, rainColor, rainAlpha);
    }
    finalColor = vec4(result, original.a);
}
