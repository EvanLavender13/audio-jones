# Visual Parameter Mapping Research

Research for ROADMAP item: Map band energies to visual parameters for audio-reactive effects.

## Current State

### Available Mapping Inputs (Audio-Derived Data)

AudioJones extracts these real-time audio features:

| Source | Output | Range | Update Rate | Location |
|--------|--------|-------|-------------|----------|
| FFTProcessor | 1025 magnitude bins | 0.0-∞ (raw) | 20 Hz | `src/analysis/fft.cpp:80` |
| BeatDetector | beatIntensity | 0.0-1.0 | per-frame | `src/analysis/beat.cpp:95` |
| BeatDetector | beatDetected | bool | per-frame | `src/analysis/beat.cpp:87` |
| BandEnergies | bassSmooth | 0.0-∞ (RMS) | per-frame | `src/analysis/bands.cpp:45` |
| BandEnergies | midSmooth | 0.0-∞ (RMS) | per-frame | `src/analysis/bands.cpp:46` |
| BandEnergies | trebSmooth | 0.0-∞ (RMS) | per-frame | `src/analysis/bands.cpp:47` |
| BandEnergies | bassAvg/midAvg/trebAvg | 0.0-∞ | slow decay | `src/analysis/bands.cpp:50` |

Band definitions at 48kHz/2048 FFT (23.4 Hz/bin):
- Bass: bins 1-10 (20-250 Hz)
- Mid: bins 11-170 (250-4000 Hz)
- Treble: bins 171-853 (4000-20000 Hz)

### Available Mapping Outputs (Visual Parameters)

#### Effect Parameters (`src/config/effect_config.h`)

| Parameter | Type | Range | Current Mapping |
|-----------|------|-------|-----------------|
| halfLife | float | 0.1-2.0 s | static |
| baseBlurScale | int | 0-4 px | static |
| beatBlurScale | int | 0-5 px | beatIntensity modulates |
| chromaticMaxOffset | int | 0-50 px | beatIntensity modulates |
| beatSensitivity | float | 1.0-3.0 | static (threshold control) |

#### Waveform Parameters (`src/config/waveform_config.h`)

| Parameter | Type | Range | Current Mapping |
|-----------|------|-------|-----------------|
| amplitudeScale | float | 0.05-0.5 | static |
| thickness | int | 1-25 px | static |
| smoothness | float | 0.0-100.0 | static |
| radius | float | 0.05-0.45 | static |
| rotationSpeed | float | -0.05-0.05 rad/tick | static |
| rotationOffset | float | 0-2π rad | static |
| color.solid | Color | RGBA | static |
| color.rainbowHue | float | 0-360° | static |
| color.rainbowSat | float | 0.0-1.0 | static |
| color.rainbowVal | float | 0.0-1.0 | static |

#### Spectrum Parameters (`src/config/spectrum_bars_config.h`)

| Parameter | Type | Range | Current Mapping |
|-----------|------|-------|-----------------|
| innerRadius | float | 0.05-0.4 | static |
| barHeight | float | 0.1-0.5 | FFT magnitude (per-band) |
| barWidth | float | 0.3-1.0 | static |
| smoothing | float | 0.0-0.95 | static |
| rotationSpeed | float | -0.05-0.05 rad/tick | static |

#### Band Sensitivity (`src/config/band_config.h`)

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| bassSensitivity | float | 0.5-2.0 | UI multiplier (unused in render) |
| midSensitivity | float | 0.5-2.0 | UI multiplier (unused in render) |
| trebSensitivity | float | 0.5-2.0 | UI multiplier (unused in render) |

## Potential Additional Inputs

Audio features not yet extracted but implementable:

| Feature | Description | Computation | Visual Use Case |
|---------|-------------|-------------|-----------------|
| Spectral Centroid | Frequency "center of gravity" | Σ(f × mag) / Σmag | Controls "brightness" (color hue/saturation) |
| Spectral Spread | Bandwidth around centroid | RMS distance from centroid | Texture complexity |
| Spectral Flux | Rate of spectrum change | Σ(mag[t] - mag[t-1])² | Transient detection (already in beat detector) |
| Zero Crossing Rate | Waveform sign changes | count(sign changes) / N | Noise vs tone detection |
| RMS Energy | Overall loudness | sqrt(Σsample²/N) | Global intensity |
| Peak Amplitude | Maximum sample | max(\|sample\|) | Transient peaks |
| Harmonic Ratio | Harmonic vs percussive energy | HPS algorithm | Separate drum vs melody layers |
| Onset Strength | Transient likelihood | Peak spectral flux | Trigger-based effects |

Implementation complexity:
- Spectral centroid: Low (weighted average of existing FFT bins)
- Zero crossing rate: Low (count in raw audio buffer)
- RMS energy: Low (already computed in BandEnergies)
- Spectral spread: Medium (requires centroid first)
- Harmonic ratio: High (median filtering, two-pass)

## Industry Patterns

### Common Mapping Conventions

Research across [Synesthesia](https://synesthesia.live/), [Magic Music Visuals](https://magicmusicvisuals.com/), [Resolume Arena](https://resolume.com/), and [TouchDesigner](https://derivative.ca/) reveals these patterns:

| Input | Common Targets | Perceptual Basis |
|-------|----------------|------------------|
| Bass energy | Scale, position bounce, camera shake | Low frequencies feel "heavy" |
| Mid energy | Color saturation, brightness | Melody carries emotional content |
| Treble energy | Particle count, detail level, shimmer | High frequencies feel "sparkly" |
| Beat detection | Strobe, flash, pulse, rotation step | Rhythmic sync |
| Spectral centroid | Hue rotation, warmth/coolness | Brightness-color cross-modal mapping |
| Overall loudness | Global intensity, bloom amount | Direct amplitude-brightness mapping |

### Cross-Modal Correspondences

Research-backed audio-visual associations:

| Audio Property | Visual Property | Source |
|----------------|-----------------|--------|
| Pitch (high) | Vertical position (up), brightness | Soundsketcher research |
| Loudness | Size, brightness | Universal mapping |
| Tempo | Animation speed | Rhythmic entrainment |
| Spectral brightness | Color temperature (warm/cool) | Timbre-color mapping |
| Note density | Particle count, complexity | Density transfer |

### Mapping Architectures

#### 1. Hardcoded Mappings
Fixed source→target relationships.

**Pros**: Predictable behavior, no user confusion, optimized performance.
**Cons**: Inflexible, one-size-fits-all.

**Example (current AudioJones)**:
```
beatIntensity → blur scale multiplier
beatIntensity → chromatic aberration offset
```

#### 2. Per-Parameter Linking
Each parameter optionally binds to one audio source.

**Pros**: User control without complexity, straightforward UI (toggle per parameter).
**Cons**: Limited to one source per target.

**Example (Magic Music Visuals)**:
Each parameter has a "link" button. Clicking it binds to audio volume with optional range mapping.

#### 3. Modulation Matrix
Grid of sources × destinations with per-cell amount controls.

**Pros**: Maximum flexibility, multiple sources per target, full routing control.
**Cons**: Complex UI, steep learning curve, performance overhead.

**Example (VS Visual Synthesizer, Ableton Wavetable)**:
```
         | Scale | Hue | Blur | Rotation |
---------|-------|-----|------|----------|
Bass     | 0.5   | 0   | 0.2  | 0        |
Mid      | 0    | 0.8 | 0    | 0        |
Treble   | 0.3   | 0   | 0    | 0.1      |
Beat     | 0    | 0   | 0.5  | 0        |
```

#### 4. Node-Based Routing
Visual patching of sources through processors to destinations.

**Pros**: Arbitrary processing chains, visual clarity for complex setups.
**Cons**: Highest complexity, requires dedicated UI system.

**Example (TouchDesigner CHOP networks)**:
```
Audio In → FFT → Analyze (low/mid/high) → Math → Lag → Export to parameter
```

### Envelope Processing

All professional tools apply envelope followers between raw audio and parameters:

| Stage | Purpose | Typical Parameters |
|-------|---------|-------------------|
| Attack | Smooth rise | 1-50 ms |
| Release | Smooth fall | 50-500 ms |
| Min/Max | Clamp output range | 0.0-1.0 normalized |
| Curve | Response shape | Linear, exponential, logarithmic |
| Invert | Flip polarity | Boolean |

AudioJones already implements attack/release in `BandEnergies` (10ms/150ms).

## Unique Opportunities

### 1. Per-Waveform Mapping
Map different bands to different waveform layers:
- Waveform 0: bass → amplitude
- Waveform 1: mid → amplitude
- Waveform 2: treble → amplitude

Creates visual "stem separation" effect without actual source separation.

### 2. Rotational Sync
Map beat to discrete rotation steps (like a clock tick) instead of continuous rotation:
```c
if (beatDetected) rotationOffset += PI/4;
```

### 3. Spectral Centroid → Hue
Map the "brightness" of sound to color hue in real-time. Bright sounds (high centroid) → cool colors; dark sounds (low centroid) → warm colors.

### 4. Harmonic/Percussive Color Split
If HPS implemented: harmonic content in one color channel, percussive in another. Creates visual drum/melody separation.

### 5. Adaptive Normalization
Current BandEnergies tracks running averages. Use these to auto-normalize:
```c
float normalizedBass = bassSmooth / (bassAvg + epsilon);
```
Produces consistent visual response across quiet and loud tracks.

### 6. Trigger vs Continuous Modes
Some parameters suit continuous modulation (scale, color), others suit trigger-based discrete changes (rotation step, layer switch). Expose this choice to users.

## User-Configurable vs Hardcoded

### Arguments for Hardcoded (Phase 1)
- Faster implementation
- Guaranteed good defaults
- Matches ROADMAP phases (bass→scale, mid→saturation, treble→bloom)
- No UI complexity

### Arguments for User-Configurable (Phase 2+)
- Different music genres need different mappings
- Artists expect customization
- Preset system already exists for persistence
- Differentiates from simpler visualizers

### Recommended Approach
**Hybrid**: Start with hardcoded mappings per ROADMAP, but architect for extensibility:

1. Define `MappingConfig` struct with source/target/amount/range fields
2. Initialize with hardcoded defaults
3. Skip UI initially (use hardcoded values)
4. Add mapping UI in future phase
5. Presets automatically save/load mappings

```c
typedef struct {
    int source;           // MAPPING_SOURCE_BASS, _MID, _TREB, _BEAT, etc.
    int target;           // MAPPING_TARGET_SCALE, _HUE, _BLOOM, etc.
    float amount;         // 0.0-1.0 modulation depth
    float min;            // Output range minimum
    float max;            // Output range maximum
    bool enabled;
} ParameterMapping;
```

## Implementation Considerations

### Performance
- Band energies already computed at 60 Hz
- Adding 3-5 mappings per frame: negligible cost
- Modulation matrix (N sources × M targets): still negligible at current scale

### UI Options
1. **Accordion section per mappable parameter** - Add source dropdown + amount slider
2. **Dedicated mapping panel** - Centralized matrix view
3. **Right-click context menu** - "Link to audio" on any slider

### Preset Compatibility
New mapping fields require default values for backward compatibility. The JSON serializer macro `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` handles this automatically.

## Recommended Phase 1 Mappings

Per ROADMAP:

| Source | Target | Amount | Implementation |
|--------|--------|--------|----------------|
| bassSmooth × bassSensitivity | waveform.amplitudeScale | +0.0-0.3 | Additive to base value |
| midSmooth × midSensitivity | color.rainbowSat | 0.5-1.0 | Override when mid high |
| trebSmooth × trebSensitivity | effects.beatBlurScale | +0-3 | Additive to base value |

Normalization strategy: Divide by running average (`bassSmooth / bassAvg`) to produce 0.0-2.0 range (1.0 = average energy).

## Sources

- [Synesthesia](https://synesthesia.live/) - MIDI/OSC mapping, shader import
- [Magic Music Visuals](https://magicmusicvisuals.com/) - Node-based linking, MIDI control
- [TouchDesigner CHOPs](https://docs.derivative.ca/Audio_Device_In_CHOP) - Parameter export system
- [Resolume FFT](https://resolume.com/support/en/wire-fft) - Frequency band isolation (low/mid/high buttons)
- [VS Visual Synthesizer](https://www.imaginando.pt/products/vs-visual-synthesizer/help/vs2/audio-reactive-visuals) - Modulation matrix UI
- [Spectral Centroid (ScienceDirect)](https://www.sciencedirect.com/topics/engineering/spectral-centroid) - Brightness feature definition
- [Harmonic-Percussive Separation (AudioLabs)](https://www.audiolabs-erlangen.de/resources/MIR/FMP/C8/C8S1_HPS.html) - Median filtering technique
- [Soundsketcher research](https://www.researchgate.net/publication/200806069_The_Sonic_Visualiser_A_Visualisation_Platform_for_Semantic_Descriptors_from_Musical_Signals) - Cross-modal correspondences
