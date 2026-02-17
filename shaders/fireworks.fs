// Fireworks: Radiant particle bursts with per-particle FFT reactivity and ping-pong trail persistence.
// Deterministic trajectories from hash — no CPU-side random state.
// Each burst slot looks back at recent burst IDs so bursts overlap when rate is high.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D previousFrame;
uniform float time;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform float burstRate;
uniform int maxBursts;
uniform int particles;
uniform float spreadArea;
uniform float yBias;
uniform float burstRadius;
uniform float gravity;
uniform float dragRate;
uniform float glowIntensity;
uniform float particleSize;
uniform float glowSharpness;
uniform float sparkleSpeed;
uniform float decayFactor;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float BURST_LIFE = 2.0;  // fixed lifetime per burst in seconds
const int MAX_TRAIL = 5;       // max overlapping bursts per slot

vec2 Hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y;
    float aspect = resolution.x / resolution.y;

    // Decay previous frame then add new particles (additive compositing)
    vec3 prev = texture(previousFrame, fragTexCoord).rgb;
    vec3 col = prev * decayFactor;

    if (burstRate <= 0.0) { finalColor = vec4(col, 1.0); return; }

    for (int i = 0; i < maxBursts; i++) {
        // Phase for this slot — determines when new bursts spawn
        float phase = time * burstRate + float(i) * (1.0 / float(maxBursts));
        float latestId = floor(phase);

        // Look back at recent bursts still within lifetime
        for (int k = 0; k < MAX_TRAIL; k++) {
            float id = latestId - float(k);
            // Age in real seconds since this burst was born
            float age = (phase - id) / burstRate;
            if (age > BURST_LIFE) continue;

            // Lifecycle fraction 0→1 over BURST_LIFE
            float ft = age / BURST_LIFE;

            vec2 range = vec2(aspect * spreadArea, spreadArea * 0.5);
            vec2 burstCenter = (Hash22(vec2(id, float(i))) - 0.5) * 2.0 * range;
            burstCenter.y += yBias;

            for (int j = 0; j < particles; j++) {
                vec2 r = Hash22(vec2(float(j), id));
                float angle = r.x * 6.283185;
                float individualSpeed = burstRadius * (0.2 + 1.2 * pow(r.y, 1.5));

                // Physics in real seconds
                float drag = 1.0 - exp(-age * dragRate);
                vec2 pos = burstCenter + vec2(cos(angle), sin(angle)) * individualSpeed * drag;
                pos.y -= age * age * gravity;

                // Per-particle FFT reactivity
                float freqT = float(j) / float(particles - 1);
                float freq = baseFreq * pow(maxFreq / baseFreq, freqT);
                float bin = freq / (sampleRate * 0.5);
                float mag = texture(fftTexture, vec2(bin, 0.5)).r;
                mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
                float energy = baseBright + mag;

                // Per-particle color from LUT
                vec3 lutColor = texture(gradientLUT, vec2(freqT, 0.5)).rgb;

                // Color lifecycle: white flash -> LUT color -> ember
                vec3 pCol = mix(vec3(2.0), lutColor, smoothstep(0.0, 0.15, ft));
                pCol = mix(pCol, vec3(0.8, 0.1, 0.0), smoothstep(0.65, 0.9, ft));

                // Point glow
                float d = length(uv - pos);
                float size = particleSize * (1.0 - ft * 0.5);
                float glow = size / (d + 0.002);
                glow = pow(glow, glowSharpness);

                // Sparkle: per-particle flicker ramping in over lifetime
                float sparkle = sin(time * sparkleSpeed + float(j)) * 0.5 + 0.5;
                float sFactor = mix(1.0, sparkle, smoothstep(0.4, 0.9, ft));

                col += pCol * glow * (1.0 - ft) * sFactor * energy * glowIntensity * 0.15;
            }
        }
    }

    finalColor = vec4(col, 1.0);
}
