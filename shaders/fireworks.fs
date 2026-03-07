// Based on "Fireworks (atz)" by ilyaev — https://www.shadertoy.com/view/wslcWN
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
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

vec2 circularSpark(float i, float noiseId, float noiseSpark) {
    noiseId = fract(noiseId * 7897654.45);
    float a = (PI2 / float(particles)) * i;
    float speed = burstSpeed * clamp(noiseId, 0.7, 1.0);
    float x = sin(a + time * ((noiseId - 0.5) * 3.0));
    float y = cos(a + time * (fract(noiseId * 4567.332) - 0.5) * 2.0);
    vec2 v0 = vec2(x, y) * speed;
    return v0;
}

vec2 rocket(vec2 start, float t) {
    float y = t;
    float x = sin(y * 10.0 + cos(t * 3.0)) * 0.1;
    vec2 p = start + vec2(x, y * rocketSpeed);
    return p;
}

vec3 firework(vec2 uv, float index, float iPause) {
    vec3 col = vec3(0.0);

    vec2 grav = vec2(0.0, -gravity);

    float epExplode = explodeTime;
    float epRocket = rocketTime;
    float episodeTime = epRocket + epExplode + iPause;

    float ratio = resolution.x / resolution.y;

    float timeScaled = (time - iPause);

    float id = floor(timeScaled / episodeTime);
    float et = mod(timeScaled, episodeTime);

    float noiseId = n21(vec2(id + 1.0, index + 1.0));

    float scale = clamp(fract(noiseId * 567.53) * 30.0, 10.0, 30.0);
    uv *= scale;

    epRocket -= (fract(noiseId * 1234.543) * 0.5);

    vec2 pRocket = rocket(vec2(0.0 + ((noiseId - 0.5) * scale * ratio * spreadArea), 0.0 - scale / 2.0 + yBias * scale), clamp(et, 0.0, epRocket));

    // FFT energy (once per burst)
    float ft0 = (index - 1.0) / float(maxBursts);
    float ft1 = index / float(maxBursts);
    float freqLo = baseFreq * pow(maxFreq / baseFreq, ft0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, ft1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float burstEnergy = baseBright + mag;

    // LUT color (once per burst)
    float burstT = fract(noiseId * 3.7);
    vec3 lutColor = texture(gradientLUT, vec2(burstT, 0.5)).rgb;

    if (et < epRocket) {
        float rd = length(uv - pRocket);
        col += (particleSize / rd) * vec3(0.9, 0.3, 0.0) * burstEnergy;
    }

    if (et > epRocket && et < (epRocket + epExplode)) {
        float burst = sign(fract(noiseId * 44432.22) - 0.6);
        for (int i = 0; i < particles; i++) {
            vec2 center = pRocket;
            float fi = float(i);
            float noiseSpark = fract(n21(vec2(id * 10.0 + index * 20.0, float(i + 1))) * 332.44);
            float t = et - epRocket;
            vec2 v0;

            if (fract(noiseId * 3532.33) > 0.5) {
                v0 = randomSpark(noiseSpark);
                t -= noiseSpark * (fract(noiseId * 543.0) * 0.2);
            } else {
                v0 = circularSpark(fi, noiseId, noiseSpark);

                if ((fract(noiseId * 973.22) - 0.5) > 0.0) {
                    float re = mod(fi, 4.0 + 10.0 * noiseId);
                    t -= floor(re / 2.0) * burst * 0.1;
                } else {
                    t -= mod(fi, 2.0) == 0.0 ? 0.0 : burst * 0.5 * clamp(noiseId, 0.3, 1.0);
                }
            }

            vec2 sv = v0 * t + (grav * t * t) / 2.0;

            vec2 p = center + sv;

            float d = length(uv - p);

            if (t > 0.0) {
                float fade = clamp((1.0 - t / epExplode), 0.0, 1.0);
                vec3 c = (particleSize / d) * lutColor;
                col += c * fade * burstEnergy * glowIntensity;
            }
        }
    }

    return col;
}

void main() {
    vec2 uv = fragTexCoord;
    uv -= 0.5;
    uv.x *= resolution.x / resolution.y;

    vec3 col = vec3(0.0);

    float basePause = float(maxBursts) / 30.0;

    for (int i = 0; i < maxBursts; i++) {
        col += firework(uv, float(i) + 1.0, float(i) * basePause + pauseTime);
    }

    finalColor = vec4(col, 1.0);
}
