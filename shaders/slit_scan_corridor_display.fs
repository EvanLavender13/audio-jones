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

    // Gentle vertical widening at edges (no vanishing-point pinch)
    // perspective=0: flat walls, perspective>0: subtle spread near viewer
    float scale = 1.0 + perspective * dist * 2.0;
    float wallY = (uv.y - 0.5) / scale + 0.5;

    vec2 bufferUV = vec2(uv.x, clamp(wallY, 0.0, 1.0));
    vec3 color = texture(texture0, bufferUV).rgb;

    // Depth fog: center (small dist) = far/dim, edges (large dist) = near/bright
    float fog = pow(clamp(dist * 2.0, 0.0, 1.0), fogStrength);
    color *= fog;

    finalColor = vec4(color, 1.0);
}
