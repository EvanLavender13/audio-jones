#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float horizon;            // 0.0-1.0 vertical position
uniform float perspectiveStrength;
uniform int mode;                 // 0=floor, 1=ceiling, 2=corridor
uniform float viewRotation;       // Pre-accumulated scene rotation
uniform float planeRotation;      // Pre-accumulated floor rotation
uniform float scale;
uniform float scrollOffset;       // Pre-accumulated scroll
uniform float strafeOffset;       // Pre-accumulated strafe
uniform float fogStrength;

mat2 rotate2d(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

void main() {
    // Center coordinates, normalize by height for aspect-correct projection
    vec2 screen = (fragTexCoord - 0.5) * vec2(resolution.x / resolution.y, 1.0);

    // Apply view rotation (spins whole scene)
    screen *= rotate2d(viewRotation);

    // Compute vertical distance from horizon
    float horizonY = horizon - 0.5;
    float depth = screen.y - horizonY;

    // Mode handling: determine if we should render this pixel
    bool isFloor = (depth < 0.0);
    bool isCeiling = (depth > 0.0);

    if (mode == 0 && !isFloor) { // FLOOR mode
        finalColor = vec4(0.0);
        return;
    }
    if (mode == 1 && !isCeiling) { // CEILING mode
        finalColor = vec4(0.0);
        return;
    }

    // For corridor mode, mirror depth for ceiling
    if (mode == 2 && isCeiling) {
        depth = -depth;
    }

    // Avoid division by zero near horizon
    float safeDepth = max(abs(depth), 0.001);
    if (depth < 0.0) safeDepth = -safeDepth;

    // Perspective projection onto infinite plane
    vec2 planeUV = vec2(screen.x / -safeDepth, perspectiveStrength / -safeDepth);

    // Apply floor rotation (spins just the texture)
    planeUV *= rotate2d(planeRotation);

    // Scale and scroll
    planeUV *= scale;
    planeUV += vec2(strafeOffset, scrollOffset);

    // Sample with mirror repeat for seamless tiling
    vec2 sampleUV = 1.0 - abs(mod(planeUV, 2.0) - 1.0);
    vec4 color = texture(texture0, sampleUV);

    // Apply fog based on depth (further = darker)
    float fog = pow(safeDepth, fogStrength);
    fog = clamp(fog, 0.0, 1.0);
    color.rgb *= fog;

    finalColor = color;
}
