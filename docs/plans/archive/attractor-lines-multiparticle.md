# Attractor Lines Multi-Particle FFT

Enhancement to existing Attractor Lines generator: 12 particle traces on the same attractor (one per chromatic semitone). Active notes make their particle's trail glow bright while quiet semitones fade to near-invisible. The attractor shape reveals itself note-by-note — chords light up more of the structure. Speed-based gradient coloring unchanged; FFT only modulates brightness.

**Research**: `docs/research/attractor-lines-multiparticle.md`

## Design

### Config Changes (AttractorLinesConfig)

New fields added to existing struct:

```
// FFT mapping
int numOctaves = 4;       // Octave count (1-8)
float baseFreq = 55.0f;   // Lowest frequency in Hz (27.5-440.0)
float gain = 3.0f;         // FFT magnitude amplifier (0.1-10.0)
float curve = 1.0f;        // Contrast exponent (0.1-3.0)
float baseBright = 0.05f;  // Minimum brightness for quiet semitones (0.0-1.0)
```

Default `steps` changes from `96.0f` to `48.0f`.

### Effect Struct Changes (AttractorLinesEffect)

New uniform locations:

```
int fftTextureLoc;
int sampleRateLoc;
int baseFreqLoc;
int numOctavesLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
```

### Setup Signature Change

Current: `AttractorLinesEffectSetup(e, cfg, deltaTime, screenWidth, screenHeight)`
New: `AttractorLinesEffectSetup(e, cfg, deltaTime, screenWidth, screenHeight, fftTexture)`

The `fftTexture` (Texture2D) parameter is passed from `shader_setup_generators.cpp` via `pe->fftTexture`.

### Algorithm

The shader changes from single-particle to 12-particle with FFT-gated brightness:

**State persistence — 12 pixels in row 0:**

Each pixel `(pid, 0)` for `pid` in `0..11` stores one particle's 3D position. On reset (first frame or NaN), seed each particle at a different position spread around a circle of radius 1.5:

```glsl
float angle = float(pid) * 6.28318 / 12.0;
float r = 1.5;
pos = vec3(r * cos(angle), r * sin(angle), r * sin(angle * 0.7 + 1.0));
```

The state-write guard changes from `gl_FragCoord.x < 1.0 && gl_FragCoord.y < 1.0` to `gl_FragCoord.y < 1.0 && gl_FragCoord.x < 12.0`. Each state pixel reads its own position, integrates forward by numSteps, and writes the final position back.

**Distance field — nested particle × step loop:**

```glsl
float d = 1e6;
float bestSpeed = 0.0;
int winnerIdx = 0;

for (int pid = 0; pid < 12; pid++) {
    vec3 pos = texelFetch(previousFrame, ivec2(pid, 0), 0).xyz;
    // validate/reset if needed
    vec2 projLast = project(pos, attractorType) + offset;

    for (int i = 0; i < numSteps; i++) {
        vec3 next = rk4Step(pos, dt, attractorType);
        // diverge check, reset if needed
        vec2 projNext = project(next, attractorType) + offset;
        float segD = dfLine(projLast, projNext, uv);
        if (segD < d) {
            d = segD;
            bestSpeed = length(attractorDerivative(next, attractorType));
            winnerIdx = pid;
        }
        pos = next;
        projLast = projNext;
    }
}
```

**FFT-gated brightness:**

For the winning particle, compute its semitone's FFT energy using spectral_arcs pattern:

```glsl
float mag = 0.0;
for (int oct = 0; oct < numOctaves; oct++) {
    float freq = baseFreq * pow(2.0, float(winnerIdx) / 12.0 + float(oct));
    float bin = freq / (sampleRate * 0.5);
    if (bin <= 1.0)
        mag += texture(fftTexture, vec2(bin, 0.5)).r;
}
mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
```

Applied to final color: `color * c * brightness` where `c` is the existing intensity × smoothstep line glow.

**Coloring unchanged**: `speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0)` → `gradientLUT` lookup.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| numOctaves | int | 1-8 | 4 | No | "Octaves" |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | "Base Freq (Hz)" |
| gain | float | 0.1-10.0 | 3.0 | Yes | "Gain" |
| curve | float | 0.1-3.0 | 1.0 | Yes | "Contrast" |
| baseBright | float | 0.0-1.0 | 0.05 | Yes | "Base Bright" |

Existing `steps` default changes from 96.0f → 48.0f.

### New Uniforms

| Uniform | Type | Source |
|---------|------|--------|
| fftTexture | sampler2D | `pe->fftTexture` (texture binding) |
| sampleRate | float | `AUDIO_SAMPLE_RATE` constant |
| baseFreq | float | config field |
| numOctaves | int | config field |
| gain | float | config field |
| curve | float | config field |
| baseBright | float | config field |

---

## Tasks

### Wave 1: Config and C++ Changes

#### Task 1.1: Update config struct and effect module

**Files**: `src/effects/attractor_lines.h` (modify), `src/effects/attractor_lines.cpp` (modify)

**Do**:

In `attractor_lines.h`:
- Add the 5 FFT config fields to `AttractorLinesConfig` (per Design section), after the `maxSpeed` field and before the Transform section
- Change `steps` default from `96.0f` to `48.0f`
- Add 7 uniform location fields to `AttractorLinesEffect` (fftTextureLoc, sampleRateLoc, baseFreqLoc, numOctavesLoc, gainLoc, curveLoc, baseBrightLoc)
- Change `AttractorLinesEffectSetup` signature: add `Texture2D fftTexture` as last parameter
- Add `#include "raylib.h"` if not already present (it is)

In `attractor_lines.cpp`:
- Add `#include "audio/audio.h"` for `AUDIO_SAMPLE_RATE`
- In `CacheLocations()`: cache the 7 new uniform locations
- In `BindScalarUniforms()`: bind `sampleRate` (float from `AUDIO_SAMPLE_RATE`), `baseFreq`, `numOctaves` (cast to int), `gain`, `curve`, `baseBright`
- Change `AttractorLinesEffectSetup` signature to accept `Texture2D fftTexture`, and call `SetShaderValueTexture` for fftTexture (follow spectral_arcs pattern — but note attractor lines uses its own render pass via `AttractorLinesEffectRender`, so the fftTexture binding needs to happen in `Render` not `Setup`). Actually: bind fftTexture in `Setup` is fine since Setup is called before Render and texture bindings are set per-shader. Follow how spectral_arcs binds fftTexture.
- In `AttractorLinesRegisterParams()`: register `baseFreq` (27.5-440), `gain` (0.1-10), `curve` (0.1-3), `baseBright` (0-1). `numOctaves` is int, NOT registered.

**Verify**: `cmake.exe --build build` compiles (will have signature mismatch in shader_setup until Task 1.2).

#### Task 1.2: Update shader setup to pass fftTexture

**Files**: `src/render/shader_setup_generators.cpp` (modify)

**Do**: In `SetupAttractorLines()`, change the call to `AttractorLinesEffectSetup` to pass `pe->fftTexture` as the last argument.

**Verify**: Compiles.

#### Task 1.3: Update preset serialization

**Files**: `src/config/preset.cpp` (modify)

**Do**: Add `numOctaves, baseFreq, gain, curve, baseBright` to the existing `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AttractorLinesConfig, ...)` macro call. Add them after `maxSpeed` and before `x`.

**Verify**: Compiles.

---

### Wave 2: Shader and UI

#### Task 2.1: Update fragment shader for multi-particle FFT

**Files**: `shaders/attractor_lines.fs` (modify)

**Depends on**: Wave 1 complete

**Do**: Modify existing `attractor_lines.fs` to implement the Algorithm section. Key changes:

1. Add new uniforms: `fftTexture` (sampler2D), `sampleRate`, `baseFreq`, `numOctaves` (int), `gain`, `curve`, `baseBright` (all float)

2. Replace `getStartingPoint()` — new version takes `int pid` parameter and returns positions spread around a circle of radius 1.5 (see Algorithm section for formula). The `int type` parameter is no longer needed for starting position since all particles start on the circle.

3. In `main()`: replace the single-particle state read + integration + distance loop with the nested 12-particle loop from the Algorithm section. Each particle reads its state from `texelFetch(previousFrame, ivec2(pid, 0), 0).xyz`, tracks closest segment to `uv`, and records `winnerIdx`.

4. Change state-write guard to `gl_FragCoord.y < 1.0 && gl_FragCoord.x < 12.0`. For each state pixel `px`, read its position from previousFrame, integrate forward numSteps, write back. On first frame (all zeros / NaN), seed using the circle formula with `px` as pid.

5. After distance loop: compute FFT brightness for `winnerIdx` using the spectral_arcs octave-stacking pattern (see Algorithm section). Multiply line intensity `c` by `(baseBright + mag)`.

6. Keep speed-based gradient coloring unchanged.

**Verify**: Shader file has correct uniform declarations and compiles at runtime.

#### Task 2.2: Add Audio UI section

**Files**: `src/ui/imgui_effects_gen_filament.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: In `DrawGeneratorsAttractorLines()`, after the existing "Appearance" section and before `DrawAttractorTransformOutput()`, add an `ImGui::SeparatorText("Audio")` section with the standard FFT slider set following project conventions:

- `ImGui::SliderInt("Octaves##attractorLines", &c->numOctaves, 1, 8)`
- `ModulatableSlider("Base Freq (Hz)##attractorLines", &c->baseFreq, "attractorLines.baseFreq", "%.1f", modSources)`
- `ModulatableSlider("Gain##attractorLines", &c->gain, "attractorLines.gain", "%.1f", modSources)`
- `ModulatableSlider("Contrast##attractorLines", &c->curve, "attractorLines.curve", "%.2f", modSources)`
- `ModulatableSlider("Base Bright##attractorLines", &c->baseBright, "attractorLines.baseBright", "%.2f", modSources)`

**Verify**: Compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Attractor Lines enables and shows configurable particle traces
- [x] Speed-based gradient coloring still works (fast regions different color from slow)
- [x] All 5 attractor types (Lorenz, Rossler, Aizawa, Thomas, Dadras) work with multi-particle
- [x] Attractor type change resets all particle states
- [x] Preset save/load preserves new parameters
- [x] Existing presets with attractor lines load correctly (new fields get defaults)

---

## Implementation Notes

Deviations from original plan during implementation:

### FFT Audio Reactivity Removed
The per-semitone FFT brightness gating was implemented but removed — it was incompatible with the effect. The fftTexture binding was lost after `BeginTextureMode`/`BeginShaderMode` batch flushes in the ping-pong render pass (texture bindings reset, unlike scalar uniforms). Even after fixing the binding, the audio reactivity didn't add value. All FFT fields, uniforms, UI, and preset serialization were stripped.

### Configurable Particle Count Instead of Fixed 12
Hardcoded 12 particles replaced with `numParticles` int slider (1-16, default 8). Lets user dial in quality vs performance.

### Steps Changed to Non-Modulatable Int
`steps` changed from modulatable float (32-256) to non-modulatable int (4-48). Performance cost of 12×steps RK4 integrations per pixel is significant — capping at 48 keeps it usable. Default lowered from 96 to 32.

### Per-Attractor Starting Points
The plan's generic circle (radius 1.5 at origin) caused Aizawa particles to diverge (basin too small) and Thomas particles to cluster in one lobe. Fix: per-attractor base position and spread radius:
- Lorenz/Rossler/Dadras: r=1.5 at their known basin points
- Aizawa: r=0.3 at (0.1, 0, 0) — compact basin
- Thomas: r=2.0 at origin — spreads across all 3 symmetric lobes

### Trail Compositing: max() Instead of Additive
Original `color * c + prev * decayFactor` caused additive saturation — steady-state brightness was `intensity / (1-decayFactor)` which far exceeded 1.0 at any reasonable half-life, making trails appear to never fade. Changed to `max(color * c, prev * decayFactor)` — active lines show at per-frame brightness without accumulation, trails fade exponentially once particles move on.

### Gaussian Glow Scaled by Particle Count
The `exp(-1000*d²)` glow term accumulated into oversaturated blobs with multiple particles. Divided by `numParticles` so total glow stays balanced regardless of particle count.
