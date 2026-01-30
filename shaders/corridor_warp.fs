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

    // Mode handling: early-out for wrong side
    // depth < 0 = floor, depth > 0 = ceiling
    if (mode == 0 && depth >= 0.0) { // FLOOR mode - only render below horizon
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    if (mode == 1 && depth <= 0.0) { // CEILING mode - only render above horizon
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    // mode == 2 (CORRIDOR) renders both sides

    // Distance from horizon (always positive)
    float dist = max(abs(depth), 0.001);

    // Perspective projection: farther from horizon = closer to camera
    // Divide by dist to get infinite plane projection
    float planeScale = perspectiveStrength / dist;
    vec2 planeUV = vec2(screen.x * planeScale, planeScale);

    // Apply floor/ceiling rotation (spins just the texture)
    planeUV *= rotate2d(planeRotation);

    // Scale and scroll
    planeUV *= scale;
    planeUV += vec2(strafeOffset, scrollOffset);

    // Sample with mirror repeat for seamless tiling
    vec2 sampleUV = 1.0 - abs(mod(planeUV, 2.0) - 1.0);
    vec4 color = texture(texture0, sampleUV);

    // Apply fog based on distance from horizon
    // Small dist = near horizon = far from camera = dark
    // Large dist = far from horizon = near camera = bright
    float fog = pow(dist, fogStrength);
    fog = clamp(fog, 0.0, 1.0);
    color.rgb *= fog;

    finalColor = color;
}
