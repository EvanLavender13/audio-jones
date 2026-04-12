# Film Grain

Photographic noise with luminance-dependent density - thick speckle in shadows, clean highlights - that shimmers temporally like analog film stock. Uses CeeJay's Box-Muller Gaussian noise with an SNR power curve for perceptually accurate grain response, extended with per-channel color noise via seed offsets.

**Research**: `docs/research/film-grain.md`

## Design

### Types

**FilmGrainConfig** (`src/effects/film_grain.h`):

```
bool enabled = false;
float intensity = 0.35f;   // Grain visibility (0.0-1.0)
float variance = 0.4f;     // Gaussian spread (0.0-1.0)
float snr = 6.0f;          // Signal-to-noise ratio power (0.0-16.0)
float colorAmount = 0.0f;  // Mono vs per-channel noise (0.0-1.0)
```

**FilmGrainEffect** (`src/effects/film_grain.h`):

```
Shader shader;
int timeLoc;
int intensityLoc;
int varianceLoc;
int snrLoc;
int colorAmountLoc;
float time;  // Animation accumulator
```

### Algorithm

CeeJay FilmGrain adapted from HLSL to GLSL with per-channel color noise extension.

Substitutions applied to the reference code:

| Reference (HLSL) | Ours (GLSL) |
|-------------------|-------------|
| `tex2D(BackBuffer, texcoord)` | `texture(texture0, fragTexCoord)` |
| `float3` / `float2` | `vec3` / `vec2` |
| `frac` | `fract` |
| `lerp` | `mix` |
| `Timer * 0.0022337` | `time` (uniform, raw seconds accumulated on CPU) |
| `Intensity` | `intensity` (uniform) |
| `Variance` | `variance` (uniform) |
| `SignalToNoiseRatio` (int) | `snr` (float uniform) |
| `SignalToNoiseRatio != 0` | `snr > 0.0` |
| `float(SignalToNoiseRatio)` | `snr` (already float) |
| `Mean` | `0.5` (hardcoded) |

Attribution header (before `#version`):

```glsl
// CeeJay FilmGrain - Box-Muller Gaussian noise with SNR power curve
// https://github.com/killashok/GShade-Shaders/blob/master/Shaders/FilmGrain.fx
// Modified: HLSL to GLSL, per-channel color noise via seed offsets
```

Full shader:

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float intensity;
uniform float variance;
uniform float snr;
uniform float colorAmount;

const float PI = 3.1415927;

// Box-Muller: uniform hash pair -> Gaussian sample centered at 0.5
float boxMuller(float s, float t, float v) {
    float sine = sin(s);
    float cosine = cos(s);
    float u1 = fract(sine * 43758.5453 + t);
    float u2 = fract(cosine * 53758.5453 - t);

    u1 = max(u1, 0.0001);
    float r = sqrt(-log(u1));
    float theta = 2.0 * PI * u2;
    return v * r * cos(theta) + 0.5;
}

void main() {
    vec3 color = texture(texture0, fragTexCoord).rgb;

    // Inverted luminance: 1.0 in shadows, 0.0 in highlights
    float inv_luma = dot(color, vec3(-1.0 / 3.0)) + 1.0;

    // Luminance-dependent variance via SNR power curve
    float stn = (snr > 0.0) ? pow(abs(inv_luma), snr) : 1.0;
    float v = (variance * variance) * stn;

    // Per-pixel seed from UV (hash, not spatial - centering convention does not apply)
    float seed = dot(fragTexCoord, vec2(12.9898, 78.233));

    // 3 independent Gaussian samples for per-channel color noise
    float gaussR = boxMuller(seed, time, v);
    float gaussG = boxMuller(seed + 1.0, time, v);
    float gaussB = boxMuller(seed + 2.0, time, v);

    // Mono vs per-channel
    vec3 gauss = mix(vec3(gaussR), vec3(gaussR, gaussG, gaussB), colorAmount);

    // Multiplicative grain blend
    vec3 grain = mix(vec3(1.0 + intensity), vec3(1.0 - intensity), gauss);
    color *= grain;

    finalColor = vec4(color, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label | Format |
|-----------|------|-------|---------|-------------|----------|--------|
| intensity | float | 0.0-1.0 | 0.35 | yes | Intensity | %.2f |
| variance | float | 0.0-1.0 | 0.4 | yes | Variance | %.2f |
| snr | float | 0.0-16.0 | 6.0 | yes | SNR | %.1f |
| colorAmount | float | 0.0-1.0 | 0.0 | yes | Color Amount | %.2f |

### Constants

- Enum: `TRANSFORM_FILM_GRAIN`
- Display name: `"Film Grain"`
- Badge: `"RET"`
- Section index: `6` (Retro)
- Flags: `EFFECT_FLAG_NONE`
- Config field: `filmGrain`
- Param prefix: `"filmGrain."`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/film_grain.h` (create)
**Creates**: `FilmGrainConfig`, `FilmGrainEffect` types, `FILM_GRAIN_CONFIG_FIELDS` macro, function declarations

**Do**: Create header with types from the Design section. Follow `solarize.h` structure. Setup takes `(FilmGrainEffect*, const FilmGrainConfig*, float deltaTime)` because of time accumulation (same pattern as `wave_ripple.h`).

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create effect module

**Files**: `src/effects/film_grain.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `solarize.cpp` structure.
- `FilmGrainEffectInit`: load `shaders/film_grain.fs`, cache 5 uniform locations (`time`, `intensity`, `variance`, `snr`, `colorAmount`).
- `FilmGrainEffectSetup`: accumulate `e->time += deltaTime`, bind all 5 uniforms.
- `FilmGrainEffectUninit`: unload shader.
- `FilmGrainRegisterParams`: register all 4 params with ranges from Design section. Param IDs: `filmGrain.intensity`, `filmGrain.variance`, `filmGrain.snr`, `filmGrain.colorAmount`.
- Colocated UI: `DrawFilmGrainParams` with 4 `ModulatableSlider` calls. Labels, formats, and param IDs from the Parameters table.
- Bridge: `SetupFilmGrain(PostEffect* pe)` delegates to `FilmGrainEffectSetup` passing `pe->currentDeltaTime`.
- Registration: `REGISTER_EFFECT(TRANSFORM_FILM_GRAIN, FilmGrain, filmGrain, "Film Grain", "RET", 6, EFFECT_FLAG_NONE, SetupFilmGrain, NULL, DrawFilmGrainParams)`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Create shader

**Files**: `shaders/film_grain.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section verbatim. Include attribution header before `#version 330`.

**Verify**: File exists at `shaders/film_grain.fs`.

---

#### Task 2.3: Register in effect config

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/film_grain.h"` with other effect includes.
2. Add `TRANSFORM_FILM_GRAIN,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`.
3. Add `TRANSFORM_FILM_GRAIN` to `TransformOrderConfig::order` initializer.
4. Add `FilmGrainConfig filmGrain;` to `EffectConfig` struct.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Add effect member to PostEffect

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/film_grain.h"` with other effect includes.
2. Add `FilmGrainEffect filmGrain;` to `PostEffect` struct.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Register serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/film_grain.h"`.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FilmGrainConfig, FILM_GRAIN_CONFIG_FIELDS)` with other config macros.
3. Add `X(filmGrain) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: Add to build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/film_grain.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Film Grain appears in Retro section of effect panel with "RET" badge
- [ ] Enabling adds it to the transform pipeline
- [ ] All 4 sliders modify grain in real-time
- [ ] SNR high: grain concentrates in shadows, highlights stay clean
- [ ] SNR low/zero: grain covers everything uniformly
- [ ] Color Amount 0: monochrome grain; 1: per-channel color speckle
- [ ] Grain shimmers temporally (changes each frame)
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to all 4 parameters
