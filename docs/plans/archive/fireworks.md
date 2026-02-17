# Fireworks

Radiant particle bursts erupting from random positions, trailing glowing embers that arc under gravity while their comet tails fade through the gradient palette into dying sparks. Per-particle FFT reactivity: each particle in a burst maps to a frequency bin, controlling both its color (gradient LUT) and brightness. Multiple bursts overlap in time at a configurable fixed rate, composited via ping-pong feedback for trail persistence.

**Research**: `docs/research/fireworks.md`

## Design

### Types

**FireworksConfig** (in `fireworks.h`):

```
enabled          bool     false
burstRate        float    1.5       // Bursts per second (0.3-5.0)
maxBursts        int      3         // Concurrent burst slots (1-8)
particles        int      60        // Particles per burst (16-120)
spreadArea       float    0.5       // Spawn distance from center (0.1-1.0)
yBias            float    0.2       // Vertical offset of burst centers (-0.5-0.5)
burstRadius      float    0.6       // Max expansion distance (0.1-1.5)
gravity          float    0.8       // Downward acceleration (0.0-2.0)
dragRate         float    2.0       // Exponential deceleration (0.5-5.0)
glowIntensity    float    1.0       // Particle peak brightness (0.1-3.0)
particleSize     float    0.008     // Base glow radius (0.002-0.03)
glowSharpness    float    1.7       // Glow falloff power (1.0-3.0)
streakLength     float    0.3       // Comet tail length, 0=dots (0.0-1.0)
sparkleSpeed     float    20.0      // Sparkle oscillation freq (5.0-40.0)
decayHalfLife    float    1.0       // Trail persistence seconds (0.1-5.0)
baseFreq         float    55.0      // Lowest FFT freq Hz (27.5-440.0)
maxFreq          float    14000.0   // Highest FFT freq Hz (1000-16000)
gain             float    2.0       // FFT sensitivity (0.1-10.0)
curve            float    1.0       // FFT contrast curve (0.1-3.0)
baseBright       float    0.1       // Min brightness floor (0.0-1.0)
gradient         ColorConfig  {.mode = COLOR_MODE_GRADIENT}
blendMode        EffectBlendMode  EFFECT_BLEND_SCREEN
blendIntensity   float    1.0
```

**FireworksEffect** (in `fireworks.h`):

```
shader              Shader
gradientLUT         ColorLUT*
pingPong[2]         RenderTexture2D
readIdx             int
time                float           // Master time accumulator
// Uniform locations (one int per uniform)
resolutionLoc, previousFrameLoc, timeLoc, fftTextureLoc, sampleRateLoc,
burstRateLoc, maxBurstsLoc, particlesLoc, spreadAreaLoc, yBiasLoc,
burstRadiusLoc, gravityLoc, dragRateLoc, glowIntensityLoc, particleSizeLoc,
glowSharpnessLoc, streakLengthLoc, sparkleSpeedLoc, decayFactorLoc,
baseFreqLoc, maxFreqLoc, gainLoc, curveLoc, baseBrightLoc, gradientLUTLoc
```

### Algorithm

The shader runs a double loop: outer loop over `maxBursts` burst slots, inner loop over `particles` per burst. All particle trajectories are deterministic from `Hash22(burstId, particleIndex)` — no CPU-side random state.

**Hash function** (verbatim from reference):

```glsl
vec2 Hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}
```

**Burst lifecycle** — each slot `i`:

```glsl
float t = time * burstRate + i * (1.0 / maxBursts);  // stagger slots evenly
float id = floor(t);
float ft = fract(t);  // 0→1 lifetime fraction
```

Only draw when `ft < 0.9` (dead zone before next burst).

**Burst center** — deterministic from burst ID:

```glsl
vec2 range = vec2(aspect * spreadArea, spreadArea * 0.5);
vec2 center = (Hash22(vec2(id, float(i))) - 0.5) * 2.0 * range;
center.y += yBias;
```

**Particle physics** — for each particle `j` in burst:

```glsl
vec2 r = Hash22(vec2(j, id));
float angle = r.x * 6.283185;
float individualSpeed = burstRadius * (0.2 + 1.2 * pow(r.y, 1.5));
float drag = 1.0 - exp(-ft * dragRate);
vec2 pos = burstCenter + vec2(cos(angle), sin(angle)) * individualSpeed * drag;
pos.y -= ft * ft * gravity;
```

**Velocity** (analytic derivative of position, for streak direction):

```glsl
vec2 dir = vec2(cos(angle), sin(angle));
float velMag = individualSpeed * dragRate * exp(-ft * dragRate);
vec2 vel = dir * velMag + vec2(0.0, -2.0 * ft * gravity);
```

**Per-particle FFT reactivity** — particle index `j` maps to a frequency bin:

```glsl
float freqT = float(j) / float(particles - 1);  // 0→1 across particles
float freq = baseFreq * pow(maxFreq / baseFreq, freqT);  // log-spaced
float bin = freq / (sampleRate * 0.5);  // normalized texture coordinate
float mag = texture(fftTexture, vec2(bin, 0.5)).r;
mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
float energy = baseBright + mag;
```

**Per-particle color** — LUT sampled by frequency position:

```glsl
vec3 lutColor = texture(gradientLUT, vec2(freqT, 0.5)).rgb;
```

**Color lifecycle** — white flash → LUT color → ember:

```glsl
vec3 col = mix(vec3(2.0), lutColor, smoothstep(0.0, 0.15, ft));
col = mix(col, vec3(0.8, 0.1, 0.0), smoothstep(0.65, 0.9, ft));
```

**Glow rendering** — point or streak:

```glsl
float d;
if (streakLength > 0.0) {
    // Line segment SDF: pos to posTail
    vec2 tailDir = normalize(vel);
    vec2 posTail = pos - tailDir * streakLength * length(vel);
    vec2 pa = uv - posTail, ba = pos - posTail;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    d = length(pa - ba * h);
} else {
    d = length(uv - pos);
}

float size = particleSize * (1.0 - ft * 0.5);
float glow = size / (d + 0.002);
glow = pow(glow, glowSharpness);
```

**Sparkle** — per-particle flicker ramping in over lifetime:

```glsl
float sparkle = sin(time * sparkleSpeed + j) * 0.5 + 0.5;
float sFactor = mix(1.0, sparkle, smoothstep(0.4, 0.9, ft));
```

**Final accumulation**:

```glsl
accumulated += col * glow * (1.0 - ft) * sFactor * energy * glowIntensity * 0.15;
```

**Ping-pong compositing** — in `main()`:

```glsl
vec3 prev = texture(previousFrame, fragTexCoord).rgb;
vec3 result = max(accumulated, prev * decayFactor);
finalColor = vec4(result, 1.0);
```

`decayFactor` is computed on CPU: `exp(-0.693147 * deltaTime / decayHalfLife)`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| burstRate | float | 0.3-5.0 | 1.5 | yes | Burst Rate |
| maxBursts | int | 1-8 | 3 | no | Bursts |
| particles | int | 16-120 | 60 | no | Particles |
| spreadArea | float | 0.1-1.0 | 0.5 | yes | Spread |
| yBias | float | -0.5-0.5 | 0.2 | yes | Y Bias |
| burstRadius | float | 0.1-1.5 | 0.6 | yes | Burst Radius |
| gravity | float | 0.0-2.0 | 0.8 | yes | Gravity |
| dragRate | float | 0.5-5.0 | 2.0 | yes | Drag |
| glowIntensity | float | 0.1-3.0 | 1.0 | yes | Glow Intensity |
| particleSize | float | 0.002-0.03 | 0.008 | yes | Particle Size |
| glowSharpness | float | 1.0-3.0 | 1.7 | yes | Sharpness |
| streakLength | float | 0.0-1.0 | 0.3 | yes | Streak Length |
| sparkleSpeed | float | 5.0-40.0 | 20.0 | yes | Sparkle Speed |
| decayHalfLife | float | 0.1-5.0 | 1.0 | yes | Decay |
| baseFreq | float | 27.5-440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.0 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.1 | yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_FIREWORKS_BLEND`
- Display name: `"Fireworks"`
- Registration: `REGISTER_GENERATOR_FULL` (GEN badge, section 10, BLEND + NEEDS_RESIZE flags)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create fireworks.h

**Files**: `src/effects/fireworks.h` (create)
**Creates**: `FireworksConfig` and `FireworksEffect` types that all other tasks depend on

**Do**: Create the effect header following the Attractor Lines pattern (`src/effects/attractor_lines.h`). Define `FireworksConfig` with all fields from the Design section, `FIREWORKS_CONFIG_FIELDS` macro, and `FireworksEffect` struct with shader handle, `ColorLUT*`, `RenderTexture2D pingPong[2]`, `int readIdx`, `float time`, and all uniform location ints. Declare the standard lifecycle functions: `FireworksEffectInit(FireworksEffect*, const FireworksConfig*, int w, int h)`, `FireworksEffectSetup(...)`, `FireworksEffectRender(...)`, `FireworksEffectResize(...)`, `FireworksEffectUninit(...)`, `FireworksConfigDefault()`, `FireworksRegisterParams()`. Include `"render/color_config.h"`, `"render/blend_mode.h"`, `"raylib.h"`, `<stdbool.h>`.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker impact yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create fireworks.cpp

**Files**: `src/effects/fireworks.cpp` (create)
**Depends on**: Wave 1

**Do**: Implement the effect module following Attractor Lines (`src/effects/attractor_lines.cpp`) as the structural pattern. Key functions:

- `FireworksEffectInit`: Load shader from `shaders/fireworks.fs`, cache all uniform locations, init `ColorLUT`, init ping-pong via `RenderUtilsInitTextureHDR`, set `readIdx=0`, `time=0`. Cascade cleanup on failure.
- `FireworksEffectSetup`: Accumulate `time += deltaTime`, compute `decayFactor = exp(-0.693147 * deltaTime / max(decayHalfLife, 0.001))` on CPU, bind all scalar uniforms. Update ColorLUT.
- `FireworksEffectRender`: Ping-pong pass — write to `pingPong[1-readIdx]`, read from `pingPong[readIdx]`, bind `previousFrame` and `gradientLUT` textures AFTER `BeginTextureMode`/`BeginShaderMode` (raylib flush requirement), draw fullscreen quad, swap `readIdx`.
- `FireworksEffectResize`: Unload + reinit ping-pong, reset `readIdx`.
- `FireworksEffectUninit`: Unload shader, LUT, ping-pong.
- `FireworksRegisterParams`: Register all modulatable float params (see Parameters table).
- Bridge functions: `SetupFireworks(PostEffect*)` and `SetupFireworksBlend(PostEffect*)` — same pattern as Attractor Lines.
- Registration: `REGISTER_GENERATOR_FULL(TRANSFORM_FIREWORKS_BLEND, Fireworks, fireworks, "Fireworks", SetupFireworksBlend, SetupFireworks)`.

Setup signature: `(FireworksEffect*, const FireworksConfig*, float deltaTime, int screenWidth, int screenHeight, Texture2D fftTexture)` — needs resolution for aspect ratio uniform and fftTexture for binding.

Includes: own header, `"audio/audio.h"` (for `AUDIO_SAMPLE_RATE`), `"automation/modulation_engine.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"render/render_utils.h"`, `<math.h>`, `<stddef.h>`.

**Verify**: Compiles with Wave 2.3 (CMakeLists).

#### Task 2.2: Create fireworks.fs

**Files**: `shaders/fireworks.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the fragment shader. This is the core visual — implement the full Algorithm section from the Design above. Key points:

- Uniforms: `resolution`, `previousFrame`, `time`, `fftTexture`, `sampleRate`, `gradientLUT`, all config params.
- Coordinate system: `vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y` (centered, aspect-correct — matches reference code).
- Double loop: outer `maxBursts`, inner `particles`.
- Per-particle FFT: map particle index to log-spaced frequency, sample `fftTexture`, apply gain/curve/baseBright.
- Per-particle color: sample `gradientLUT` at `freqT` position.
- Color lifecycle: white flash → LUT → ember (smoothstep transitions from Algorithm section).
- Glow: point distance or line-segment SDF based on `streakLength`.
- Sparkle: sinusoidal flicker.
- Ping-pong: read `previousFrame`, composite with `max(new, prev * decayFactor)`.
- `maxBursts` and `particles` are `int` uniforms controlling loop bounds.

**Verify**: Shader is runtime-compiled — no build step. Visual verification at runtime.

#### Task 2.3: Config registration and build

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**:

1. **effect_config.h**: Add `#include "effects/fireworks.h"`. Add `TRANSFORM_FIREWORKS_BLEND` to `TransformEffectType` enum (before `TRANSFORM_EFFECT_COUNT`). Add to `TransformOrderConfig::order` array. Add `FireworksConfig fireworks;` to `EffectConfig`.
2. **post_effect.h**: Add `#include "effects/fireworks.h"`. Add `FireworksEffect fireworks;` member in `PostEffect` struct (near `AttractorLinesEffect attractorLines`).
3. **CMakeLists.txt**: Add `src/effects/fireworks.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add a special-case branch for `TRANSFORM_FIREWORKS_BLEND` in the transform loop, directly after the `TRANSFORM_ATTRACTOR_LINES_BLEND` case (line ~320). Same pattern:

```cpp
} else if (effectType == TRANSFORM_FIREWORKS_BLEND) {
    EFFECT_DESCRIPTORS[effectType].scratchSetup(pe);
    FireworksEffectRender(
        &pe->fireworks, &pe->effects.fireworks,
        pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
    RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader,
               entry.setup);
}
```

Add `#include "effects/fireworks.h"` at the top.

**Verify**: Compiles.

#### Task 2.5: UI panel

**Files**: `src/ui/imgui_effects_gen_atmosphere.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add Fireworks section to the atmosphere generator UI, following the Nebula pattern in the same file. Add `#include "effects/fireworks.h"` at top. Add `static bool sectionFireworks = false;`.

Create `DrawGeneratorsFireworks(EffectConfig*, const ModSources*, ImU32)`:

- `DrawSectionBegin("Fireworks", ...)` with `e->fireworks.enabled`
- Enable checkbox with `MoveTransformToEnd` on fresh enable using `TRANSFORM_FIREWORKS_BLEND`
- Sections with `ImGui::SeparatorText`:
  - **Burst**: Burst Rate, Bursts (`SliderInt`), Particles (`SliderInt`), Spread, Y Bias
  - **Physics**: Burst Radius, Gravity, Drag
  - **Visual**: Glow Intensity, Particle Size, Sharpness, Streak Length, Sparkle Speed, Decay
  - **Audio**: Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Base Bright — standard audio section pattern
  - **Output**: Brightness not needed (glowIntensity covers it), gradient color mode, Blend Intensity, Blend Mode combo
- All float params use `ModulatableSlider` with `"fireworks.<field>"` ID. Audio section follows standard conventions (see `docs/conventions.md`).

Call `DrawGeneratorsFireworks` from `DrawGeneratorsAtmosphere` with `ImGui::Spacing()` before it, between Nebula and Solid Color.

**Verify**: Compiles.

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/fireworks.h"`. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FireworksConfig, FIREWORKS_CONFIG_FIELDS)`. Add `X(fireworks) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Compiles.

---

## Future Work

The ping-pong render pipeline special-casing (Attractor Lines and now Fireworks) should be generalized. See `docs/research/ping-pong-generalization.md` for the refactor plan.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Fireworks appears in transform order pipeline
- [ ] Effect shows "GEN" category badge
- [ ] Enabling effect adds it to the pipeline list
- [ ] UI controls show all 5 sections (Burst, Physics, Visual, Audio, Output)
- [ ] Particles react per-frequency to audio input
- [ ] Streak rendering activates when streakLength > 0
- [ ] Ping-pong trails persist and decay
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to registered parameters
