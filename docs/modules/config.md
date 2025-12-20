# Config Module

> Part of [AudioJones](../architecture.md)

## Purpose

Defines serializable parameter structs and JSON preset save/load.

## Files

- `src/config/lfo_config.h` - LFO waveform and rate parameters
- `src/config/effect_config.h` - Post-effect parameters (includes LFOConfig)
- `src/config/waveform_config.h` - Per-waveform parameters
- `src/config/spectrum_bars_config.h` - Spectrum display parameters
- `src/config/band_config.h` - Band energy sensitivity parameters
- `src/config/app_configs.h` - Aggregated config pointer struct for UI
- `src/config/preset.h` - Preset struct and I/O API
- `src/config/preset.cpp` - nlohmann/json serialization

## Function Reference

| Function | Purpose |
|----------|---------|
| `PresetDefault` | Returns default preset with one waveform |
| `PresetSave` | Writes preset to JSON file |
| `PresetLoad` | Reads preset from JSON file |
| `PresetListFiles` | Lists preset files in directory (max 32) |
| `PresetFromAppConfigs` | Copies app config values into preset for saving |
| `PresetToAppConfigs` | Copies preset values into app configs for loading |

## Types

### EffectConfig

| Field | Default | Range | Description |
|-------|---------|-------|-------------|
| `halfLife` | 0.5 | 0.1-2.0s | Trail persistence |
| `baseBlurScale` | 1 | 0-4px | Base blur distance |
| `beatBlurScale` | 2 | 0-5px | Additional blur on beats |
| `chromaticMaxOffset` | 12 | 0-20px | Max RGB split on beats |
| `feedbackZoom` | 0.98 | 0.9-1.0 | Zoom per frame (lower = faster inward) |
| `feedbackRotation` | 0.005 | 0-0.05rad | Rotation per frame |
| `feedbackDesaturate` | 0.05 | 0-0.2 | Fade toward dark gray per frame |
| `warpStrength` | 0.0 | 0-1 | Domain warp intensity (0 = disabled) |
| `warpScale` | 5.0 | 1-20 | Noise frequency (lower = larger swirls) |
| `warpOctaves` | 3 | 1-5 | Detail layers |
| `warpLacunarity` | 2.0 | 1-4 | Frequency multiplier per octave |
| `warpSpeed` | 0.5 | 0-5 | Animation rate |
| `kaleidoSegments` | 1 | 1,4,6,8,12 | Mirror segments (1 = disabled) |
| `voronoiScale` | 15.0 | 5-50 | Cell count across screen |
| `voronoiIntensity` | 0.0 | 0-1 | Blend amount (0 = disabled) |
| `voronoiSpeed` | 0.5 | - | Animation rate |
| `voronoiEdgeWidth` | 0.05 | - | Edge thickness |
| `rotationLFO` | - | - | LFOConfig for animated rotation |
| `physarum` | - | - | PhysarumConfig for slime mold simulation |

### LFOConfig

| Field | Default | Range | Description |
|-------|---------|-------|-------------|
| `enabled` | false | - | Enable LFO modulation |
| `rate` | 0.1 | 0.01-10.0Hz | Oscillation frequency |
| `waveform` | `LFO_WAVE_SINE` | - | Waveform shape |

### LFOWaveform

| Value | Description |
|-------|-------------|
| `LFO_WAVE_SINE` | Smooth sinusoidal |
| `LFO_WAVE_TRIANGLE` | Linear ramp up/down |
| `LFO_WAVE_SAWTOOTH` | Linear ramp up, instant reset |
| `LFO_WAVE_SQUARE` | Binary high/low |
| `LFO_WAVE_SAMPLE_HOLD` | Random value per cycle |

### WaveformConfig

| Field | Default | Range | Description |
|-------|---------|-------|-------------|
| `amplitudeScale` | 0.35 | 0.05-0.5 | Height relative to minDim |
| `thickness` | 2 | 1-25px | Line thickness |
| `smoothness` | 5.0 | 0-50 | Smoothing window radius |
| `radius` | 0.25 | 0.05-0.45 | Base radius fraction |
| `rotationSpeed` | 0.0 | -0.05-0.05 | Radians per tick |
| `rotationOffset` | 0.0 | - | Base rotation offset |
| `color` | - | - | ColorConfig |

### SpectrumConfig

| Field | Default | Range | Description |
|-------|---------|-------|-------------|
| `enabled` | false | - | Show spectrum bars |
| `innerRadius` | 0.15 | - | Base radius fraction |
| `barHeight` | 0.25 | - | Max bar height fraction |
| `barWidth` | 0.8 | 0.5-1.0 | Bar width fraction |
| `smoothing` | 0.8 | 0.0-0.95 | Per-band decay rate |
| `minDb`, `maxDb` | 10, 50 | - | dB thresholds |
| `rotationSpeed`, `rotationOffset` | 0.0 | - | Rotation parameters |
| `color` | - | - | ColorConfig |

### Preset

| Field | Description |
|-------|-------------|
| `name[64]` | Display name |
| `effects` | EffectConfig |
| `audio` | AudioConfig |
| `waveforms[8]` | WaveformConfig array |
| `waveformCount` | Active layers (1-8) |
| `spectrum` | SpectrumConfig |
| `bands` | BandConfig |

### BandConfig

| Field | Default | Range | Description |
|-------|---------|-------|-------------|
| `bassSensitivity` | 0.5 | 0.5-2.0 | Bass meter multiplier |
| `midSensitivity` | 0.5 | 0.5-2.0 | Mid meter multiplier |
| `trebSensitivity` | 0.5 | 0.5-2.0 | Treble meter multiplier |

### AppConfigs

Aggregates pointers to all config structs for UI panel functions.

| Field | Type | Description |
|-------|------|-------------|
| `waveforms` | `WaveformConfig*` | Array of waveform configs |
| `waveformCount` | `int*` | Active layer count |
| `selectedWaveform` | `int*` | UI selection index |
| `effects` | `EffectConfig*` | Post-effect parameters |
| `audio` | `AudioConfig*` | Channel mode |
| `spectrum` | `SpectrumConfig*` | Spectrum settings |
| `beat` | `BeatDetector*` | Beat detection state |
| `bands` | `BandConfig*` | Band sensitivity |
| `bandEnergies` | `BandEnergies*` | Live band energy values |

## Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `PRESET_NAME_MAX` | 64 | Max preset name length |
| `PRESET_PATH_MAX` | 256 | Max file path length |
| `MAX_PRESET_FILES` | 32 | Max presets listed |

## Data Flow

1. **Entry:** UI modifies config structs in-place
2. **Save:** `PresetSave` serializes to `presets/*.json`
3. **Load:** `PresetLoad` deserializes and overwrites current config
