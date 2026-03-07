// Fireworks: Analytical ballistic firework system with rocket phase, circular/random bursts,
// and FFT-reactive glow with gradient LUT coloring.
// Based on "Fireworks (atz)" by ilyaev — https://www.shadertoy.com/view/wslcWN
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D previousFrame;
uniform float time;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform int maxBursts;
uniform int particles;
uniform float spreadArea;
uniform float yBias;
uniform float rocketTime;
uniform float explodeTime;
uniform float pauseTime;
uniform float gravity;
uniform float burstSpeed;
uniform float rocketSpeed;
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

#define PI2 6.28318530718

float n21(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

vec2 randomSpark(float noise) {
    vec2 v0 = vec2((noise - 0.5) * 13.0, (fract(noise * 123.0) - 0.5) * 15.0);
    return v0;
}

vec2 circularSpark(float i, float noiseId, float noiseSpark, float time, int particles) {
    noiseId = fract(noiseId * 7897654.45);
    float a = (PI2 / float(particles)) * i;
    float speed = burstSpeed * clamp(noiseId, 0.7, 1.0);
    float x = sin(a + time * ((noiseId - 0.5) * 3.0));
    float y = cos(a + time * (fract(noiseId * 4567.332) - 0.5) * 2.0);
    vec2 v0 = vec2(x, y) * speed;
    return v0;
}

vec2 rocket(vec2 start, float t, float rocketSpeed) {
    float y = t;
    float x = sin(y * 10.0 + cos(t * 3.0)) * 0.1;
    vec2 p = start + vec2(x, y * rocketSpeed);
    return p;
}

void main() {
    vec2 uv = fragTexCoord;
    uv -= 0.5;
    uv.x *= resolution.x / resolution.y;

    vec3 prev = texture(previousFrame, fragTexCoord).rgb;
    vec3 col = prev * decayFactor;

    for (int i = 0; i < maxBursts; i++) {
        float basePause = float(maxBursts) / 30.0;
        float iPause = float(i) * basePause;
        float episodeTime = rocketTime + explodeTime + pauseTime;
        float timeScaled = (time - iPause);
        float id = floor(timeScaled / episodeTime);
        float et = mod(timeScaled, episodeTime);
        float noiseId = n21(vec2(id + 1.0, float(i) + 1.0));

        float scale = clamp(fract(noiseId * 567.53) * 30.0, 10.0, 30.0);
        vec2 scaledUv = uv * scale;
        float ratio = resolution.x / resolution.y;

        // Adjust rocketTime per burst
        float thisRocketTime = rocketTime - (fract(noiseId * 1234.543) * 0.5);

        // Launch position: spreadArea controls horizontal range
        vec2 rocketStart = vec2((noiseId - 0.5) * scale * ratio * spreadArea,
                                -scale * 0.5 + yBias * scale);
        vec2 pRocket = rocket(rocketStart, clamp(et, 0.0, thisRocketTime), rocketSpeed);

        // === FFT sampling (once per burst, outside spark loop) ===
        float t0 = float(i) / float(maxBursts);
        float t1 = float(i + 1) / float(maxBursts);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float burstEnergy = baseBright + mag;

        // === Gradient LUT (once per burst) ===
        float burstT = fract(noiseId * 3.7);
        vec3 lutColor = texture(gradientLUT, vec2(burstT, 0.5)).rgb;

        // Phase 1: Rocket
        if (et < thisRocketTime) {
            float rd = length(scaledUv - pRocket);
            col += pow(0.05 / rd, 1.9) * vec3(0.9, 0.3, 0.0) * burstEnergy;
        }

        // Phase 2: Explosion
        vec2 gravity2d = vec2(0.0, -gravity);
        if (et > thisRocketTime && et < (thisRocketTime + explodeTime)) {
            float burst = sign(fract(noiseId * 44432.22) - 0.6);

            for (int j = 0; j < particles; j++) {
                vec2 center = pRocket;
                float fj = float(j);
                float noiseSpark = fract(n21(vec2(id * 10.0 + float(i) * 20.0, float(j + 1))) * 332.44);
                float t = et - thisRocketTime;
                vec2 v0;

                if (fract(noiseId * 3532.33) > 0.5) {
                    // Random burst
                    v0 = randomSpark(noiseSpark);
                    t -= noiseSpark * (fract(noiseId * 543.0) * 0.2);
                } else {
                    // Circular burst
                    v0 = circularSpark(fj, noiseId, noiseSpark, time, particles);
                    if ((fract(noiseId * 973.22) - 0.5) > 0.0) {
                        float re = mod(fj, 4.0 + 10.0 * noiseId);
                        t -= floor(re / 2.0) * burst * 0.1;
                    } else {
                        t -= mod(fj, 2.0) == 0.0 ? 0.0 : burst * 0.5 * clamp(noiseId, 0.3, 1.0);
                    }
                }

                // Ballistic position
                vec2 s = v0 * t + (gravity2d * t * t) / 2.0;
                vec2 p = center + s;
                float d = length(scaledUv - p);

                if (t > 0.0) {
                    float burstFraction = t / explodeTime;
                    float fade = clamp(1.0 - burstFraction, 0.0, 1.0);

                    // Color lifecycle
                    vec3 pCol = mix(vec3(2.0), lutColor, smoothstep(0.0, 0.15, burstFraction));
                    pCol = mix(pCol, vec3(0.8, 0.1, 0.0), smoothstep(0.65, 0.9, burstFraction));

                    // Glow
                    float glow = pow(particleSize / (d + 0.002), glowSharpness);

                    // Sparkle
                    float sparkle = sin(time * sparkleSpeed + float(j)) * 0.5 + 0.5;
                    float sFactor = mix(1.0, sparkle, smoothstep(0.4, 0.9, burstFraction));

                    col += pCol * glow * fade * sFactor * burstEnergy * glowIntensity;
                }
            }
        }
    }

    finalColor = vec4(col, 1.0);
}
