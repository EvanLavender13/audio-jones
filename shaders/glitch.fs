#version 330

// Glitch: video corruption simulation with CRT, Analog, Digital, and VHS modes
// Order: CRT barrel (UV distort) -> Analog+Digital (stackable) -> VHS -> Overlay (additive)

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform int frame;

// CRT mode
uniform bool crtEnabled;
uniform float curvature;
uniform bool vignetteEnabled;

// Analog mode (enabled when analogIntensity > 0)
uniform float analogIntensity;
uniform float aberration;

// Digital mode (enabled when blockThreshold > 0)
uniform float blockThreshold;
uniform float blockOffset;

// VHS mode
uniform bool vhsEnabled;
uniform float trackingBarIntensity;
uniform float scanlineNoiseIntensity;
uniform float colorDriftIntensity;

// Overlay
uniform float scanlineAmount;
uniform float noiseAmount;

out vec4 finalColor;

// Hash by David_Hoskins - converts vec3 to pseudo-random vec3 in [-1, 1]
vec3 hash33(vec3 p) {
    uvec3 q = uvec3(ivec3(p)) * uvec3(1597334673U, 3812015801U, 2798796415U);
    q = (q.x ^ q.y ^ q.z) * uvec3(1597334673U, 3812015801U, 2798796415U);
    return -1.0 + 2.0 * vec3(q) * (1.0 / float(0xffffffffU));
}

// Gradient noise by iq - returns [-1, 1]
float gnoise(vec3 x) {
    vec3 p = floor(x);
    vec3 w = fract(x);
    vec3 u = w * w * w * (w * (w * 6.0 - 15.0) + 10.0);  // quintic interpolant

    vec3 ga = hash33(p + vec3(0,0,0)); vec3 gb = hash33(p + vec3(1,0,0));
    vec3 gc = hash33(p + vec3(0,1,0)); vec3 gd = hash33(p + vec3(1,1,0));
    vec3 ge = hash33(p + vec3(0,0,1)); vec3 gf = hash33(p + vec3(1,0,1));
    vec3 gg = hash33(p + vec3(0,1,1)); vec3 gh = hash33(p + vec3(1,1,1));

    float va = dot(ga, w - vec3(0,0,0)); float vb = dot(gb, w - vec3(1,0,0));
    float vc = dot(gc, w - vec3(0,1,0)); float vd = dot(gd, w - vec3(1,1,0));
    float ve = dot(ge, w - vec3(0,0,1)); float vf = dot(gf, w - vec3(1,0,1));
    float vg = dot(gg, w - vec3(0,1,1)); float vh = dot(gh, w - vec3(1,1,1));

    return 2.0 * (va + u.x*(vb-va) + u.y*(vc-va) + u.z*(ve-va) +
                  u.x*u.y*(va-vb-vc+vd) + u.y*u.z*(va-vc-ve+vg) +
                  u.z*u.x*(va-vb-ve+vf) + u.x*u.y*u.z*(-va+vb+vc-vd+ve-vf-vg+vh));
}

float gnoise01(vec3 x) { return 0.5 + 0.5 * gnoise(x); }

// Simple rand for VHS scanline noise
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// CRT barrel distortion
vec2 crt(vec2 uv) {
    vec2 centered = uv * 2.0 - 1.0;
    float r = length(centered);
    r /= (1.0 - curvature * r * r);
    float theta = atan(centered.y, centered.x);
    return vec2(r * cos(theta), r * sin(theta)) * 0.5 + 0.5;
}

// VHS vertical tracking bar displacement
float verticalBar(float pos, float uvY, float offset) {
    float range = 0.05;
    float x = smoothstep(pos - range, pos, uvY) * offset;
    x -= smoothstep(pos, pos + range, uvY) * offset;
    return x;
}

void main()
{
    vec2 uv = fragTexCoord;
    float t = time;

    // Stage 1: CRT barrel distortion (UV-level)
    if (crtEnabled) {
        uv = crt(uv);
    }

    // Stage 2: Analog + Digital (stackable like reference shader)
    vec3 col = vec3(0.0);
    vec2 eps = vec2(aberration / resolution.x, 0.0);

    // Analog: enabled when analogIntensity > 0
    if (analogIntensity > 0.0) {
        float y = uv.y * resolution.y;

        // Scale intensity like reference: (glitchAmount * 4.0 + 0.1)
        float distortion = gnoise(vec3(0.0, y * 0.01, t * 500.0)) * (analogIntensity * 4.0 + 0.1);
        distortion *= gnoise(vec3(0.0, y * 0.02, t * 250.0)) * (analogIntensity * 2.0 + 0.025);

        // Sync pulse artifacts
        distortion += smoothstep(0.999, 1.0, sin((uv.y + t * 1.6) * 2.0)) * 0.02;
        distortion -= smoothstep(0.999, 1.0, sin((uv.y + t) * 2.0)) * 0.02;

        vec2 st = uv + vec2(distortion, 0.0);

        // Chromatic aberration: R+offset, G center, B-offset
        col.r = texture(texture0, st + eps).r;
        col.g = texture(texture0, st).g;
        col.b = texture(texture0, st - eps).b;
    }
    else {
        // No analog: sample normally
        col = texture(texture0, uv).rgb;
    }

    // Digital: enabled when blockThreshold > 0, modifies color (stacks with analog)
    if (blockThreshold > 0.0) {
        float bt = floor(t * 30.0) * 300.0;

        float blockNoiseX = step(gnoise01(vec3(0.0, uv.x * 3.0, bt)), blockThreshold);
        float blockNoiseX2 = step(gnoise01(vec3(0.0, uv.x * 1.5, bt * 1.2)), blockThreshold);
        float blockNoiseY = step(gnoise01(vec3(0.0, uv.y * 4.0, bt)), blockThreshold);
        float blockNoiseY2 = step(gnoise01(vec3(0.0, uv.y * 6.0, bt * 1.2)), blockThreshold);
        float block = blockNoiseX2 * blockNoiseY2 + blockNoiseX * blockNoiseY;

        vec2 st = vec2(uv.x + sin(bt) * hash33(vec3(uv, 0.5)).x * blockOffset, uv.y);

        // Multiply existing color by (1 - block), then add block-displaced samples
        col *= 1.0 - block;
        block *= 1.15;
        col.r += texture(texture0, st + eps).r * block;
        col.g += texture(texture0, st).g * block;
        col.b += texture(texture0, st - eps).b * block;
    }

    // VHS: separate effect (tracking bars + scanline noise)
    if (vhsEnabled) {
        vec2 vhsUV = uv;

        // Multiple traveling bars
        for (float i = 0.0; i < 0.71; i += 0.1313) {
            float d = mod(t * i, 1.7);
            float o = sin(1.0 - tan(t * 0.24 * i)) * trackingBarIntensity;
            vhsUV.x += verticalBar(d, vhsUV.y, o);
        }

        // Per-scanline noise
        float uvY = floor(vhsUV.y * 240.0) / 240.0;
        float noise = rand(vec2(t * 0.00001, uvY));
        vhsUV.x += noise * scanlineNoiseIntensity;

        // Drifting chromatic aberration
        vec2 offsetR = vec2(0.006 * sin(t), 0.0) * colorDriftIntensity;
        vec2 offsetG = vec2(0.0073 * cos(t * 0.97), 0.0) * colorDriftIntensity;

        // VHS replaces the color (different aesthetic from analog/digital)
        col.r = texture(texture0, vhsUV + offsetR).r;
        col.g = texture(texture0, vhsUV + offsetG).g;
        col.b = texture(texture0, vhsUV).b;
    }

    // Stage 3: Overlay effects (white noise + scanlines)
    col += hash33(vec3(gl_FragCoord.xy, mod(float(frame), 1000.0))).r * noiseAmount;
    col -= sin(4.0 * t + uv.y * resolution.y * 1.75) * scanlineAmount;

    // CRT vignette
    if (crtEnabled && vignetteEnabled) {
        float vig = 8.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
        col *= pow(vig, 0.25) * 1.5;
    }

    // Clamp out-of-bounds UVs to black (for CRT barrel)
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        col = vec3(0.0);
    }

    finalColor = vec4(col, 1.0);
}
