// shaders/surface_warp.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float intensity;
uniform float angle;
uniform float rotation;      // Pre-accumulated rotation (avoids jumps on speed change)
uniform float scrollOffset;  // Pre-accumulated scroll (avoids jumps on speed change)
uniform float depthShade;

void main() {
    vec2 uv = fragTexCoord;

    // Step 1: Rotate into warp space
    float effectiveAngle = angle + rotation;
    float c = cos(effectiveAngle);
    float s = sin(effectiveAngle);
    mat2 rot = mat2(c, -s, s, c);
    vec2 ruv = (uv - 0.5) * rot;

    // Step 2: Compute wave height (layered sines at irrational ratios)
    float t = scrollOffset;
    float wave = sin(ruv.x * 2.0 + t * 0.7) * 0.35
               + sin(ruv.x * 3.7 + t * 1.1) * 0.25
               + sin(ruv.x * 5.3 + t * 0.4) * 0.18
               + sin(ruv.x * 7.9 + t * 0.9) * 0.12
               + sin(ruv.x * 11.3 + t * 1.7) * 0.10;

    // Step 3: Stretch perpendicular axis (soft abs smooths the sharp tip)
    float m = exp(sqrt(wave * wave + 0.01) * intensity);
    ruv.y *= m;

    // Step 4: Rotate back and sample with mirror repeat edges
    mat2 rotBack = mat2(c, s, -s, c);
    vec2 warpedUV = ruv * rotBack + 0.5;
    warpedUV = 1.0 - abs(mod(warpedUV, 2.0) - 1.0);  // Mirror repeat
    vec4 color = texture(texture0, warpedUV);

    // Step 5: Apply depth shading (darkens stretched ridges, floors at 20%)
    float shadeM = m - 1.0;  // Offset so flat areas start at 0
    float shade = mix(1.0, 1.0 - smoothstep(0.0, 1.5, shadeM) * 0.8, depthShade);
    color.rgb *= shade;

    finalColor = color;
}
