#version 330

// Surface Depth: luminance heightfield with parallax displacement and normal-based lighting

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int depthMode;
uniform float heightScale;
uniform float heightPower;
uniform int steps;
uniform vec2 viewAngle;
uniform int lighting;
uniform float lightAngle;
uniform float lightHeight;
uniform float intensity;
uniform float reliefScale;
uniform float shininess;
uniform int fresnelEnabled;
uniform float fresnelExponent;
uniform float fresnelIntensity;
uniform float time;

out vec4 finalColor;

// Height interpretation - converts texture luminance to height/depth

float getHeight(vec2 uv) {
    float lum = dot(texture(texture0, uv).rgb, vec3(0.299, 0.587, 0.114));
    return pow(lum, heightPower);
}

float getDepth(vec2 uv) {
    return 1.0 - getHeight(uv);
}

// Steep Parallax Mapping - ray-step with jitter, no interpolation

vec2 parallaxSimple(vec2 uv, vec3 viewDir) {
    float jitter = fract(sin(fract(time) + dot(uv, vec2(41.97, 289.13))) * 43758.5453);
    jitter *= min(0.25 + length(uv - 0.5), 1.0);

    vec2 p = viewDir.xy * heightScale;
    uv += p * 0.5;  // center the displacement range so mid-height stays put
    vec2 deltaUVs = p / float(steps);
    float layerDepth = 1.0 / float(steps);

    float d = layerDepth * jitter;
    uv -= deltaUVs * jitter;

    for (int i = 0; i < steps; i++) {
        float texDepth = getDepth(uv);
        if (d > texDepth) break;
        uv -= deltaUVs;
        d += layerDepth;
    }
    return uv;
}

// POM parallax - layer stepping with linear interpolation

vec2 parallaxPOM(vec2 uv, vec3 viewDir) {
    float numLayers = float(steps);
    float layerDepth = 1.0 / numLayers;

    vec2 p = viewDir.xy * heightScale;
    uv += p * 0.5;  // center the displacement range so mid-height stays put
    vec2 deltaUVs = p / numLayers;

    float texDepth = getDepth(uv);
    float d = 0.0;

    while (d < texDepth) {
        uv -= deltaUVs;
        texDepth = getDepth(uv);
        d += layerDepth;
    }

    // Linear interpolation between hit layer and previous layer
    vec2 lastUVs = uv + deltaUVs;
    float after = texDepth - d;
    float before = getDepth(lastUVs) - d + layerDepth;
    float w = after / (after - before);
    return mix(uv, lastUVs, w);
}

// Sobel normals - 9-tap 3x3 Sobel gradient, used for None mode

vec3 normalSobel(vec2 uv, vec2 texel) {
    vec4 n[9];
    n[0] = texture(texture0, uv + vec2(-texel.x, -texel.y));
    n[1] = texture(texture0, uv + vec2(    0.0, -texel.y));
    n[2] = texture(texture0, uv + vec2( texel.x, -texel.y));
    n[3] = texture(texture0, uv + vec2(-texel.x,     0.0));
    n[4] = texture(texture0, uv);
    n[5] = texture(texture0, uv + vec2( texel.x,     0.0));
    n[6] = texture(texture0, uv + vec2(-texel.x,  texel.y));
    n[7] = texture(texture0, uv + vec2(    0.0,  texel.y));
    n[8] = texture(texture0, uv + vec2( texel.x,  texel.y));

    vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    float gx = dot(sobelH.rgb, vec3(0.299, 0.587, 0.114));
    float gy = dot(sobelV.rgb, vec3(0.299, 0.587, 0.114));

    return normalize(vec3(-gx, -gy, reliefScale));
}

// 4-tap cross normals - used for Simple/POM modes

vec3 normal4Tap(vec2 uv, vec2 texel) {
    float r = getHeight(uv + vec2(texel.x, 0.0));
    float t = getHeight(uv + vec2(0.0, texel.y));
    float l = getHeight(uv - vec2(texel.x, 0.0));
    float b = getHeight(uv - vec2(0.0, texel.y));

    float dx = l - r;
    float dy = b - t;

    return normalize(vec3(dx, dy, reliefScale));
}

// Fresnel - rim glow

float fresnelTerm(vec3 normal, vec3 view, float exponent) {
    return pow(1.0 - clamp(dot(normalize(normal), normalize(view)), 0.0, 1.0), exponent);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    // Construct view direction from CPU-computed view angle
    vec3 viewDir = normalize(vec3(viewAngle, 1.0));

    // Depth mode dispatch: displace UV
    if (depthMode == 1) {
        uv = parallaxSimple(uv, viewDir);
    } else if (depthMode == 2) {
        uv = parallaxPOM(uv, viewDir);
    }

    vec4 texColor = texture(texture0, uv);
    vec3 result = texColor.rgb;

    // Hoist normal computation so it's shared by lighting and fresnel
    bool needNormal = (lighting != 0) || (fresnelEnabled != 0);
    vec3 normal = vec3(0.0, 0.0, 1.0);
    if (needNormal) {
        if (depthMode == 0) {
            normal = normalSobel(uv, texel);
        } else {
            normal = normal4Tap(uv, texel);
        }
    }

    // Lighting
    if (lighting != 0) {
        vec3 lightDir = normalize(vec3(cos(lightAngle), sin(lightAngle), lightHeight));

        // Lambertian diffuse with bias to avoid pure black
        float diffuse = dot(normal, lightDir) * 0.5 + 0.5;

        // Blinn-Phong specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), shininess);

        float lit = diffuse + spec;
        result = texColor.rgb * mix(1.0, lit, intensity);
    }

    // Fresnel rim glow (texture-derived color)
    if (fresnelEnabled != 0) {
        float f = fresnelTerm(normal, viewDir, fresnelExponent);
        result += f * fresnelIntensity * texColor.rgb;
    }

    finalColor = vec4(result, texColor.a);
}
