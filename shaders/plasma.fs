// shaders/plasma.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;    // Source texture for additive blend
uniform sampler2D gradientLUT; // 1D gradient: core (0.0) → halo (1.0)
uniform vec2 resolution;

// Phase accumulators (accumulated on CPU)
uniform float animPhase;       // For FBM noise evolution
uniform float driftPhase;      // For bolt horizontal drift
uniform float flickerTime;     // Independent time for flicker (1:1 with real time)

// Parameters
uniform int boltCount;
uniform int layerCount;
uniform int octaves;
uniform int falloffType;
uniform float driftAmount;
uniform float displacement;
uniform float glowRadius;
uniform float coreBrightness;
uniform float flickerAmount;

// Simple hash for randomness
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

// 2D noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n = i.x + i.y * 57.0;
    return mix(mix(hash(n), hash(n + 1.0), f.x),
               mix(hash(n + 57.0), hash(n + 58.0), f.x), f.y);
}

// 2D rotation matrix
mat2 rotate2d(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

// Fractal Brownian Motion
float fbm(vec2 p, int oct) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < oct; i++) {
        value += amplitude * noise(p);
        p = rotate2d(0.45) * p; // Rotate to break grid artifacts
        p *= 2.0;               // Lacunarity
        amplitude *= 0.5;       // Persistence
    }
    return value;
}

// Falloff functions
float applyFalloff(float d, int type) {
    float safeDist = max(d, 0.001);
    if (type == 0) {
        // Sharp: 1/d² - tight bright core
        return 1.0 / (safeDist * safeDist);
    } else if (type == 2) {
        // Soft: 1/√d - wide hazy plasma
        return 1.0 / sqrt(safeDist);
    }
    // Linear (default): 1/d - balanced glow
    return 1.0 / safeDist;
}

void main() {
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= resolution.x / resolution.y; // Aspect correction

    vec3 total = vec3(0.0);

    // Process each depth layer (back to front)
    for (int layer = layerCount - 1; layer >= 0; layer--) {
        float layerScale = 1.0 - float(layer) * 0.3;     // Layers get smaller toward back
        float layerBright = 1.0 - float(layer) * 0.35;   // Layers get dimmer toward back
        float layerSpeed = 1.0 - float(layer) * 0.2;     // Layers animate slower toward back

        for (int i = 0; i < boltCount; i++) {
            // Per-bolt flicker (uses independent flickerTime, not driftPhase)
            float flickerSeed = floor(flickerTime * 30.0) + float(i) * 7.3 + float(layer) * 13.7;
            float flicker = mix(1.0, hash(flickerSeed), flickerAmount);

            // Base X position evenly distributed
            float baseX = (float(i) + 0.5) / float(boltCount) * 2.0 - 1.0;

            // Horizontal drift with golden ratio offset per bolt
            float phase = driftPhase * layerSpeed + float(i) * 1.618 + float(layer) * 0.5;
            float boltX = baseX + sin(phase) * driftAmount;

            // Displace UV with FBM noise (both X and Y per research)
            vec2 displaced = uv * layerScale;
            displaced += (2.0 * fbm(displaced + animPhase * layerSpeed, octaves) - 1.0) * displacement;

            // Distance to vertical line at boltX
            float dist = abs(displaced.x - boltX);

            // Glow intensity (no clamp - let tonemap handle HDR)
            float glow = applyFalloff(dist / glowRadius, falloffType);
            glow *= coreBrightness * layerBright * flicker;

            // Sample gradient by distance: 0.0 = core, 1.0 = halo edge
            // Normalize distance to [0,1] range (dist/glowRadius capped)
            float lutCoord = clamp(dist / (glowRadius * 3.0), 0.0, 1.0);
            vec3 col = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;

            total += col * glow;
        }
    }

    // Tonemap to prevent harsh clipping when bolts overlap
    total = 1.0 - exp(-total);

    // Additive blend with source
    vec3 source = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(source + total, 1.0);
}
