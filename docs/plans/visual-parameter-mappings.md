# Visual Parameter Mappings

Map band energies to visual parameters for audio-reactive effects.

## Goal

Connect bass, mid, and treble frequency bands to waveform scale, color saturation, and bloom intensity. Produces automatic audio-reactive behavior without user intervention—bass pulses grow the waveform, bright mids saturate colors, treble adds sparkle via bloom.

## Current State

Band energies are extracted and smoothed in `src/analysis/bands.cpp:50-70`:
- `bassSmooth`, `midSmooth`, `trebSmooth` - envelope-followed values (10ms attack, 150ms release)
- `bassAvg`, `midAvg`, `trebAvg` - running averages for normalization

Visual parameters exist but remain static:
- `WaveformConfig::amplitudeScale` used at `src/render/waveform.cpp:177,213`
- `ColorConfig::rainbowSat` used at `src/render/waveform.cpp:15`
- `EffectConfig::beatBlurScale` applied in post-processing

The draw loop in `src/main.cpp:196-210` passes `WaveformConfig*` to render functions. Band energies exist in `AppContext::bands` but no code bridges them to visual parameters.

## Algorithm

### Normalization

Raw RMS values vary with input level. Normalize by dividing by running average:

```c
float normalizedBass = bands->bassSmooth / (bands->bassAvg + 1e-6f);
```

Produces values centered around 1.0 (average energy), ranging 0.0–2.0+ during dynamics.

### Mapping Functions

Each mapping applies a normalized band value to modulate a base parameter:

| Source | Target | Formula | Rationale |
|--------|--------|---------|-----------|
| Bass | amplitudeScale | `base + (normalized - 1.0) * depth` | Additive around neutral |
| Mid | rainbowSat | `base + (normalized - 1.0) * depth` | Saturation boost on melody |
| Treble | beatBlurScale | `base + normalized * depth` | Proportional bloom |

**Depth controls**: Initial values allow tuning responsiveness:
- Bass→Scale depth: 0.15 (±15% of minDim at ±1σ from average)
- Mid→Sat depth: 0.3 (±30% saturation swing)
- Treble→Bloom depth: 2.0 (0–4 extra blur pixels)

### Clamp Output Ranges

Prevent visual artifacts from extreme values:

| Parameter | Min | Max | Reason |
|-----------|-----|-----|--------|
| amplitudeScale | 0.05 | 0.6 | Keep waveform visible, prevent overflow |
| rainbowSat | 0.2 | 1.0 | Avoid grayscale, allow full saturation |
| beatBlurScale | 0 | 6 | Shader limit |

## Integration

### 1. Add Mapping Config (`src/config/mapping_config.h`)

Create new header for mapping parameters:

```c
#ifndef MAPPING_CONFIG_H
#define MAPPING_CONFIG_H

struct MappingConfig {
    bool bassToScale = true;
    float bassScaleDepth = 0.15f;

    bool midToSat = true;
    float midSatDepth = 0.3f;

    bool trebToBloom = true;
    float trebBloomDepth = 2.0f;
};

#endif
```

### 2. Add to AppContext (`src/main.cpp:31`)

Insert after `BandConfig bandConfig;`:

```c
MappingConfig mapping;
```

### 3. Create Mapping Processor (`src/analysis/mapping.h`, `src/analysis/mapping.cpp`)

Compute effective visual values each frame:

```c
// mapping.h
#ifndef MAPPING_H
#define MAPPING_H

#include "bands.h"
#include "config/mapping_config.h"
#include "config/waveform_config.h"
#include "config/effect_config.h"

typedef struct {
    float effectiveScale;    // Modulated amplitudeScale
    float effectiveSat;      // Modulated rainbowSat
    int effectiveBloom;      // Modulated beatBlurScale
} MappingOutput;

void MappingProcess(MappingOutput* out, const BandEnergies* bands,
                    const MappingConfig* cfg, const WaveformConfig* wf,
                    const EffectConfig* fx);

#endif
```

Implementation computes normalized values and applies formulas from Algorithm section.

### 4. Apply in Render Loop (`src/main.cpp:190-210`)

Before draw calls, compute effective values and substitute:

```c
// Compute mappings
MappingOutput mappedValues;
MappingProcess(&mappedValues, &ctx->bands, &ctx->mapping,
               &ctx->waveforms[0], &ctx->postEffect->effects);

// Store original values
float origScale = ctx->waveforms[0].amplitudeScale;
float origSat = ctx->waveforms[0].color.rainbowSat;

// Apply mapped values for this frame
ctx->waveforms[0].amplitudeScale = mappedValues.effectiveScale;
ctx->waveforms[0].color.rainbowSat = mappedValues.effectiveSat;

// Draw
DrawScene(ctx, &renderCtx);

// Restore (preserves user's base settings for presets)
ctx->waveforms[0].amplitudeScale = origScale;
ctx->waveforms[0].color.rainbowSat = origSat;
```

Bloom handled separately in post-effect pipeline—pass `mappedValues.effectiveBloom` to `PostEffectApply()`.

### 5. Extend Preset Serialization (`src/preset.cpp`)

Add `MappingConfig` fields to preset JSON. Use existing macro pattern:

```c
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MappingConfig,
    bassToScale, bassScaleDepth, midToSat, midSatDepth, trebToBloom, trebBloomDepth)
```

## File Changes

| File | Change |
|------|--------|
| `src/config/mapping_config.h` | Create - struct definition |
| `src/analysis/mapping.h` | Create - function declarations |
| `src/analysis/mapping.cpp` | Create - mapping processor |
| `src/main.cpp` | Add MappingConfig to AppContext, call MappingProcess, apply values |
| `src/preset.cpp` | Serialize MappingConfig |
| `CMakeLists.txt` | Add `src/analysis/mapping.cpp` |

## Validation

- [ ] Bass-heavy music (kick drums, bass drops) visibly pulses waveform scale
- [ ] Mid-heavy passages (vocals, guitar) increase color saturation
- [ ] Treble-heavy content (hi-hats, cymbals) increases bloom glow
- [ ] Quiet passages return to base visual values (no residual modulation)
- [ ] Presets save and restore mapping enable/depth settings
- [ ] Frame rate stays above 60 FPS (negligible computation cost)

## Future Extensions

After this phase completes:
- UI controls for mapping enable toggles and depth sliders
- Per-waveform mapping (different bands to different layers)
- Additional targets: rotation speed, hue shift, thickness

## References

- `docs/research/visual-parameter-mapping.md` - Detailed research on industry patterns and mapping architectures
- `docs/plans/archive/multi-band-energy-extraction.md` - Prior phase implementing band extraction
