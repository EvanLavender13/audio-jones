#version 330

// Heightfield Relief: Surface normal from luminance gradient with directional lighting
// Creates embossed 3D appearance from texture structure

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float intensity;
uniform float reliefScale;
uniform float lightAngle;
uniform float lightHeight;
uniform float shininess;

out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    vec4 texColor = texture(texture0, uv);

    // Sample 3x3 neighborhood for Sobel
    vec4 n[9];
    n[0] = texture(texture0, uv + vec2(-texel.x, -texel.y));
    n[1] = texture(texture0, uv + vec2(    0.0, -texel.y));
    n[2] = texture(texture0, uv + vec2( texel.x, -texel.y));
    n[3] = texture(texture0, uv + vec2(-texel.x,     0.0));
    n[4] = texColor;
    n[5] = texture(texture0, uv + vec2( texel.x,     0.0));
    n[6] = texture(texture0, uv + vec2(-texel.x,  texel.y));
    n[7] = texture(texture0, uv + vec2(    0.0,  texel.y));
    n[8] = texture(texture0, uv + vec2( texel.x,  texel.y));

    // Sobel horizontal and vertical gradients
    vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    // Convert RGB gradients to luminance
    float gx = dot(sobelH.rgb, vec3(0.299, 0.587, 0.114));
    float gy = dot(sobelV.rgb, vec3(0.299, 0.587, 0.114));

    // Normal from gradient: reliefScale controls steepness (higher = flatter)
    vec3 normal = normalize(vec3(-gx, -gy, reliefScale));

    // Light direction from angle and height
    vec3 lightDir = normalize(vec3(cos(lightAngle), sin(lightAngle), lightHeight));

    // Lambertian diffuse with bias to avoid pure black
    float diffuse = dot(normal, lightDir) * 0.5 + 0.5;

    // Blinn-Phong specular (orthographic view assumption)
    vec3 viewDir = vec3(0.0, 0.0, 1.0);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), shininess);

    // Combine lighting and blend with original
    float lighting = diffuse + spec;
    vec3 result = texColor.rgb * mix(1.0, lighting, intensity);

    finalColor = vec4(result, texColor.a);
}
