#version 330

// Glitch: video corruption simulation with CRT, Analog, Digital, and VHS modes
// Order: CRT+VHS (UV distort) -> Analog+Digital (stackable, chromatic aberration) -> Overlay (additive)

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

// Datamosh: variable resolution pixelation
uniform bool datamoshEnabled;
uniform float datamoshIntensity;
uniform float datamoshMin;
uniform float datamoshMax;
uniform float datamoshSpeed;
uniform float datamoshBands;

// Row Slice: horizontal displacement
uniform bool rowSliceEnabled;
uniform float rowSliceIntensity;
uniform float rowSliceBurstFreq;
uniform float rowSliceBurstPower;
uniform float rowSliceColumns;

// Column Slice: vertical displacement
uniform bool colSliceEnabled;
uniform float colSliceIntensity;
uniform float colSliceBurstFreq;
uniform float colSliceBurstPower;
uniform float colSliceRows;

// Diagonal Bands: 45-degree UV displacement
uniform bool diagonalBandsEnabled;
uniform float diagonalBandCount;
uniform float diagonalBandDisplace;
uniform float diagonalBandSpeed;

// Block Mask: random color tinting
uniform bool blockMaskEnabled;
uniform float blockMaskIntensity;
uniform int blockMaskMinSize;
uniform int blockMaskMaxSize;
uniform vec3 blockMaskTint;

// Temporal Jitter: radial spatial displacement
uniform bool temporalJitterEnabled;
uniform float temporalJitterAmount;
uniform float temporalJitterGate;

// Block Multiply: recursive UV folding with chromatic offset
uniform bool blockMultiplyEnabled;
uniform float blockMultiplySize;
uniform float blockMultiplyControl;
uniform int blockMultiplyIterations;
uniform float blockMultiplyIntensity;

out vec4 finalColor;

// Hash by David_Hoskins - converts vec3 to pseudo-random vec3 in [-1, 1]
vec3 hash33(vec3 p) {
    uvec3 q = uvec3(ivec3(p)) * uvec3(1597334673U, 3812015801U, 2798796415U);
    q = (q.x ^ q.y ^ q.z) * uvec3(1597334673U, 3812015801U, 2798796415U);
    return -1.0 + 2.0 * vec3(q) * (1.0 / float(0xffffffffU));
}

// Single float hash (for seeds)
float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

// Integer hash (for block indices)
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
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

// Chromatic spectrum offset for Block Multiply
vec3 spectrumOffset(float t) {
    float lo = step(t, 0.5);
    float hi = 1.0 - lo;
    float w = clamp((t - 1.0/6.0) / (4.0/6.0), 0.0, 1.0);
    w = 1.0 - abs(2.0 * w - 1.0);  // triangle ramp
    float neg_w = 1.0 - w;
    vec3 ret = vec3(lo, 1.0, hi) * vec3(neg_w, w, neg_w);
    return pow(ret, vec3(1.0 / 2.2));
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

    // Stage 1: UV distortions

    // Datamosh: variable resolution pixelation (first, before all other distortions)
    if (datamoshEnabled) {
        uint offset = uint(float(frame) / datamoshSpeed) + uint((uv.x + uv.y) * datamoshBands);
        float res = mix(datamoshMin, datamoshMax, hash11(float(offset)));
        vec2 pixelatedUv = floor(uv * res) / res;
        uv = mix(uv, pixelatedUv, datamoshIntensity);
    }

    // Diagonal Bands: 45-degree stripe displacement
    if (diagonalBandsEnabled) {
        float diagonal = uv.x + uv.y;
        float band = floor(diagonal * diagonalBandCount);
        float bandOffset = hash11(band + floor(t * diagonalBandSpeed)) * 2.0 - 1.0;
        uv += bandOffset * diagonalBandDisplace;
    }

    // Row Slice: horizontal rows that displace horizontally
    if (rowSliceEnabled) {
        uint seed = uint(gl_FragCoord.y + t * 60.0) / uint(rowSliceColumns);
        float gate = step(hash11(float(seed)), pow(abs(sin(t * rowSliceBurstFreq)), rowSliceBurstPower));
        float offset = (hash11(float(seed + 1u)) * 2.0 - 1.0) * gate * rowSliceIntensity;
        uv.x += offset;
    }

    // Column Slice: vertical columns that displace vertically
    if (colSliceEnabled) {
        uint seed = uint(gl_FragCoord.x + t * 60.0) / uint(colSliceRows);
        float gate = step(hash11(float(seed)), pow(abs(sin(t * colSliceBurstFreq)), colSliceBurstPower));
        float offset = (hash11(float(seed + 1u)) * 2.0 - 1.0) * gate * colSliceIntensity;
        uv.y += offset;
    }

    // CRT barrel distortion
    if (crtEnabled) {
        uv = crt(uv);
    }

    if (vhsEnabled) {
        // Tracking bars: horizontal displacement that travels vertically
        for (float i = 0.0; i < 0.71; i += 0.1313) {
            float d = mod(t * i, 1.7);
            float o = sin(1.0 - tan(t * 0.24 * i)) * trackingBarIntensity;
            uv.x += verticalBar(d, uv.y, o);
        }

        // Per-scanline noise jitter
        float uvY = floor(uv.y * 240.0) / 240.0;
        float noise = rand(vec2(t * 0.00001, uvY));
        uv.x += noise * scanlineNoiseIntensity;
    }

    // Stage 2: Analog + Digital (stackable, share chromatic aberration)
    vec3 col = vec3(0.0);

    // Chromatic aberration: Analog's static offset + VHS's drifting offset
    vec2 eps = vec2(aberration / resolution.x, 0.0);
    if (vhsEnabled) {
        eps.x += 0.006 * sin(t) * colorDriftIntensity;
    }

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

    // Block Mask: random block color tinting
    if (blockMaskEnabled) {
        int gridSize = (frame % (blockMaskMaxSize - blockMaskMinSize + 1)) + blockMaskMinSize;
        ivec2 blockCoord = ivec2(gl_FragCoord.xy) / gridSize;
        int index = blockCoord.y * int(resolution.x / float(gridSize)) + blockCoord.x;

        uint blockHash = hash(uint(index + frame / 6));
        int pattern = int(blockHash % 7u) + 2;
        float mask = (index % pattern == 0) ? 1.0 : 0.0;

        col = mix(col, col * blockMaskTint, mask * blockMaskIntensity);
    }

    // Temporal Jitter: radial spatial displacement (blends jittered color with existing)
    if (temporalJitterEnabled) {
        vec2 coord = gl_FragCoord.xy;
        float radial = (coord.x * coord.y) / (resolution.x * resolution.y * 0.25);
        float jitterSeed = hash11(radial + floor(t * 10.0));
        float gate = step(jitterSeed, temporalJitterGate);

        vec2 jitterOffset = vec2(hash11(radial), hash11(radial + 1.0)) * 2.0 - 1.0;
        vec2 jitteredUv = uv + jitterOffset * temporalJitterAmount;
        vec3 jitteredCol = texture(texture0, jitteredUv).rgb;
        col = mix(col, jitteredCol, gate);
    }

    // Block Multiply: recursive UV folding with chromatic offset
    if (blockMultiplyEnabled) {
        vec2 bmUv = uv;
        vec4 sum = texture(texture0, bmUv);

        float density = 68.0 - blockMultiplySize;  // Invert so larger size = larger blocks
        for (int i = 0; i < blockMultiplyIterations; i++) {
            // Recursive UV folding: mix between 1.0 (no effect) and block pattern
            vec2 blockBase = mix(vec2(1.0), fract(density * bmUv) + 0.5, blockMultiplyControl);
            bmUv /= pow(blockBase, vec2(0.1));

            // Clamp to prevent blowout
            sum = clamp(sum, 0.15, 1.0);

            // Cross-sampling with multiply/divide alternation
            float fi = float(i);
            vec2 px = 1.0 / resolution;
            sum /= 0.1 + 0.9 * clamp(texture(texture0, bmUv + vec2(px.x, fi * px.y)), 0.0, 2.0);
            sum *= 0.1 + 0.9 * clamp(texture(texture0, bmUv + vec2(px.x, -fi * px.y)), 0.0, 2.0);
            sum *= 0.1 + 0.9 * clamp(texture(texture0, bmUv + vec2(-fi * px.x, fi * px.y)), 0.0, 2.0);
            sum /= 0.1 + 0.9 * clamp(texture(texture0, bmUv + vec2(-fi * px.x, -fi * px.y)), 0.0, 2.0);

            // Chromatic spectrum offset based on luminance
            float lum = length(sum.xyz);
            sum.xyz /= 1.01 - 0.025 * spectrumOffset(1.0 - lum);
            sum.xyz *= 1.0 + 0.01 * spectrumOffset(lum);
        }

        // Normalize and contrast
        sum = 0.1 + 0.9 * sum;
        sum /= length(sum);
        sum = (-0.2 + 2.0 * sum) * 0.9;

        // Blend with existing color
        col = mix(col, sum.rgb, blockMultiplyIntensity);
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
