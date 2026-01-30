// shaders/surface_warp.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float intensity;
uniform float angle;
uniform float rotateSpeed;
uniform float scrollSpeed;
uniform float depthShade;

void main() {
    vec2 uv = fragTexCoord;

    // Step 1: Rotate into warp space
    float effectiveAngle = angle + rotateSpeed * time;
    float c = cos(effectiveAngle);
    float s = sin(effectiveAngle);
    mat2 rot = mat2(c, -s, s, c);
    vec2 ruv = (uv - 0.5) * rot;

    // Step 2: Compute wave height (layered sines at irrational ratios)
    float t = scrollSpeed * time;
    float wave = sin(ruv.x * 2.0 + t * 0.7) * 0.5
               + sin(ruv.x * 3.7 + t * 1.1) * 0.3
               + sin(ruv.x * 5.3 + t * 0.4) * 0.2;

    // Step 3: Stretch perpendicular axis (exp ensures multiplier >= 1)
    float m = exp(abs(wave) * intensity);
    ruv.y *= m;

    // Step 4: Rotate back and sample with mirror repeat edges
    mat2 rotBack = mat2(c, s, -s, c);
    vec2 warpedUV = ruv * rotBack + 0.5;
    warpedUV = 1.0 - abs(mod(warpedUV, 2.0) - 1.0);  // Mirror repeat
    vec4 color = texture(texture0, warpedUV);

    // Step 5: Apply depth shading (darkens valleys)
    float shade = mix(1.0, smoothstep(0.0, 2.0, m), depthShade);
    color.rgb *= shade;

    finalColor = color;
}
