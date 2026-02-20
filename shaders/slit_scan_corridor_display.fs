#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;  // corridor ping-pong result
uniform float rotation;
uniform float perspective;
uniform float fogStrength;

void main() {
    vec2 uv = fragTexCoord;

    // Optional rotation
    vec2 centered = uv - vec2(0.5);
    float c = cos(rotation), s = sin(rotation);
    uv = vec2(c * centered.x + s * centered.y,
              -s * centered.x + c * centered.y) + vec2(0.5);

    // Horizontal distance from slit (center)
    float dist = max(abs(uv.x - 0.5), 0.001);

    // 1/dist perspective projection on the y axis
    float wallY = (uv.y - 0.5) * perspective / dist;

    // Outside tunnel walls -> black
    float wallEdge = smoothstep(0.5, 0.47, abs(wallY));
    if (wallEdge <= 0.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 bufferUV = vec2(uv.x, wallY + 0.5);
    vec3 color = texture(texture0, bufferUV).rgb;

    // Depth fog: center (small dist) = far/dim, edges (large dist) = near/bright
    float fog = pow(clamp(dist * 2.0, 0.0, 1.0), fogStrength);
    color *= fog;

    finalColor = vec4(color * wallEdge, 1.0);
}
