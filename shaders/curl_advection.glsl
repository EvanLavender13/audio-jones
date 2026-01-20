#version 430

layout(local_size_x = 16, local_size_y = 16) in;

// State texture: xy = velocity, z = divergence
layout(binding = 0) uniform sampler2D stateTexture;
layout(rgba16f, binding = 1) uniform writeonly image2D stateOut;
layout(rgba32f, binding = 2) uniform image2D trailMap;
layout(binding = 3) uniform sampler2D colorLUT;
layout(binding = 4) uniform sampler2D accumTexture;

uniform vec2 resolution;
uniform int steps;
uniform float advectionCurl;
uniform float curlScale;
uniform float laplacianScale;
uniform float pressureScale;
uniform float divergenceScale;
uniform float divergenceUpdate;
uniform float divergenceSmoothing;
uniform float selfAmp;
uniform float updateSmoothing;
uniform float injectionIntensity;
uniform float injectionThreshold;
uniform float value;

// Stencil weights for differential operators
#define _D 0.6
#define _K0 (-20.0 / 6.0)
#define _K1 (4.0 / 6.0)
#define _K2 (1.0 / 6.0)
#define _G0 0.25
#define _G1 0.125
#define _G2 0.0625

const float PI = 3.14159265359;

vec2 rotate2d(vec2 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

vec3 sampleState(vec2 uv) {
    return texture(stateTexture, uv).xyz;
}

// Compute curl and blur from shared neighbor samples
void computeCurlAndBlur(vec2 uv, vec2 texel, out float curl, out vec3 blur) {
    vec3 c      = sampleState(uv);
    vec3 uv_n   = sampleState(uv + vec2(0, texel.y));
    vec3 uv_s   = sampleState(uv + vec2(0, -texel.y));
    vec3 uv_e   = sampleState(uv + vec2(texel.x, 0));
    vec3 uv_w   = sampleState(uv + vec2(-texel.x, 0));
    vec3 uv_ne  = sampleState(uv + vec2(texel.x, texel.y));
    vec3 uv_nw  = sampleState(uv + vec2(-texel.x, texel.y));
    vec3 uv_se  = sampleState(uv + vec2(texel.x, -texel.y));
    vec3 uv_sw  = sampleState(uv + vec2(-texel.x, -texel.y));

    curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y
         + _D * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y
               + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);

    blur = _G0 * c
         + _G1 * (uv_n + uv_e + uv_w + uv_s)
         + _G2 * (uv_nw + uv_sw + uv_ne + uv_se);
}

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= int(resolution.x) || pos.y >= int(resolution.y)) {
        return;
    }

    vec2 uv = (vec2(pos) + 0.5) / resolution;
    vec2 texel = 1.0 / resolution;

    // Sample 9 neighbors
    vec3 center = sampleState(uv);
    vec3 uv_n  = sampleState(uv + vec2(0, texel.y));
    vec3 uv_s  = sampleState(uv + vec2(0, -texel.y));
    vec3 uv_e  = sampleState(uv + vec2(texel.x, 0));
    vec3 uv_w  = sampleState(uv + vec2(-texel.x, 0));
    vec3 uv_ne = sampleState(uv + vec2(texel.x, texel.y));
    vec3 uv_nw = sampleState(uv + vec2(-texel.x, texel.y));
    vec3 uv_se = sampleState(uv + vec2(texel.x, -texel.y));
    vec3 uv_sw = sampleState(uv + vec2(-texel.x, -texel.y));

    // Curl (rotation): dv/dx - du/dy
    float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y
        + _D * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y
              + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);

    // Divergence (expansion): du/dx + dv/dy
    float div = uv_s.y - uv_n.y - uv_e.x + uv_w.x
        + _D * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y
              + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);

    // Laplacian (diffusion)
    vec3 lapl = _K0 * center
              + _K1 * (uv_n + uv_e + uv_w + uv_s)
              + _K2 * (uv_nw + uv_sw + uv_ne + uv_se);

    // Multi-step self-advection
    vec2 off = center.xy;
    vec2 offd = off;
    vec3 ab = vec3(0);

    for (int i = 0; i < steps; i++) {
        vec2 sampleUV = uv - off * texel;
        float localCurl;
        vec3 localBlur;
        computeCurlAndBlur(sampleUV, texel, localCurl, localBlur);
        offd = rotate2d(offd, advectionCurl * localCurl);
        off += offd;
        ab += localBlur / float(steps);
    }

    // Field update equation
    float sp = pressureScale * lapl.z;
    float sc = curlScale * curl;
    float sd = center.z + divergenceUpdate * div + divergenceSmoothing * lapl.z;

    vec2 norm = length(center.xy) > 0.0 ? normalize(center.xy) : vec2(0);

    vec2 tab = selfAmp * ab.xy
             + laplacianScale * lapl.xy
             + norm * sp
             + center.xy * divergenceScale * sd;

    vec2 rab = rotate2d(tab, sc);

    vec3 result = mix(vec3(rab, sd), center.xyz, updateSmoothing);

    // Accumulation-based energy injection
    if (injectionIntensity > 0.0) {
        vec4 accumSample = texture(accumTexture, uv);
        float brightness = dot(accumSample.rgb, vec3(0.299, 0.587, 0.114));
        float contribution = max(brightness - injectionThreshold, 0.0);

        vec2 injDir = length(result.xy) > 0.001 ? normalize(result.xy) : vec2(1.0, 0.0);
        result.xy += injectionIntensity * contribution * injDir;
    }

    // Clamp to valid range
    result.z = clamp(result.z, -1.0, 1.0);
    if (length(result.xy) > 1.0) {
        result.xy = normalize(result.xy);
    }

    // Write state
    imageStore(stateOut, pos, vec4(result, 0.0));

    // Color output: velocity angle maps to LUT
    float angle = atan(result.y, result.x);
    float t = (angle + PI) / (2.0 * PI);
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
    float brightness = length(result.xy);

    // Write to trail map
    vec4 trailColor = vec4(color * brightness * value, brightness * value);
    imageStore(trailMap, pos, trailColor);
}
