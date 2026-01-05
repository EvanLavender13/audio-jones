#version 330

// Kaleidoscope post-process effect
// Mirrors visual output into radial segments around screen center
// Supports radial twist, moving focal point, and fBM domain warping

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform int segments;         // Mirror segments (1 = disabled)
uniform float rotation;       // Pattern rotation (radians)
uniform float time;           // Animation time (seconds)
uniform float twistAngle;     // Radial twist amount (radians)
uniform vec2 focalOffset;     // Lissajous center offset (UV units)
uniform float warpStrength;   // fBM warp intensity (0 = disabled)
uniform float warpSpeed;      // fBM animation speed
uniform float noiseScale;     // fBM spatial scale
uniform int mode;             // 0=Polar, 1=KIFS
uniform int kifsIterations;   // Folding iterations (1-8)
uniform float kifsScale;      // Per-iteration scale factor
uniform vec2 kifsOffset;      // Translation after fold

out vec4 finalColor;

const float TWO_PI = 6.28318530718;

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

// Sample kaleidoscope at given angle and radius
vec4 sampleKaleido(float angle, float radius, float segmentAngle)
{
    float a = mod(angle, segmentAngle);
    a = min(a, segmentAngle - a);
    vec2 uv = vec2(cos(a), sin(a)) * radius + 0.5;
    return texture(texture0, clamp(uv, 0.0, 1.0));
}

void main()
{
    // Bypass when disabled
    if (segments <= 1) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    // Center UV coordinates and apply focal offset
    vec2 uv = fragTexCoord - 0.5 - focalOffset;

    if (mode == 1) {
        // KIFS: Circular/polar folding for organic fractal patterns
        vec2 p = uv * 2.0;

        // Apply radial twist (inner regions rotate more, same as Polar mode)
        float r = length(p);
        p = rotate2d(p, twistAngle * (1.0 - r * 0.5) + rotation);

        // Accumulate from multiple depths
        vec3 colorAccum = vec3(0.0);
        float weightAccum = 0.0;

        vec2 pIter = p;
        for (int i = 0; i < kifsIterations; i++) {
            // Polar fold: fold based on angle, not x/y axes
            float r = length(pIter);
            float a = atan(pIter.y, pIter.x);

            // Fold angle into segment (creates radial symmetry, not square)
            float foldAngle = TWO_PI / float(segments);
            a = abs(mod(a + foldAngle * 0.5, foldAngle) - foldAngle * 0.5);

            // Back to Cartesian after polar fold
            pIter = vec2(cos(a), sin(a)) * r;

            // Scale toward offset point (IFS contraction)
            pIter = pIter * kifsScale - kifsOffset;

            // Per-iteration rotation
            pIter = rotate2d(pIter, float(i) * 0.3 + time * 0.05);

            // Sample at this depth
            vec2 iterUV = pIter * 0.15 + 0.5;
            iterUV = 0.5 + 0.4 * sin(iterUV * TWO_PI * 0.5);

            float weight = 1.0 / float(i + 2);
            colorAccum += texture(texture0, iterUV).rgb * weight;
            weightAccum += weight;
        }

        // Final sample from deepest iteration
        vec2 newUV = 0.5 + 0.4 * sin(pIter * 0.3);

        // Apply fBM warp if enabled
        if (warpStrength > 0.0) {
            vec2 noiseCoord = newUV * noiseScale + time * warpSpeed;
            float warpX = fbm(noiseCoord);
            float warpY = fbm(noiseCoord + vec2(5.2, 1.3));
            newUV += vec2(warpX, warpY) * warpStrength;
        }

        vec3 finalSample = texture(texture0, fract(newUV)).rgb;
        vec3 blended = (weightAccum > 0.0)
            ? mix(finalSample, colorAccum / weightAccum, 0.5)
            : finalSample;

        finalColor = vec4(blended, 1.0);

    } else {
        // Polar: Standard kaleidoscope transform
        float radius = length(uv);
        float angle = atan(uv.y, uv.x);

        // Apply radial twist (inner regions rotate more)
        angle += twistAngle * (1.0 - radius);

        // Mirror corners back into the circle (keeps circular mandala shape)
        if (radius > 0.5) {
            radius = 1.0 - radius;
        }

        // Apply rotation offset
        angle += rotation;

        // Apply fBM domain warping
        if (warpStrength > 0.0) {
            vec2 polar = vec2(cos(angle), sin(angle)) * radius;
            vec2 noiseCoord = polar * noiseScale + time * warpSpeed;
            float warpX = fbm(noiseCoord);
            float warpY = fbm(noiseCoord + vec2(5.2, 1.3));
            polar += vec2(warpX, warpY) * warpStrength;
            radius = length(polar);
            angle = atan(polar.y, polar.x);
        }

        float segmentAngle = TWO_PI / float(segments);

        // 4x supersampling with angular offsets to smooth segment boundaries
        float offset = 0.002;
        vec4 color = sampleKaleido(angle - offset, radius, segmentAngle)
                   + sampleKaleido(angle + offset, radius, segmentAngle)
                   + sampleKaleido(angle, radius - offset * 0.5, segmentAngle)
                   + sampleKaleido(angle, radius + offset * 0.5, segmentAngle);

        finalColor = color * 0.25;
    }
}
