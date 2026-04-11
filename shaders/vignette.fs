#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float intensity;
uniform float radius;
uniform float softness;
uniform float roundness;
uniform vec3 vignetteColor;

float sdSquare(vec2 point, float width) {
    vec2 d = abs(point) - width;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

void main() {
    vec4 color = texture(texture0, fragTexCoord);

    // Center UV to (-0.5, 0.5) range
    vec2 uv = fragTexCoord - 0.5;

    // SDF shape - symmetric size simplifies TyLindberg UV clamping away
    // roundness=1: boxSize=0, dist=length(uv)-radius -> circle
    // roundness=0: boxSize=radius, dist=sdSquare(uv,radius) -> rectangle
    float boxSize = radius * (1.0 - roundness);
    float dist = sdSquare(uv, boxSize) - (radius * roundness);

    // Falloff and blend (GPUImage approach)
    float percent = smoothstep(0.0, softness, dist) * intensity;
    finalColor = vec4(mix(color.rgb, vignetteColor, percent), color.a);
}
