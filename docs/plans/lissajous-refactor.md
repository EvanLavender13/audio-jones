# Lissajous Refactor

Unify all lissajous motion under `DualLissajousConfig`. The config owns phase accumulation internally. Callers pass `deltaTime` and get back position offsets. Only `amplitude` and `motionSpeed` are modulatable (frequency changes cause discontinuities).

## Specification

### Updated DualLissajousConfig

```cpp
struct DualLissajousConfig {
  // Modulatable params
  float amplitude = 0.2f;   // Motion amplitude (0.0-0.5)
  float motionSpeed = 1.0f; // Phase accumulation rate (0.0-5.0)

  // Shape params (not modulatable - cause discontinuities)
  float freqX1 = 0.05f;   // Primary X frequency (Hz)
  float freqY1 = 0.08f;   // Primary Y frequency (Hz)
  float freqX2 = 0.0f;    // Secondary X frequency (Hz, 0 = disabled)
  float freqY2 = 0.0f;    // Secondary Y frequency (Hz, 0 = disabled)
  float offsetX2 = 0.3f;  // Phase offset for secondary X (radians)
  float offsetY2 = 3.48f; // Phase offset for secondary Y (radians)

  // Internal state (not serialized)
  float phase = 0.0f; // Accumulated phase
};
```

### Updated Function

```cpp
// Accumulates phase internally, then computes offset
// deltaTime: frame time in seconds
// perSourceOffset: additional phase offset for this source (e.g., i/count * TWO_PI)
// outX, outY: offset values to add to base position
inline void DualLissajousUpdate(DualLissajousConfig *cfg, float deltaTime,
                                float perSourceOffset, float *outX, float *outY) {
  cfg->phase += deltaTime * cfg->motionSpeed;

  const float phaseX1 = cfg->phase * cfg->freqX1 + perSourceOffset;
  const float phaseY1 = cfg->phase * cfg->freqY1 + perSourceOffset;

  float x = sinf(phaseX1);
  float y = cosf(phaseY1);

  if (cfg->freqX2 > 0.0f) {
    const float phaseX2 = cfg->phase * cfg->freqX2 + cfg->offsetX2 + perSourceOffset;
    x += sinf(phaseX2);
  }
  if (cfg->freqY2 > 0.0f) {
    const float phaseY2 = cfg->phase * cfg->freqY2 + cfg->offsetY2 + perSourceOffset;
    y += cosf(phaseY2);
  }

  const float scaleX = (cfg->freqX2 > 0.0f) ? 0.5f : 1.0f;
  const float scaleY = (cfg->freqY2 > 0.0f) ? 0.5f : 1.0f;

  *outX = cfg->amplitude * x * scaleX;
  *outY = cfg->amplitude * y * scaleY;
}
```

### Modulatable Parameters

| Parameter | Range | Default |
|-----------|-------|---------|
| `*.lissajous.amplitude` | 0.0-0.5 | 0.2 |
| `*.lissajous.motionSpeed` | 0.0-5.0 | 1.0 |

All frequency and offset params removed from registry.

---

## Tasks

### Wave 1: Config Headers

All config updates. No file overlap.

#### Task 1.1: Update DualLissajousConfig

**Files**: `src/config/dual_lissajous_config.h`

**Build**:
1. Add `float motionSpeed = 1.0f;` after amplitude
2. Add `float phase = 0.0f;` at end (internal state)
3. Rename `DualLissajousCompute` to `DualLissajousUpdate`
4. Add `cfg->phase += deltaTime * cfg->motionSpeed;` at start of function
5. Change signature: `(DualLissajousConfig *cfg, float deltaTime, float perSourceOffset, float *outX, float *outY)`
6. Remove `const` from cfg parameter (it mutates phase now)

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Update WaveRipple Config

**Files**: `src/config/wave_ripple_config.h`

**Build**:
1. Add include: `#include "config/dual_lissajous_config.h"`
2. Remove: `originAmplitude`, `originFreqX`, `originFreqY`
3. Add: `DualLissajousConfig originLissajous;`
4. Set defaults: `originLissajous.amplitude = 0.0f` (disabled by default), `originLissajous.freqX1 = 1.0f`, `originLissajous.freqY1 = 1.0f`

**Verify**: Compiles.

#### Task 1.3: Update Mobius Config

**Files**: `src/config/mobius_config.h`

**Build**:
1. Add include: `#include "config/dual_lissajous_config.h"`
2. Remove: `pointAmplitude`, `pointFreq1`, `pointFreq2`
3. Add: `DualLissajousConfig point1Lissajous;` and `DualLissajousConfig point2Lissajous;`
4. Set defaults: both with `amplitude = 0.0f` (disabled), `freqX1 = freqY1 = 1.0f` for point1, `1.3f` for point2

**Verify**: Compiles.

#### Task 1.4: Update ParametricTrail Config

**Files**: `src/config/drawable_config.h`

**Build**:
1. Add include at top: `#include "config/dual_lissajous_config.h"`
2. In ParametricTrailData:
   - Remove: `speed`, `amplitude`, `freqX1`, `freqY1`, `freqX2`, `freqY2`, `offsetX`, `offsetY`, `phase`
   - Add: `DualLissajousConfig lissajous;`
   - Set defaults:
     - `lissajous.motionSpeed = 1.0f`
     - `lissajous.amplitude = 0.25f`
     - `lissajous.freqX1 = 3.14159f`
     - `lissajous.freqY1 = 1.0f`
     - `lissajous.freqX2 = 0.72834f`
     - `lissajous.freqY2 = 2.781374f`
     - `lissajous.offsetX2 = 0.3f`
     - `lissajous.offsetY2 = 3.47912f`

**Verify**: Compiles.

---

### Wave 2: Simulation Updates

Isolated simulation files. No file overlap.

#### Task 2.1: Update Cymatics

**Files**: `src/simulation/cymatics.cpp`, `src/simulation/cymatics.h`

**Build**:
1. In `cymatics.h`: Remove `float sourcePhase;` from Cymatics struct
2. In `cymatics.cpp` `CymaticsUpdate()`:
   - Remove `c->sourcePhase += deltaTime;`
   - For source loop: first source gets deltaTime, rest get 0
   - Change `DualLissajousCompute(&cfg.lissajous, c->sourcePhase, ...)` to `DualLissajousUpdate(&c->config.lissajous, dt, perSourceOffset, ...)`
   - where dt = (i == 0) ? deltaTime : 0.0f

**Verify**: Compiles.

#### Task 2.2: Update Physarum

**Files**: `src/simulation/physarum.h`, `src/simulation/physarum.cpp`

**Build**:
1. In `physarum.h` PhysarumConfig:
   - Remove: `lissajousAmplitude`, `lissajousFreqX`, `lissajousFreqY`
   - Add: `DualLissajousConfig lissajous;`
   - Keep: `lissajousBaseRadius` (rename to `attractorBaseRadius`)
   - Add include: `#include "config/dual_lissajous_config.h"`
2. In `physarum.h` Physarum struct:
   - Remove: `float lissajousPhase;`
3. In `physarum.cpp` `PhysarumInit()`:
   - Remove: `p->lissajousPhase = 0.0f;`
4. In `physarum.cpp` `PhysarumUpdate()`:
   - Remove: `p->lissajousPhase += deltaTime * TWO_PI;`
   - Remove inline lissajous calculation
   - For each attractor: call `DualLissajousUpdate(&p->config.lissajous, dt, perSourceOffset, &offsetX, &offsetY)` where dt = (i==0) ? deltaTime : 0
   - Compute attractor position: `baseX + offsetX`, `baseY + offsetY`
5. Set defaults in lissajous config to match old behavior:
   - `amplitude = 0.1f` (was lissajousAmplitude)
   - `freqX1 = 0.05f`, `freqY1 = 0.08f` (were lissajousFreqX/Y)
   - `motionSpeed = 6.28318f` (was multiplied by TWO_PI)

**Verify**: Compiles.

---

### Wave 3: Render Layer Updates

Single task for render_pipeline.cpp (no conflicts). Parallel task for other render files.

#### Task 3.1: Update render_pipeline.cpp

**Files**: `src/render/render_pipeline.cpp`

**Build**:
1. **Interference**: Remove `pe->interferenceSourcePhase += deltaTime;` line (phase now in lissajous config)
2. **WaveRipple**: Remove inline wave ripple lissajous calculation (lines using `wr->originAmplitude * sinf(...)`), replace with:
   ```cpp
   float wrOffsetX, wrOffsetY;
   DualLissajousUpdate(&pe->effects.waveRipple.originLissajous, deltaTime, 0.0f, &wrOffsetX, &wrOffsetY);
   pe->currentWaveRippleOrigin[0] = wr->originX + wrOffsetX;
   pe->currentWaveRippleOrigin[1] = wr->originY + wrOffsetY;
   ```
3. **Mobius**: Remove inline mobius lissajous calculation, replace with:
   ```cpp
   float m1OffsetX, m1OffsetY;
   DualLissajousUpdate(&pe->effects.mobius.point1Lissajous, deltaTime, 0.0f, &m1OffsetX, &m1OffsetY);
   pe->currentMobiusPoint1[0] = m->point1X + m1OffsetX;
   pe->currentMobiusPoint1[1] = m->point1Y + m1OffsetY;

   float m2OffsetX, m2OffsetY;
   DualLissajousUpdate(&pe->effects.mobius.point2Lissajous, deltaTime, 0.0f, &m2OffsetX, &m2OffsetY);
   pe->currentMobiusPoint2[0] = m->point2X + m2OffsetX;
   pe->currentMobiusPoint2[1] = m->point2Y + m2OffsetY;
   ```

**Verify**: Compiles.

#### Task 3.2: Update shader_setup_generators.cpp

**Files**: `src/render/shader_setup_generators.cpp`

**Build**:
1. In `SetupInterference()`:
   - Remove `pe->interferenceSourcePhase` usage
   - Get deltaTime from `GetFrameTime()` for first source, 0 for rest
   - Change `DualLissajousCompute(...)` to `DualLissajousUpdate(&cfg.lissajous, dt, perSourceOffset, &offsetX, &offsetY)`
   - where dt = (i == 0) ? GetFrameTime() : 0.0f

**Verify**: Compiles.

#### Task 3.3: Update drawable.cpp

**Files**: `src/render/drawable.cpp`

**Build**:
1. In `DrawableRenderParametricTrail()`:
   - Get deltaTime from `GetFrameTime()`
   - Replace inline calculation with: `DualLissajousUpdate(&trail.lissajous, deltaTime, 0.0f, &offsetX, &offsetY)`
   - Compute position: `x = d->base.x + offsetX`, `y = d->base.y + offsetY`
   - Replace `fmodf(trail.phase, 1.0f)` with `fmodf(trail.lissajous.phase, 1.0f)`
2. In `DrawableTickRotations()`:
   - Remove the DRAWABLE_PARAMETRIC_TRAIL case that accumulates phase (now done in render)

**Verify**: Compiles.

#### Task 3.4: Update post_effect.h/cpp (cleanup)

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`

**Build**:
1. Remove `float interferenceSourcePhase` from PostEffect struct
2. Remove initialization in PostEffectInit

**Verify**: Compiles.

---

### Wave 4: Param Registry and Automation

Single task for param_registry.cpp. Parallel task for drawable_params.cpp.

#### Task 4.1: Update Param Registry

**Files**: `src/automation/param_registry.cpp`

**Build**:
1. Remove all entries for:
   - `interference.lissajous.freqX1/X2/Y1/Y2`
   - `interference.lissajous.offsetX2/Y2`
   - `physarum.lissajousAmplitude`
   - `physarum.lissajousFreqX`
   - `physarum.lissajousFreqY`
   - `physarum.lissajousBaseRadius`
2. Add entries for:
   - `interference.lissajous.amplitude` {0.0f, 0.5f}
   - `interference.lissajous.motionSpeed` {0.0f, 5.0f}
   - `cymatics.lissajous.amplitude` {0.0f, 0.5f}
   - `cymatics.lissajous.motionSpeed` {0.0f, 5.0f}
   - `physarum.lissajous.amplitude` {0.0f, 0.5f}
   - `physarum.lissajous.motionSpeed` {0.0f, 10.0f}

**Verify**: Compiles.

#### Task 4.2: Update Drawable Params

**Files**: `src/automation/drawable_params.cpp`

**Build**:
1. Remove registrations for: `speed`, `amplitude`, `freqX1`, `freqY1`, `freqX2`, `freqY2`
2. Add registrations for: `lissajous.amplitude` (0.05-0.5), `lissajous.motionSpeed` (0.1-10.0)

**Verify**: Compiles.

---

### Wave 5: UI Updates

Three parallel tasks for different UI files.

#### Task 5.1: Update imgui_effects_generators.cpp

**Files**: `src/ui/imgui_effects_generators.cpp`

**Build**:
1. In interference section, replace lissajous sliders:
   - Remove ModulatableSlider calls for freqX1/Y1/X2/Y2/offsetX2/Y2
   - Add ModulatableSlider for amplitude and motionSpeed
   - Add regular SliderFloat for freqX1, freqY1, freqX2, freqY2, offsetX2, offsetY2

**Verify**: Compiles.

#### Task 5.2: Update imgui_effects.cpp (Cymatics + Physarum)

**Files**: `src/ui/imgui_effects.cpp`

**Build**:
1. **Cymatics section**:
   - Find lissajous UI section
   - Replace with ModulatableSlider for amplitude/motionSpeed
   - Add regular SliderFloat for freq/offset params
2. **Physarum section**:
   - Find lissajous UI section
   - Replace inline param sliders with lissajous config sliders
   - Use ModulatableSlider for amplitude/motionSpeed
   - Add regular SliderFloat for freq/offset params
   - Rename lissajousBaseRadius to attractorBaseRadius in UI

**Verify**: Compiles.

#### Task 5.3: Update imgui_effects_warp.cpp (WaveRipple + Mobius)

**Files**: `src/ui/imgui_effects_warp.cpp`

**Build**:
1. **Wave ripple section**:
   - Find origin motion section
   - Replace originAmplitude/FreqX/FreqY sliders with originLissajous config sliders
2. **Mobius section**:
   - Find point motion section
   - Replace pointAmplitude/Freq1/Freq2 sliders with point1Lissajous and point2Lissajous config sliders

**Verify**: Compiles.

#### Task 5.4: Update drawable_type_controls.cpp

**Files**: `src/ui/drawable_type_controls.cpp`

**Build**:
1. Find parametric trail UI section
2. Replace speed/amplitude/freq sliders with lissajous config sliders
3. Use ModulatableSlider for amplitude/motionSpeed
4. Add regular SliderFloat for freq/offset params

**Verify**: Compiles.

---

### Wave 6: Preset Serialization

#### Task 6.1: Update Preset Serialization

**Files**: `src/config/preset.cpp`

**Build**:
1. Add serialization for DualLissajousConfig including new `motionSpeed` and `phase` fields
2. Handle migration: if old field names found, map to new structure
3. Skip serializing `phase` (runtime state, always starts at 0)

**Verify**: Compiles, presets load without crash.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Interference sources move smoothly
- [ ] Cymatics sources move smoothly
- [ ] Physarum attractors move smoothly
- [ ] ParametricTrail cursor moves smoothly
- [ ] WaveRipple origin moves (when amplitude > 0)
- [ ] Mobius points move (when amplitude > 0)
- [ ] Modulating amplitude causes smooth scaling
- [ ] Modulating motionSpeed causes smooth speed change (no position jump)
- [ ] Modulating freq params is NOT possible (removed from registry)
