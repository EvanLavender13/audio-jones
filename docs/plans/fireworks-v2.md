# Fireworks v2

Replace the current fireworks generator's triple-nested-loop shader with analytical ballistic physics from the "Fireworks (atz)" Shadertoy reference. Episode lifecycle (rocket rise, explosion, pause), two burst shape types (circular and random), per-burst FFT reactivity and gradient LUT color. ~20x reduction in per-pixel work.

**Research**: `docs/research/fireworks-v2.md`

## Design

### Types

```c
struct FireworksConfig {
  bool enabled = false;

  // Burst
  int maxBursts = 6;       // Concurrent firework slots (1-8)
  int particles = 40;      // Sparks per burst (10-60)
  float spreadArea = 0.5f; // Horizontal range of launch positions (0.1-1.0)
  float yBias = 0.0f;      // Vertical offset of burst region (-0.5-0.5)

  // Timing
  float rocketTime = 1.1f;  // Rising rocket phase duration (0.3-2.0)
  float explodeTime = 0.9f; // Explosion phase duration (0.3-2.0)
  float pauseTime = 0.5f;   // Gap between episodes per slot (0.0-2.0)

  // Physics
  float gravity = 9.8f;     // Downward acceleration (0.0-20.0)
  float burstSpeed = 10.0f; // Initial outward velocity of sparks (5.0-20.0)
  float rocketSpeed = 8.0f; // Upward velocity of rising rocket (2.0-12.0)

  // Appearance
  float glowIntensity = 1.0f;  // Particle peak brightness (0.1-3.0)
  float particleSize = 0.05f;  // Base glow radius in scaled UV space (0.01-0.1)
  float glowSharpness = 1.9f;  // Glow falloff power (1.0-3.0)
  float sparkleSpeed = 20.0f;  // Sparkle oscillation frequency (5.0-40.0)
  float decayHalfLife = 0.5f;  // Trail persistence in seconds (0.05-2.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT freq Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT freq Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // FFT contrast curve (0.1-3.0)
  float baseBright = 0.15f; // Min brightness floor (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

`FireworksEffect` struct: same shape as current — replace `burstRateLoc`, `burstRadiusLoc`, `dragRateLoc` with `rocketTimeLoc`, `explodeTimeLoc`, `pauseTimeLoc`, `burstSpeedLoc`, `rocketSpeedLoc`, `decayHalfLifeLoc`. Remove `decayFactorLoc` (computed on CPU, passed as `decayFactor` but sourced from `decayHalfLife` now). Actually keep `decayFactorLoc` — the CPU computes `decayFactor = exp(-0.693147 * deltaTime / decayHalfLife)` and sends it. Add `decayHalfLifeLoc` is NOT needed (CPU-only). So the uniform location changes are:

**Remove**: `burstRateLoc`, `burstRadiusLoc`, `dragRateLoc`
**Add**: `rocketTimeLoc`, `explodeTimeLoc`, `pauseTimeLoc`, `burstSpeedLoc`, `rocketSpeedLoc`
**Keep**: `decayFactorLoc` (CPU computes from `decayHalfLife`)

### Algorithm

The shader implements the reference's analytical ballistic firework system. All GLSL below is the complete algorithm the shader task must implement.

#### Hash Function

```glsl
float n21(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}
```

#### Spark Direction Generators

```glsl
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
```

Note: `circularSpark` replaces the reference's hardcoded `10.0` speed with `burstSpeed` uniform. The `float(SPARKS)` divisor becomes `float(particles)`.

#### Rocket Path

```glsl
vec2 rocket(vec2 start, float t, float rocketSpeed) {
    float y = t;
    float x = sin(y * 10.0 + cos(t * 3.0)) * 0.1;
    vec2 p = start + vec2(x, y * rocketSpeed);
    return p;
}
```

Note: `y * 8.0` in reference becomes `y * rocketSpeed`.

#### Per-Burst FFT Sampling

Each burst slot `i` maps to a frequency band. One FFT texture fetch per burst (outside spark loop):

```glsl
// Compute per-burst frequency band (log-spaced across maxBursts)
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
```

#### Per-Burst Gradient LUT Color

Sample once per burst based on the burst's hash:

```glsl
float burstT = fract(noiseId * 3.7);
vec3 lutColor = texture(gradientLUT, vec2(burstT, 0.5)).rgb;
```

#### Color Lifecycle (per-particle, using per-burst color)

```glsl
// White flash at ignition -> LUT color -> ember fade
vec3 pCol = mix(vec3(2.0), lutColor, smoothstep(0.0, 0.15, burstFraction));
pCol = mix(pCol, vec3(0.8, 0.1, 0.0), smoothstep(0.65, 0.9, burstFraction));
```

Where `burstFraction = t / explodeTime` (0→1 over explosion phase).

#### Sparkle

```glsl
float sparkle = sin(time * sparkleSpeed + float(j)) * 0.5 + 0.5;
float sFactor = mix(1.0, sparkle, smoothstep(0.4, 0.9, burstFraction));
```

#### Glow

```glsl
float d = length(uv - p);
float glow = pow(particleSize / (d + 0.002), glowSharpness);
```

#### Main Structure

```glsl
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
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| maxBursts | int | 1-8 | 6 | No | Bursts |
| particles | int | 10-60 | 40 | No | Particles |
| spreadArea | float | 0.1-1.0 | 0.5 | Yes | Spread |
| yBias | float | -0.5-0.5 | 0.0 | Yes | Y Bias |
| rocketTime | float | 0.3-2.0 | 1.1 | Yes | Rocket Time |
| explodeTime | float | 0.3-2.0 | 0.9 | Yes | Explode Time |
| pauseTime | float | 0.0-2.0 | 0.5 | Yes | Pause Time |
| gravity | float | 0.0-20.0 | 9.8 | Yes | Gravity |
| burstSpeed | float | 5.0-20.0 | 10.0 | Yes | Burst Speed |
| rocketSpeed | float | 2.0-12.0 | 8.0 | Yes | Rocket Speed |
| glowIntensity | float | 0.1-3.0 | 1.0 | Yes | Glow Intensity |
| particleSize | float | 0.01-0.1 | 0.05 | Yes | Particle Size |
| glowSharpness | float | 1.0-3.0 | 1.9 | Yes | Sharpness |
| sparkleSpeed | float | 5.0-40.0 | 20.0 | Yes | Sparkle Speed |
| decayHalfLife | float | 0.05-2.0 | 0.5 | Yes | Decay |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |
| blendMode | enum | - | EFFECT_BLEND_SCREEN | No | Blend Mode |

### Constants

- Enum name: `TRANSFORM_FIREWORKS_BLEND` (existing — no change)
- Display name: `"Fireworks"` (existing — no change)
- Category section: 15 (Scatter, existing — no change)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Update FireworksConfig and FireworksEffect structs

**Files**: `src/effects/fireworks.h`
**Creates**: Updated config struct and effect struct that `.cpp` and serialization depend on

**Do**: Replace the current `FireworksConfig` struct with the Design section's struct layout. Update `FireworksEffect` uniform location fields: remove `burstRateLoc`, `burstRadiusLoc`, `dragRateLoc`; add `rocketTimeLoc`, `explodeTimeLoc`, `pauseTimeLoc`, `burstSpeedLoc`, `rocketSpeedLoc`. Keep `decayFactorLoc`. Update `FIREWORKS_CONFIG_FIELDS` macro to match new field names. Function signatures unchanged (same Init/Setup/Render/Resize/Uninit pattern). Keep includes unchanged.

**Verify**: `cmake.exe --build build` compiles (will have linker errors until Wave 2, but header syntax must be valid).

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Rewrite fireworks shader

**Files**: `shaders/fireworks.fs`
**Depends on**: Wave 1 complete (uniform names must match header)

**Do**: Replace the entire shader with the Algorithm section. Key points:
- Keep `#version 330`, `in vec2 fragTexCoord`, `out vec4 finalColor` boilerplate
- Uniforms: `resolution`, `previousFrame`, `time`, `fftTexture`, `sampleRate`, `gradientLUT`, `maxBursts`, `particles`, `spreadArea`, `yBias`, `rocketTime`, `explodeTime`, `pauseTime`, `gravity`, `burstSpeed`, `rocketSpeed`, `glowIntensity`, `particleSize`, `glowSharpness`, `sparkleSpeed`, `decayFactor`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`
- Remove: `burstRate`, `burstRadius`, `dragRate`, `BURST_LIFE`, `MAX_TRAIL`, `Hash22`
- Implement `n21`, `randomSpark`, `circularSpark`, `rocket` functions from Algorithm section
- Main loop: `maxBursts` outer loop with episode lifecycle (rocket/explosion/pause). NO inner `k` trail loop — ballistic formula + ping-pong decay handles trails
- Per-burst FFT sampling with `BAND_SAMPLES = 4` (outside spark loop)
- Per-burst gradient LUT sampling (outside spark loop)
- Per-particle: color lifecycle, sparkle, glow (inside spark loop)
- UV setup: `fragTexCoord - 0.5`, then `uv.x *= resolution.x / resolution.y` (centered coordinates, matching reference)
- Attribution comment at top: `// Based on "Fireworks (atz)" by ilyaev — https://www.shadertoy.com/view/wslcWN`

**Verify**: Shader file parses as valid GLSL 330 (no syntax errors).

#### Task 2.2: Update C++ lifecycle and UI

**Files**: `src/effects/fireworks.cpp`
**Depends on**: Wave 1 complete (config struct field names)

**Do**: Update the C++ module to match new config fields. Pattern: follow existing `fireworks.cpp` structure exactly.

`CacheLocations`: Remove `burstRateLoc`, `burstRadiusLoc`, `dragRateLoc`. Add `rocketTimeLoc` (`"rocketTime"`), `explodeTimeLoc` (`"explodeTime"`), `pauseTimeLoc` (`"pauseTime"`), `burstSpeedLoc` (`"burstSpeed"`), `rocketSpeedLoc` (`"rocketSpeed"`).

`FireworksEffectSetup`: Replace removed uniform binds with new ones. Change decay computation to `decayFactor = expf(-0.693147f * deltaTime / cfg->decayHalfLife)` (was hardcoded `0.14f`).

`FireworksRegisterParams`: Remove `burstRate`, `burstRadius`, `dragRate` registrations. Add: `rocketTime` (0.3-2.0), `explodeTime` (0.3-2.0), `pauseTime` (0.0-2.0), `gravity` (0.0-20.0), `burstSpeed` (5.0-20.0), `rocketSpeed` (2.0-12.0), `decayHalfLife` (0.05-2.0). Update `gravity` range from (0.0-2.0) to (0.0-20.0). Update `particleSize` range from (0.002-0.03) to (0.01-0.1).

`DrawFireworksParams` UI: Reorganize into sections matching the parameter table:
- **Burst**: Bursts (SliderInt 1-8), Particles (SliderInt 10-60), Spread, Y Bias
- **Timing**: Rocket Time, Explode Time, Pause Time
- **Physics**: Gravity, Burst Speed, Rocket Speed
- **Visual**: Glow Intensity, Particle Size, Sharpness, Sparkle Speed, Decay
- **Audio**: Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Base Bright

All other functions (`Init`, `Render`, `Resize`, `Uninit`, bridge functions, registration macro) stay unchanged.

**Verify**: `cmake.exe --build build` compiles and links.

---

## Final Verification

- [x] Build succeeds with no warnings
- [ ] Effect loads without shader compilation errors at runtime
- [ ] Rocket phase shows rising dots with sine wobble
- [ ] Explosion phase shows ballistic sparks falling under gravity
- [ ] Two distinct burst shape types visible (circular rings vs random scatter)
- [ ] FFT reactivity: quiet bands = dim bursts, loud bands = bright
- [ ] Gradient LUT colors applied per-burst
- [ ] All new sliders (Rocket Time, Explode Time, Pause Time, Burst Speed, Rocket Speed) appear and function

---

## Implementation Notes

### Dropped from plan

- **Ping-pong feedback / decay**: The plan called for preserving the ping-pong trail system from the previous implementation. In practice, even tiny per-particle glow values (`0.05/d`) accumulated over frames via `prev * decayFactor` into a screenwide fog. The reference renders fresh each frame with no feedback and looks correct. Removed `previousFrame`, `decayFactor`, `decayHalfLife`, `pingPong[2]`, `readIdx`, and all associated C++ machinery. The effect now uses a single `RenderTexture2D target`.
- **`glowSharpness` / `pow()` glow**: The plan replaced the reference's simple `(0.05 / d)` glow with `pow(particleSize / (d + eps), glowSharpness)`. The power function concentrated brightness into massive blobs near particles instead of the reference's crisp pinpoints. Reverted to the reference's linear inverse distance: `(particleSize / d)`.
- **`sparkleSpeed` / sparkle modulation**: Per-particle `sin(time * sparkleSpeed + j)` caused visible flickering. The reference has no sparkle. Removed.
- **Color lifecycle (white flash → LUT → ember)**: `mix(vec3(2.0), ...)` at ignition doubled brightness. The reference uses flat per-burst color from hash. Replaced with flat LUT color per burst.

### Structural fix

- **`firework()` function**: The plan inlined all burst logic in `main()`. The reference uses a `firework(uv, index, pauseTime)` function where each burst gets its own copy of `uv` scaled independently. Restructured to match.
- **Stagger as dual-purpose pause**: In the reference, the per-burst stagger offset (`i * BASE_PAUSE`) is both subtracted from time AND added to `episodeTime`, naturally desynchronizing bursts. The plan's version used a uniform `pauseTime` for all bursts, causing lockstep transitions. Fixed to match reference pattern.
