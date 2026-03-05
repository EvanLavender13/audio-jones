#version 330

// Chladni: FFT-driven resonant plate eigenmode visualization
// Generates (n,m) mode pairs mathematically, maps each to its resonant
// frequency bin in the FFT, weights by energy.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // ping-pong read buffer
uniform sampler2D fftTexture;    // FFT magnitudes
uniform sampler2D gradientLUT;   // color mapping
uniform vec2 resolution;
uniform float plateSize;
uniform float coherence;
uniform float visualGain;
uniform float nodalEmphasis;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform int diffusionScale;
uniform float decayFactor;

const float PI = 3.14159265;

// Rectangular plate vibration pattern for mode (n, m)
float chladni(vec2 uv, float n_val, float m_val, float L) {
    float nx = n_val * PI / L;
    float mx = m_val * PI / L;
    return cos(nx * uv.x) * cos(mx * uv.y) - cos(mx * uv.x) * cos(nx * uv.y);
}

void main() {
    float nyquist = sampleRate * 0.5;
    float aspect = resolution.x / resolution.y;
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= aspect;

    // Resonant frequency of mode (n,m) is proportional to n^2 + m^2.
    // Map so that the lowest mode (1,2) corresponds to baseFreq.
    float lowestModeSum = 5.0; // 1^2 + 2^2
    float freqScale = baseFreq / lowestModeSum;

    // Generate all (n,m) pairs where n != m, evaluate if their resonant
    // frequency falls within [baseFreq, maxFreq], weight by FFT energy.
    float totalPattern = 0.0;
    float totalWeight = 0.0;

    // Iterate n and m up to a limit where n^2+m^2 could still be in range
    int maxN = int(sqrt(maxFreq / freqScale)) + 1;
    maxN = min(maxN, 20); // GPU loop bound

    for (int n = 1; n <= maxN; n++) {
        for (int m = 1; m <= maxN; m++) {
            if (n == m) continue; // trivial mode, always zero
            if (n > m) continue;  // avoid duplicate (n,m) and (m,n) — symmetric

            float modeFreq = freqScale * float(n * n + m * m);
            if (modeFreq < baseFreq || modeFreq > maxFreq) continue;

            // Map resonant frequency to FFT bin and sample
            float bin = modeFreq / nyquist;
            if (bin > 1.0) continue;

            float mag = texture(fftTexture, vec2(bin, 0.5)).r;
            mag = pow(clamp(mag * gain, 0.0, 1.0), curve);

            // Coherence sharpens: high coherence suppresses weak modes
            mag = pow(mag, mix(1.0, 4.0, coherence));

            if (mag < 0.001) continue; // skip silent modes entirely

            totalPattern += mag * chladni(uv, float(n), float(m), plateSize);
            totalWeight += mag;
        }
    }

    // Normalize by total weight so brightness doesn't scale with mode count
    if (totalWeight > 0.0) {
        totalPattern /= totalWeight;
    }

    // Coherence: sharpen toward dominant modes by raising weights to a power
    // (applied implicitly — high gain + curve already sharpens FFT response)

    // Nodal emphasis: blend between signed field and absolute value
    float pattern = mix(totalPattern, abs(totalPattern), nodalEmphasis);

    // Color mapping
    float compressed = tanh(pattern * visualGain);
    float t = compressed * 0.5 + 0.5;
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

    // emphasis=0: asymmetric — positive regions bright, negative dark
    // emphasis=1: symmetric — both peaks bright, nodal lines dark
    float signedBright = compressed * 0.5 + 0.5;
    float absBright = abs(compressed);
    float brightness = mix(signedBright, absBright, nodalEmphasis) ;
    vec4 newColor = vec4(color * brightness, brightness);

    // Trail persistence
    vec4 existing;
    if (diffusionScale == 0) {
        existing = texture(texture0, fragTexCoord);
    } else {
        vec2 texel = vec2(float(diffusionScale)) / resolution;

        vec4 h = texture(texture0, fragTexCoord + vec2(-2.0 * texel.x, 0.0)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(-1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2( 1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord + vec2( 2.0 * texel.x, 0.0)) * 0.0625;

        vec4 v = texture(texture0, fragTexCoord + vec2(0.0, -2.0 * texel.y)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(0.0, -1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2(0.0,  1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord + vec2(0.0,  2.0 * texel.y)) * 0.0625;

        existing = (h + v) * 0.5;
    }
    existing *= decayFactor;
    finalColor = existing + newColor * (1.0 - existing);
}
