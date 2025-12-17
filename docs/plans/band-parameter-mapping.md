# Band-to-Parameter Mapping

UI-driven system to modulate effect parameters with audio frequency band energy values.

## Goal

Enable real-time audio reactivity by connecting bass/mid/treble energy to visual effect parameters. Starting with physarum sensing and turning parameters as a prototype, with architecture designed to expand to other parameters and modulation sources.

This creates tighter audio-visual coupling than the current beat-intensity bloom, allowing users to customize which frequency bands drive which visual behaviors.

## Current State

### Band Energy System
- `src/analysis/bands.h:17-32` defines `BandEnergies` struct with raw, smoothed, and average values for bass/mid/treble
- `src/analysis/bands.cpp:50-71` computes band RMS from FFT magnitude with attack/release envelope
- Band values are smoothed (10ms attack, 150ms release) and normalized against running average
- Currently only displayed in UI band meter (`src/ui/ui_widgets.cpp:188-254`), not used to drive effects

### Physarum Parameters
- `src/render/physarum.h:15-28` defines `PhysarumConfig` with target parameters:
  - `sensorDistance` (1-100px) - how far agents look ahead
  - `sensorAngle` (0-6.28rad) - angle spread of sensors
  - `turningAngle` (0-6.28rad) - how sharply agents turn
- `src/render/physarum.cpp:298-300` sends values to GPU via uniforms
- `src/render/post_effect.cpp:134` calls `PhysarumApplyConfig()` before update

### Existing Modulation Pattern
- `src/automation/lfo.h:6-10` defines `LFOState` with phase tracking
- `src/config/lfo_config.h:13-17` defines `LFOConfig` with rate/waveform
- `src/render/post_effect.cpp:283-287` applies LFO modulation: `effectiveRotation *= lfoValue`
- Pattern: Config (user settings) + State (runtime) + Process function

### UI Structure
- `src/ui/ui_main.cpp:111-161` renders accordion sections
- `src/ui/ui_panel_analysis.cpp:4-18` draws beat graph and band meter
- `src/ui/ui_panel_effects.cpp:10-80` draws effect controls with conditional sections

## Algorithm

### Modulation Formula

Multiplicative modulation with configurable depth:

```
effectiveValue = baseValue * (1.0 + normalizedBandEnergy * depth)
```

| Term | Description |
|------|-------------|
| `baseValue` | User-configured parameter value from slider |
| `normalizedBandEnergy` | `bandSmooth / bandAvg` (0.0-2.0 typical range) |
| `depth` | User-configured modulation amount (0.0-2.0) |

At depth=1.0, a band at 2x its average doubles the parameter. At depth=0.0, no modulation occurs.

### Config Structure

```cpp
// src/config/band_mapping_config.h

typedef enum {
    BAND_TARGET_NONE,
    BAND_TARGET_PHYSARUM_SENSOR,
    BAND_TARGET_PHYSARUM_ANGLE,
    BAND_TARGET_PHYSARUM_TURN,
    BAND_TARGET_COUNT
} BandTarget;

typedef struct {
    BandTarget target;  // Which parameter this band modulates
    float depth;        // Modulation depth (0.0-2.0)
} BandMapping;

typedef struct {
    BandMapping bass;
    BandMapping mid;
    BandMapping treb;
} BandMappingConfig;
```

### Modulation Module

```cpp
// src/automation/band_mod.h

// Compute normalized band value (smoothed / average), clamped to [0, maxNorm]
float BandModGetNormalizedValue(const BandEnergies* bands, int bandIndex, float maxNorm);

// Apply band modulation to physarum config
// Modifies effective values in-place based on mapping config
void BandModApplyPhysarum(PhysarumConfig* effective,
                          const PhysarumConfig* base,
                          const BandEnergies* bands,
                          const BandMappingConfig* mapping);
```

### Modulation Application

```cpp
// In src/render/post_effect.cpp, before PhysarumApplyConfig():

if (pe->effects.physarum.enabled) {
    PhysarumConfig effectivePhysarum = pe->effects.physarum;  // Copy base
    BandModApplyPhysarum(&effectivePhysarum, &pe->effects.physarum, bands, mapping);
    PhysarumApplyConfig(pe->physarum, &effectivePhysarum);
    // ... rest unchanged
}
```

## Integration

### Data Flow

```
BandEnergies (analysis) ──┐
                          ├──> BandModApplyPhysarum() ──> effectivePhysarum ──> GPU
PhysarumConfig (base) ────┘
BandMappingConfig (UI) ───┘
```

### Hook Points

1. **Config Storage** (`src/config/effect_config.h:7-28`)
   - Add `BandMappingConfig bandMapping` field to `EffectConfig`

2. **PostEffect API** (`src/render/post_effect.h:65-66`)
   - Extend `PostEffectBeginAccum()` signature to receive `BandEnergies*`

3. **Main Loop** (`src/main.cpp:175-176`)
   - Pass `&ctx->analysis.bands` to `PostEffectBeginAccum()`

4. **UI Panel** (`src/ui/ui_main.cpp`)
   - Add new "Band Mapping" accordion section
   - Move band meter from Analysis panel
   - Add target dropdown + depth slider per band

### UI Layout

```
[+] Band Mapping
    ┌──────────────────────────────────┐
    │ [Bass ███░░] [Mid ██░░░] [Treb █░░░░] │  <- Band meter (moved)
    ├──────────────────────────────────┤
    │ Bass  [None      ▼] Depth [====] 1.0 │  <- Dropdown + slider
    │ Mid   [P.Sensor  ▼] Depth [==  ] 0.5 │
    │ Treb  [P.Turn    ▼] Depth [=== ] 0.8 │
    └──────────────────────────────────┘
```

Target dropdown options: "None", "P.Sensor", "P.Angle", "P.Turn"

## File Changes

| File | Change |
|------|--------|
| `src/config/band_mapping_config.h` | Create - BandTarget enum, BandMapping/BandMappingConfig structs |
| `src/automation/band_mod.h` | Create - BandModGetNormalizedValue, BandModApplyPhysarum declarations |
| `src/automation/band_mod.cpp` | Create - Modulation implementation |
| `src/config/effect_config.h` | Add `BandMappingConfig bandMapping` field |
| `src/render/post_effect.h` | Add `BandEnergies*` parameter to `PostEffectBeginAccum()` |
| `src/render/post_effect.cpp` | Apply band modulation before `PhysarumApplyConfig()` |
| `src/main.cpp` | Pass bands to `PostEffectBeginAccum()` |
| `src/ui/ui_panel_band_mapping.h` | Create - Panel function declaration |
| `src/ui/ui_panel_band_mapping.cpp` | Create - Band meter + mapping controls |
| `src/ui/ui_main.h` | Add accordion expansion flag for band mapping |
| `src/ui/ui_main.cpp` | Add Band Mapping accordion section, remove band meter from Analysis |
| `src/ui/ui_panel_analysis.cpp` | Remove band meter (moved to Band Mapping panel) |
| `src/config/preset.cpp` | Add BandMappingConfig serialization |

## Build Sequence

### Phase 1: Config and Module (Single Session)

1. Create `src/config/band_mapping_config.h` with enums and structs
2. Create `src/automation/band_mod.h` with function declarations
3. Create `src/automation/band_mod.cpp` with implementation
4. Add `BandMappingConfig` to `EffectConfig`
5. Update `PostEffectBeginAccum()` signature and implementation
6. Update `main.cpp` call site
7. Add serialization to `preset.cpp`
8. Build and verify no regressions

### Phase 2: UI Panel (Single Session)

1. Create `src/ui/ui_panel_band_mapping.h` with panel declaration
2. Create `src/ui/ui_panel_band_mapping.cpp` with band meter + controls
3. Add accordion section to `ui_main.cpp`
4. Remove band meter from `ui_panel_analysis.cpp`
5. Add dropdown state to `PanelState` if needed
6. Build and test UI interaction

## Validation

- [ ] Band mapping config persists in presets (save/load roundtrip)
- [ ] Physarum sensor distance visibly changes with bass when mapped
- [ ] Physarum turning angle visibly changes with treble when mapped
- [ ] Depth=0 produces no modulation (parameter unchanged)
- [ ] Depth=2 produces strong modulation (parameter doubles at 2x average energy)
- [ ] Target=None disables modulation for that band
- [ ] Band meter displays correctly in new panel location
- [ ] Analysis panel still shows beat graph without band meter
- [ ] No performance regression (modulation is simple arithmetic)

## Future Extensions

Once the pattern is established, easy to add:
- More targets: bloom, rotation, chromatic offset, voronoi intensity
- More sources: beat intensity, spectral centroid, onset detection
- Per-target depth curves (linear, exponential, threshold)
- Modulation visualization (show effective vs base values)
