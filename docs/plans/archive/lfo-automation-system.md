# LFO Automation System

Add a Low Frequency Oscillator (LFO) system to automate visual parameters over time, starting with feedback rotation.

## Goal

Enable smooth, automated parameter variation without user intervention. An LFO generates periodic control signals that modulate visual parameters like rotation, zoom, and desaturation. This creates organic, evolving visuals reminiscent of MilkDrop's per-frame equations while remaining simple to configure.

The LFO system provides a foundation for future modulation—once implemented, additional parameters can be LFO-controlled with minimal code changes.

## Current State

Visual effect parameters in `src/config/effect_config.h:4-12` remain static per frame:

```cpp
float feedbackZoom = 0.98f;       // Could oscillate between zoom in/out
float feedbackRotation = 0.005f; // Always rotates same direction
float feedbackDesaturate = 0.05f;// Always same desaturation rate
```

The UI in `src/ui/ui_panel_effects.cpp:22` exposes rotation as a one-directional slider (0.0 to 0.02). Users cannot make rotation reverse direction or vary over time.

The main loop in `src/main.cpp:147` already computes `deltaTime = GetFrameTime()`, which provides the time delta needed for LFO phase accumulation.

## Algorithm

### LFO Core

An LFO produces a value oscillating between -1.0 and +1.0 based on:
- **Phase**: Current position in the cycle (0.0 to 1.0, wraps)
- **Rate**: Oscillation frequency in Hz (cycles per second)
- **Waveform**: Shape of the oscillation

Phase advances each frame:
```cpp
phase += rate * deltaTime;
if (phase >= 1.0f) phase -= 1.0f;
```

### Waveform Functions

Each waveform maps phase (0.0–1.0) to output (-1.0 to +1.0):

| Waveform | Formula | Character |
|----------|---------|-----------|
| Sine | `sinf(phase * TAU)` | Smooth, organic |
| Triangle | `1.0 - fabsf(phase * 4.0 - 2.0)` | Linear, balanced |
| Sawtooth | `phase * 2.0 - 1.0` | Rising ramp, reset |
| Square | `phase < 0.5 ? 1.0 : -1.0` | Abrupt toggle |
| Sample & Hold | Random value, held until next cycle | Unpredictable |

**Rationale**: Sine produces the smoothest motion; triangle feels more mechanical but predictable; sample & hold adds controlled chaos.

### Modulation Application

The LFO output (-1.0 to +1.0) modulates parameters via depth scaling:

```cpp
float effectiveRotation = baseRotation + lfoOutput * depth;
```

| Parameter | Depth Meaning | Typical Range |
|-----------|---------------|---------------|
| Rotation | `depth` = ±rotation when LFO at ±1 | 0.005–0.02 rad |
| Zoom | `depth` = ±zoom deviation | 0.01–0.05 |
| Desaturate | `depth` = ±desaturate deviation | 0.02–0.1 |

### Default Parameters

| Parameter | Default | Rationale |
|-----------|---------|-----------|
| Rate | 0.1 Hz | 10-second full cycle; slow enough to feel ambient |
| Depth | 0.01 rad | Subtle rotation that reverses smoothly |
| Waveform | Sine | Smoothest transitions |
| Enabled | false | Opt-in to preserve existing behavior |

## Integration

### 1. Create LFO Config (`src/config/lfo_config.h`)

Define the LFO parameter structure:

```cpp
#ifndef LFO_CONFIG_H
#define LFO_CONFIG_H

typedef enum {
    LFO_WAVE_SINE,
    LFO_WAVE_TRIANGLE,
    LFO_WAVE_SAWTOOTH,
    LFO_WAVE_SQUARE,
    LFO_WAVE_SAMPLE_HOLD,
    LFO_WAVE_COUNT
} LFOWaveform;

struct LFOConfig {
    bool enabled = false;
    float rate = 0.1f;          // Hz
    float depth = 0.01f;        // Modulation amplitude
    int waveform = LFO_WAVE_SINE;
};

#endif
```

### 2. Create LFO Processor (`src/automation/lfo.h`, `src/automation/lfo.cpp`)

Encapsulate LFO state and update logic:

```cpp
// lfo.h
#ifndef LFO_H
#define LFO_H

#include "config/lfo_config.h"

typedef struct {
    float phase;           // 0.0 to 1.0
    float currentOutput;   // -1.0 to 1.0
    float heldValue;       // For sample & hold
} LFOState;

void LFOStateInit(LFOState* state);
float LFOProcess(LFOState* state, const LFOConfig* config, float deltaTime);

#endif
```

Implementation handles phase accumulation, waveform generation, and sample & hold randomization.

### 3. Extend EffectConfig (`src/config/effect_config.h`)

Add LFO config for rotation (and optionally zoom/desat):

```cpp
struct EffectConfig {
    // ... existing fields ...

    // LFO automation
    LFOConfig rotationLFO;
    // Future: LFOConfig zoomLFO;
    // Future: LFOConfig desatLFO;
};
```

### 4. Add LFO State to PostEffect (`src/render/post_effect.h`)

Store runtime LFO state alongside effect config:

```cpp
typedef struct PostEffect {
    // ... existing fields ...
    LFOState rotationLFOState;
} PostEffect;
```

Initialize in `PostEffectInit()`.

### 5. Apply LFO in Render Pipeline (`src/render/post_effect.cpp:110-125`)

In `PostEffectBeginAccum()`, compute modulated rotation before shader upload:

```cpp
float effectiveRotation = pe->effects.feedbackRotation;
if (pe->effects.rotationLFO.enabled) {
    float lfoValue = LFOProcess(&pe->rotationLFOState,
                                 &pe->effects.rotationLFO,
                                 deltaTime);
    effectiveRotation += lfoValue * pe->effects.rotationLFO.depth;
}
SetShaderValue(pe->feedbackShader, pe->feedbackRotationLoc,
               &effectiveRotation, SHADER_UNIFORM_FLOAT);
```

### 6. Add UI Controls (`src/ui/ui_panel_effects.cpp`)

Add LFO controls after the rotation slider:

```cpp
DrawLabeledSlider(l, "Rotation", &effects->feedbackRotation, 0.0f, 0.02f);

// LFO sub-section
DrawCheckbox(l, "LFO", &effects->rotationLFO.enabled);
if (effects->rotationLFO.enabled) {
    DrawLabeledSlider(l, "Rate", &effects->rotationLFO.rate, 0.01f, 1.0f);
    DrawLabeledSlider(l, "Depth", &effects->rotationLFO.depth, 0.0f, 0.03f);
    DrawDropdown(l, "Wave", &effects->rotationLFO.waveform,
                 "Sine;Triangle;Saw;Square;S&H");
}
```

### 7. Extend Preset Serialization (`src/preset.cpp`)

Add LFOConfig serialization using existing macro pattern:

```cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LFOConfig,
    enabled, rate, depth, waveform)
```

Update EffectConfig serialization to include `rotationLFO`.

## File Changes

| File | Change |
|------|--------|
| `src/config/lfo_config.h` | Create - LFOConfig struct and LFOWaveform enum |
| `src/automation/lfo.h` | Create - LFOState struct and function declarations |
| `src/automation/lfo.cpp` | Create - LFO processing implementation |
| `src/config/effect_config.h` | Add `rotationLFO` field |
| `src/render/post_effect.h` | Add `rotationLFOState` field |
| `src/render/post_effect.cpp` | Initialize LFO state, apply modulation |
| `src/ui/ui_panel_effects.cpp` | Add LFO controls |
| `src/preset.cpp` | Serialize LFOConfig |
| `CMakeLists.txt` | Add `src/automation/lfo.cpp` |

## Validation

- [ ] With LFO disabled, rotation behaves identically to current implementation
- [ ] Enabling sine LFO produces smooth back-and-forth rotation
- [ ] Triangle LFO produces linear direction changes
- [ ] Sawtooth LFO produces rising rotation with periodic reset
- [ ] Square LFO produces abrupt direction toggles
- [ ] Sample & Hold produces random rotation values that hold steady between changes
- [ ] Rate slider changes oscillation speed (0.01 Hz = 100s cycle, 1.0 Hz = 1s cycle)
- [ ] Depth slider controls modulation intensity
- [ ] LFO settings save and load correctly in presets
- [ ] No performance impact (LFO computation is trivial)

## Future Extensions

After rotation LFO validates the system:

1. **Additional targets**: Add `zoomLFO` and `desatLFO` to EffectConfig
2. **Beat sync**: Option to reset LFO phase on beat detection
3. **Tempo sync**: Lock rate to BPM divisions (1/4 note, 1/8 note, etc.)
4. **Multiple LFOs**: Sum LFOs at different rates for complex motion
5. **LFO-to-LFO**: Modulate one LFO's rate or depth with another
6. **Custom curves**: User-drawn waveforms (like Serum/Vital)

## References

- [Lunacy Audio - LFO Ultimate Guide](https://lunacy.audio/low-frequency-oscillator-lfos/) - Comprehensive LFO parameter overview
- [LFO Shapes Guide](https://audioservices.studio/sound-design/lfo-shapes-a-guide-to-modulating-sound-with-different-waveforms) - Waveform characteristics
- [MilkDrop Preset Authoring](http://wiki.shoutcast.com/wiki/MilkDrop_Preset_Authoring) - Per-frame equation inspiration
