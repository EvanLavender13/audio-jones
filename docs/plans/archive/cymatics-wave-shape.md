# Cymatics Wave Shape

Add selectable wave shapes (sine, triangle, square, sawtooth) to Ripple Tank's parametric mode and a new parametric mode for Faraday Waves. Both effects currently use `cos()`/`sin()` only — this adds a shared `wave()` function with four periodic shapes.

**Research**: `docs/research/cymatics-wave-shape.md`

## Design

### Types

**RippleTankConfig** — add one field after `waveSource`:

```c
int waveShape = 0;  // Parametric wave shape: 0=sine, 1=triangle, 2=sawtooth, 3=square
```

Add `waveShape` to `RIPPLE_TANK_CONFIG_FIELDS`.

**RippleTankEffect** — add one field:

```c
int waveShapeLoc;
```

**FaradayConfig** — add four fields after `enabled`:

```c
int waveSource = 0;     // 0=audio (FFT-driven), 1=parametric
int waveShape = 0;      // Lattice wave shape: 0=sine, 1=triangle, 2=sawtooth, 3=square
float waveFreq = 30.0f; // Parametric temporal frequency (5.0-100.0)
float waveSpeed = 2.0f; // Parametric animation speed (0.0-10.0)
```

Add `waveSource, waveShape, waveFreq, waveSpeed` to `FARADAY_CONFIG_FIELDS`.

**FaradayEffect** — add three fields:

```c
int waveSourceLoc;
int waveShapeLoc;
int waveFreqLoc;
```

### Algorithm

**Shared GLSL wave function** — define identically in both shaders:

```glsl
const float TAU = 6.283185307;

// Periodic waveform: 0=sine, 1=triangle, 2=sawtooth, 3=square
float wave(float x, int shape) {
    if (shape == 0) return sin(x);
    float p = fract(x / TAU);
    if (shape == 1) return (p < 0.5) ? p * 4.0 - 1.0 : 3.0 - p * 4.0;
    if (shape == 2) return p * 2.0 - 1.0;
    return (p < 0.5) ? 1.0 : -1.0;
}
```

The `sin()` path (shape==0) skips the `fract()` normalization since `sin()` is already periodic.

**Ripple Tank shader substitutions** — in the parametric branches only (`waveSource == 1`):

`waveFromSource` (line 75):

```glsl
// Before:
return sin(dist * waveFreq - time + phase) * atten;
// After:
return wave(dist * waveFreq - time + phase, waveShape) * atten;
```

Chromatic overload (line 90):

```glsl
// Before:
return sin(dist * freq - time + phase) * atten;
// After:
return wave(dist * freq - time + phase, waveShape) * atten;
```

Audio mode branches unchanged — they sample the ring buffer, not a periodic function.

**Faraday shader** — add `latticeWave()` alongside the existing `lattice()`:

```glsl
// Parametric lattice: selectable wave shape replaces cos()
float latticeWave(vec2 p, float k, int N, float rot, int shape) {
    float h = 0.0;
    for (int i = 0; i < N; i++) {
        float theta = float(i) * PI / float(N) + rot;
        vec2 dir = vec2(cos(theta), sin(theta));
        h += wave(dot(dir, p) * k, shape);
    }
    return h / float(N);
}
```

Keep the existing `lattice()` unchanged — audio mode uses `cos()` for physical accuracy.

In `main()`, the layer loop body branches on `waveSource`:

```glsl
for (int i = 0; i < layers; i++) {
    float t0 = (layers > 1) ? float(i) / float(layers - 1) : 0.5;
    float freq = baseFreq * pow(maxFreq / baseFreq, t0);
    float k = freq * spatialScale;

    float mag;
    float layerHeight;

    if (waveSource == 0) {
        // === Audio mode: existing FFT-driven behavior (unchanged) ===
        float t1 = float(i + 1) / float(layers);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freq / nyquist;
        float binHi = freqHi / nyquist;

        float energy = 0.0;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        if (mag < 0.001) continue;

        layerHeight = lattice(uv, k, waveCount, rotationOffset);
        layerHeight *= cos(time * freq * 0.5);
    } else {
        // === Parametric mode: uniform amplitude, selectable wave shape ===
        mag = 1.0;
        layerHeight = latticeWave(uv, k, waveCount, rotationOffset, waveShape);
        layerHeight *= wave(time * waveFreq * 0.5, waveShape);
    }

    totalHeight += mag * layerHeight;
    totalWeight += mag;
}
```

Per-layer `k` still computed from `baseFreq`/`maxFreq`/`spatialScale` in both modes. Only the amplitude source (FFT vs uniform) and wave function (`cos` vs `wave`) change.

**Faraday CPU time accumulation** — branch on `waveSource` in `FaradayEffectSetup`:

```c
if (cfg->waveSource == 0) {
    e->time += deltaTime;                    // Audio: real-time
} else {
    e->time += cfg->waveSpeed * deltaTime;   // Parametric: speed-controlled
}
```

### Parameters

**Ripple Tank** (new field):

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveShape | int  | 0-3   | 0       | No          | Wave Shape |

**Faraday** (new fields):

| Parameter  | Type  | Range     | Default | Modulatable | UI Label   |
|------------|-------|-----------|---------|-------------|------------|
| waveSource | int   | 0-1       | 0       | No          | Wave Source |
| waveShape  | int   | 0-3       | 0       | No          | Wave Shape |
| waveFreq   | float | 5.0-100.0 | 30.0    | Yes         | Wave Freq  |
| waveSpeed  | float | 0.0-10.0  | 2.0     | Yes         | Wave Speed |

### UI Layout

**Ripple Tank** — in the Wave section, add `Wave Shape` combo inside the `waveSource == 1` block, after `Wave Source`:

```
Wave Source: [Audio|Sine]
  Wave Shape:  [Sine|Triangle|Sawtooth|Square]  ← only when Sine
  Wave Freq:   [slider]                         ← only when Sine (existing)
  Wave Speed:  [slider]                         ← only when Sine (existing)
```

**Faraday** — add `Wave Source` combo at the top of the Wave section. Show parametric controls only when `waveSource == 1`:

```
Wave Source:   [Audio|Parametric]
  Wave Shape:  [Sine|Triangle|Sawtooth|Square]  ← only when Parametric
  Wave Freq:   [slider 5.0-100.0, "%.1f"]       ← only when Parametric
  Wave Speed:  [slider 0.0-10.0, "%.1f"]         ← only when Parametric
Wave Count:    [slider 1-6]                      ← always
Spatial Scale: [slider]                          ← always
Vis Gain:      [slider]                          ← always
Rotation:      [slider]                          ← always
Offset:        [slider]                          ← always
```

Audio section (`Base Freq`, `Max Freq`, `Gain`, `Contrast`) remains always visible — `baseFreq`/`maxFreq` still drive per-layer spatial frequencies in parametric mode.

---

## Tasks

### Wave 1: Parallel implementation

#### Task 1.1: Ripple Tank wave shape

**Files**: `src/effects/ripple_tank.h`, `src/effects/ripple_tank.cpp`, `shaders/ripple_tank.fs`

**Do**:

1. **Header** (`ripple_tank.h`):
   - Add `int waveShape = 0;` to `RippleTankConfig` after `waveSource` (with range comment `// Parametric wave shape: 0=sine, 1=triangle, 2=sawtooth, 3=square`)
   - Add `waveShape` to `RIPPLE_TANK_CONFIG_FIELDS` after `waveSource`
   - Add `int waveShapeLoc;` to `RippleTankEffect`

2. **Shader** (`ripple_tank.fs`):
   - Add `uniform int waveShape;` with the other wave uniforms
   - Add the `wave()` function from the Algorithm section (place before `falloff()`, after the uniform declarations)
   - In `waveFromSource` parametric branch (line 75): replace `sin(dist * waveFreq - time + phase)` with `wave(dist * waveFreq - time + phase, waveShape)`
   - In chromatic overload parametric branch (line 90): replace `sin(dist * freq - time + phase)` with `wave(dist * freq - time + phase, waveShape)`
   - Audio branches unchanged

3. **Implementation** (`ripple_tank.cpp`):
   - In `RippleTankEffectInit`: cache `e->waveShapeLoc = GetShaderLocation(e->shader, "waveShape");`
   - In `BindWaveSource` parametric branch: bind `SetShaderValue(e->shader, e->waveShapeLoc, &cfg->waveShape, SHADER_UNIFORM_INT);`
   - `waveShape` is a non-modulatable int — no `ModEngineRegisterParam` call needed
   - **UI** in `DrawRippleTankParams`: inside the `waveSource == 1` block, add `ImGui::Combo("Wave Shape##rt", &e->rippleTank.waveShape, "Sine\0Triangle\0Sawtooth\0Square\0");` before the `Wave Freq` slider

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

#### Task 1.2: Faraday parametric mode

**Files**: `src/effects/faraday.h`, `src/effects/faraday.cpp`, `shaders/faraday.fs`

**Do**:

1. **Header** (`faraday.h`):
   - Add four fields to `FaradayConfig` after `enabled`:
     - `int waveSource = 0;    // 0=audio (FFT-driven), 1=parametric`
     - `int waveShape = 0;     // Lattice wave shape: 0=sine, 1=triangle, 2=sawtooth, 3=square`
     - `float waveFreq = 30.0f; // Parametric temporal frequency (5.0-100.0)`
     - `float waveSpeed = 2.0f;  // Parametric animation speed (0.0-10.0)`
   - Add `waveSource, waveShape, waveFreq, waveSpeed` to `FARADAY_CONFIG_FIELDS`
   - Add three fields to `FaradayEffect`: `int waveSourceLoc;`, `int waveShapeLoc;`, `int waveFreqLoc;`

2. **Shader** (`faraday.fs`):
   - Add uniforms: `uniform int waveSource;`, `uniform int waveShape;`, `uniform float waveFreq;`
   - Add the `wave()` function from the Algorithm section (before `lattice()`)
   - Add the `latticeWave()` function from the Algorithm section (after `lattice()`)
   - Restructure the layer loop in `main()` to branch on `waveSource` as shown in the Algorithm section. Audio mode (0): existing behavior unchanged. Parametric mode (1): `mag = 1.0`, `latticeWave()` for spatial, `wave(time * waveFreq * 0.5, waveShape)` for temporal.

3. **Implementation** (`faraday.cpp`):
   - In `FaradayEffectInit`: cache `waveSourceLoc`, `waveShapeLoc`, `waveFreqLoc`
   - In `FaradayEffectSetup`: change time accumulation to branch on `waveSource` as shown in Algorithm section. Bind new uniforms (`waveSource`, `waveShape`, `waveFreq`).
   - In `FaradayRegisterParams`: register `faraday.waveFreq` (5.0-100.0) and `faraday.waveSpeed` (0.0-10.0)
   - **UI** in `DrawFaradayParams`:
     - At top of Wave section: add `ImGui::Combo("Wave Source##faraday", &e->faraday.waveSource, "Audio\0Parametric\0");`
     - When `waveSource == 1`: show `Wave Shape` combo, `Wave Freq` slider (`ModulatableSlider`, `"%.1f"`), `Wave Speed` slider (`ModulatableSlider`, `"%.1f"`)
     - Keep all other controls (`Wave Count`, `Spatial Scale`, `Vis Gain`, rotation, Audio section, Geometry, Trail) unchanged and always visible

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Ripple Tank: parametric mode shows wave shape combo with 4 options
- [ ] Ripple Tank: selecting triangle/sawtooth/square changes the visible interference pattern
- [ ] Ripple Tank: audio mode unaffected (no wave shape combo visible)
- [ ] Faraday: wave source combo switches between Audio and Parametric
- [ ] Faraday: parametric mode shows wave shape, freq, speed controls
- [ ] Faraday: audio mode unchanged (FFT-driven, no new controls visible)
- [ ] Preset save/load preserves new fields (automatic via CONFIG_FIELDS macros)
