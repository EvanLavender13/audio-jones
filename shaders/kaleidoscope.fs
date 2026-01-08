#version 330

// Kaleidoscope post-process effect
// Blends multiple radial symmetry techniques via intensity sliders

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

// Shared parameters
uniform int segments;         // Mirror segments (1 = disabled)
uniform float rotation;       // Pattern rotation (radians)
uniform float time;           // Animation time (seconds)
uniform float twistAngle;     // Radial twist amount (radians)
uniform vec2 focalOffset;     // Lissajous center offset (UV units)
uniform float warpStrength;   // fBM warp intensity (0 = disabled)
uniform float warpSpeed;      // fBM animation speed
uniform float noiseScale;     // fBM spatial scale

// Technique intensities
uniform float polarIntensity;
uniform float kifsIntensity;
uniform float drosteIntensity;
uniform float iterMirrorIntensity;
uniform float hexFoldIntensity;
uniform float powerMapIntensity;

// KIFS params
uniform int kifsIterations;
uniform float kifsScale;
uniform vec2 kifsOffset;

// Droste params
uniform float drosteScale;


// Hex Fold params
uniform float hexScale;

// Power Map params
uniform float powerMapN;

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const vec2 HEX_S = vec2(1.0, 1.7320508);  // 1, sqrt(3)

// Hash function for gradient noise
vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy) * 2.0 - 1.0;
}

// 2D gradient noise
float gnoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(dot(hash22(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)),
                   dot(hash22(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
               mix(dot(hash22(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
                   dot(hash22(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x), u.y);
}

// fBM: 4 octaves, lacunarity=2.0, gain=0.5
float fbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * gnoise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// 2D rotation
vec2 rotate2d(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

// Mirror UV into 0-1 range
vec2 mirror(vec2 x)
{
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

// Get hex cell coordinates
vec4 getHex(vec2 p)
{
    vec4 hC = floor(vec4(p, p - vec2(0.5, 1.0)) / HEX_S.xyxy) + 0.5;
    vec4 h = vec4(p - hC.xy * HEX_S, p - (hC.zw + 0.5) * HEX_S);
    return dot(h.xy, h.xy) < dot(h.zw, h.zw)
        ? vec4(h.xy, hC.xy)
        : vec4(h.zw, hC.zw + 0.5);
}

// ============================================================================
// Technique 1: Polar (standard kaleidoscope)
// ============================================================================
vec3 samplePolar(vec2 uv)
{
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

    // Radial twist (inner rotates more)
    angle += twistAngle * (1.0 - radius);

    // Mirror corners into circle
    if (radius > 0.5) {
        radius = 1.0 - radius;
    }

    // Apply rotation
    angle += rotation;

    // Segment mirroring
    float segmentAngle = TWO_PI / float(segments);
    float a = mod(angle, segmentAngle);
    a = min(a, segmentAngle - a);

    // 4x supersampling for smooth edges
    float offset = 0.002;
    vec3 color = vec3(0.0);
    for (int i = 0; i < 4; i++) {
        float aOff = a + (float(i / 2) - 0.5) * offset;
        float rOff = radius + (float(i % 2) - 0.5) * offset * 0.5;
        vec2 newUV = vec2(cos(aOff), sin(aOff)) * rOff + 0.5;
        color += texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
    }
    return color * 0.25;
}

// ============================================================================
// Technique 2: KIFS (Kaleidoscopic IFS)
// ============================================================================
vec3 sampleKIFS(vec2 uv)
{
    vec2 p = uv * 2.0;

    // Apply twist + rotation
    float r = length(p);
    p = rotate2d(p, twistAngle * (1.0 - r * 0.5) + rotation);

    // Accumulate from multiple depths
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    for (int i = 0; i < kifsIterations; i++) {
        float ri = length(p);
        float a = atan(p.y, p.x);

        // Polar fold into segment
        float foldAngle = TWO_PI / float(segments);
        a = abs(mod(a + foldAngle * 0.5, foldAngle) - foldAngle * 0.5);

        // Back to Cartesian
        p = vec2(cos(a), sin(a)) * ri;

        // IFS contraction
        p = p * kifsScale - kifsOffset;

        // Per-iteration rotation
        p = rotate2d(p, float(i) * 0.3 + time * 0.05);

        // Sample at this depth
        vec2 iterUV = 0.5 + 0.4 * sin(p * 0.15 * TWO_PI * 0.5);
        float w = 1.0 / float(i + 2);
        colorAccum += texture(texture0, iterUV).rgb * w;
        weightAccum += w;
    }

    // Final sample
    vec2 newUV = 0.5 + 0.4 * sin(p * 0.3);
    vec3 finalSample = texture(texture0, fract(newUV)).rgb;
    return (weightAccum > 0.0) ? mix(finalSample, colorAccum / weightAccum, 0.5) : finalSample;
}

// ============================================================================
// Technique 3: Droste (log-polar spiral)
// ============================================================================
vec3 sampleDroste(vec2 uv)
{
    float r = length(uv) + 0.0001;
    float theta = atan(uv.y, uv.x);

    // Work in log-polar space
    float logR = log(r);
    float period = log(drosteScale);

    // Spiral shear: rotating around the image also zooms
    // segments controls number of spiral arms
    logR += float(segments) * theta / TWO_PI * period;

    // Tile in log space (creates infinite zoom repetition)
    // Add large offset to handle negative logR values
    logR = mod(logR + period * 100.0, period);

    // Back to linear radius, in range [1, drosteScale]
    float newR = exp(logR);

    // Normalize to [0, 0.5] for texture sampling
    newR = (newR - 1.0) / (drosteScale - 1.0 + 0.0001) * 0.5;

    // Animation and twist
    theta += rotation + time * 0.1;
    theta += twistAngle * (1.0 - newR * 2.0);

    vec2 newUV = vec2(cos(theta), sin(theta)) * newR + 0.5;

    return texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
}

// ============================================================================
// Technique 4: Iterative Mirror
// ============================================================================
vec3 sampleIterMirror(vec2 uv)
{
    uv = uv * 2.0;  // Scale to -1..1 range
    float a = rotation + time * 0.1;

    for (int i = 0; i < segments; i++) {
        // Rotate
        float c = cos(a), s = sin(a);
        uv = vec2(s * uv.y - c * uv.x, s * uv.x + c * uv.y);

        // Mirror
        uv = mirror(uv);

        // Evolve angle (creates variation per iteration)
        a += float(i + 1);
        a /= float(i + 1);
    }

    // Apply twist based on distance
    float r = length(uv);
    float angle = atan(uv.y, uv.x) + twistAngle * (1.0 - r);
    uv = vec2(cos(angle), sin(angle)) * r;

    return texture(texture0, mirror(uv * 0.5 + 0.5)).rgb;
}

// ============================================================================
// Technique 5: Hex Lattice Fold
// ============================================================================
vec3 sampleHexFold(vec2 uv)
{
    // Apply rotation
    uv = rotate2d(uv, rotation);

    // Apply twist
    float r = length(uv);
    float angle = atan(uv.y, uv.x) + twistAngle * (1.0 - r);
    uv = vec2(cos(angle), sin(angle)) * r;

    // Scale and animate
    vec2 hexUV = uv * hexScale + HEX_S.yx * time * 0.2;

    // Get hex cell coordinates
    vec4 h = getHex(hexUV);

    // h.xy = position within hex cell, h.zw = cell ID
    // Fold within hex cell for symmetry
    vec2 cellUV = h.xy;

    // 6-fold symmetry within hex
    float cellAngle = atan(cellUV.y, cellUV.x);
    float segAngle = TWO_PI / 6.0;
    cellAngle = mod(cellAngle, segAngle);
    cellAngle = min(cellAngle, segAngle - cellAngle);
    float cellR = length(cellUV);
    cellUV = vec2(cos(cellAngle), sin(cellAngle)) * cellR;

    // Map back to texture coords
    vec2 newUV = cellUV / hexScale + 0.5;

    return texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
}

// ============================================================================
// Technique 6: Conformal Power Map
// ============================================================================
vec3 samplePowerMap(vec2 uv)
{
    // Apply rotation first
    uv = rotate2d(uv, rotation);

    // Complex power: z^n in polar form
    float r = length(uv);
    float a = atan(uv.y, uv.x);

    // Apply twist before power
    a += twistAngle * (1.0 - r);

    // Power transform: r^n, angle*n
    float newR = pow(r + 0.001, powerMapN);
    float newAngle = a * powerMapN;

    // Segment mirroring on the result
    if (segments > 1) {
        float segmentAngle = TWO_PI / float(segments);
        newAngle = mod(newAngle, segmentAngle);
        newAngle = min(newAngle, segmentAngle - newAngle);
    }

    vec2 newUV = vec2(cos(newAngle), sin(newAngle)) * newR * 0.5 + 0.5;

    return texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
}

// ============================================================================
// Main
// ============================================================================
void main()
{
    float totalIntensity = polarIntensity + kifsIntensity + drosteIntensity +
                           iterMirrorIntensity + hexFoldIntensity + powerMapIntensity;

    // Bypass when no techniques active
    if (totalIntensity <= 0.0) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    // Center UV coordinates and apply focal offset
    vec2 uv = fragTexCoord - 0.5 - focalOffset;

    // Apply fBM warp to input UV (single calculation for all techniques)
    if (warpStrength > 0.0) {
        vec2 noiseCoord = uv * noiseScale + time * warpSpeed;
        uv += vec2(fbm(noiseCoord), fbm(noiseCoord + vec2(5.2, 1.3))) * warpStrength;
    }

    // Accumulate weighted samples from each active technique
    vec3 color = vec3(0.0);

    if (polarIntensity > 0.0) {
        color += samplePolar(uv) * polarIntensity;
    }
    if (kifsIntensity > 0.0) {
        color += sampleKIFS(uv) * kifsIntensity;
    }
    if (drosteIntensity > 0.0) {
        color += sampleDroste(uv) * drosteIntensity;
    }
    if (iterMirrorIntensity > 0.0) {
        color += sampleIterMirror(uv) * iterMirrorIntensity;
    }
    if (hexFoldIntensity > 0.0) {
        color += sampleHexFold(uv) * hexFoldIntensity;
    }
    if (powerMapIntensity > 0.0) {
        color += samplePowerMap(uv) * powerMapIntensity;
    }

    finalColor = vec4(color / totalIntensity, 1.0);
}
