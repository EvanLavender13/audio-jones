#version 330

// Oil Paint Relief: Gradient-derived normal mapping for paint surface lighting
// Applies diffuse + specular shading to simulate 3D paint thickness

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float specular;

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;
    float delta = 1.0 / resolution.y;
    vec2 d = vec2(delta, 0.0);

    float val_l = length(texture(texture0, uv - d.xy).rgb);
    float val_r = length(texture(texture0, uv + d.xy).rgb);
    float val_d = length(texture(texture0, uv - d.yx).rgb);
    float val_u = length(texture(texture0, uv + d.yx).rgb);
    vec2 grad = vec2(val_r - val_l, val_u - val_d) / delta;

    vec3 n = normalize(vec3(grad, 150.0));
    vec3 light = normalize(vec3(-1.0, 1.0, 1.4));
    float diff = clamp(dot(n, light), 0.0, 1.0);
    float spec = pow(clamp(dot(reflect(light, n), vec3(0, 0, -1)), 0.0, 1.0), 12.0) * specular;
    float shadow = pow(clamp(dot(reflect(light * vec3(-1, -1, 1), n), vec3(0, 0, -1)), 0.0, 1.0), 4.0) * 0.1;

    vec3 tint = vec3(0.85, 1.0, 1.15);
    finalColor.rgb = texture(texture0, uv).rgb * mix(diff, 1.0, 0.9)
                 + spec * tint + shadow * tint;
    finalColor.a = 1.0;
}
