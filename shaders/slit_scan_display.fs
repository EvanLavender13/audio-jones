#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int mode;
uniform float rotation;
uniform float perspective;
uniform float fogStrength;
uniform float center;
uniform float glow;

void main() {
    vec2 uv = fragTexCoord;

    // Aspect-corrected rotation around screen center
    float aspect = resolution.x / resolution.y;
    vec2 centered = uv - vec2(0.5, 0.5);
    centered.x *= aspect;
    float c = cos(rotation), s = sin(rotation);
    centered = vec2(c * centered.x + s * centered.y,
                    -s * centered.x + c * centered.y);
    centered.x /= aspect;
    uv = centered + vec2(0.5, 0.5);

    vec3 color;

    if (mode == 1) {
        // Flat mode: straight passthrough, no depth effects
        color = texture(texture0, uv).rgb;
    } else {
        // Corridor mode: perspective, fog, glow
        float dist = max(abs(uv.x - center), 0.001);
        float maxDist = max(center, 1.0 - center);
        float normDist = clamp(dist / maxDist, 0.0, 1.0);

        float scale = 1.0 + perspective * normDist;
        float wallY = (uv.y - 0.5) / scale + 0.5;

        vec2 bufferUV = vec2(uv.x, clamp(wallY, 0.0, 1.0));
        color = texture(texture0, bufferUV).rgb;

        float fog = pow(normDist, fogStrength);
        color *= fog;

        // Center glow: additive white for blown-out center seam
        if (glow > 0.0) {
            float g = pow(1.0 - normDist, 6.0 - glow);
            color += vec3(g);
        }
    }

    finalColor = vec4(color, 1.0);
}
