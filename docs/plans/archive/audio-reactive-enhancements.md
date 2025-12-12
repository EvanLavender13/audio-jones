# Audio-Reactive Enhancements Plan

Implementation plan based on deep research analysis (`docs/research/deep-research1.md`). Addresses gaps between current AudioJones architecture and industry-standard audio visualization patterns.

## Current State

AudioJones implements:
- 2-thread architecture with lock-free ring buffer (`src/audio.cpp:7-46`)
- 2048-sample FFT with Hann window, 75% overlap (`src/spectral.cpp:8-99`)
- Spectral flux beat detection in bass range (`src/beat.cpp:47-99`)
- Radial waveform with 5-tap Gaussian bloom (`src/waveform.cpp:205-256`, `shaders/blur_v.fs`)
- Beat intensity drives blur scale and chromatic aberration (`src/visualizer.cpp:118,152`)

## Gap Analysis

| Category | Gap | Impact | Complexity |
|----------|-----|--------|------------|
| Features | No mid/treble band extraction | High | Low |
| Smoothing | Single-rate exponential only | High | Low |
| Beat detection | No log compression | Medium | Low |
| GPU pipeline | FFT stays on CPU | High | Medium |
| Features | No spectral centroid | Medium | Low |
| Bloom | Single-pass 5-tap blur | Low | Medium |

---

## Phase 1: Multi-Band Feature Extraction

**Goal**: Extract bass, mid, treble energy bands with MilkDrop-style smoothed values.

### 1.1 Add Band Energy Extraction

Create `src/bands.h` and `src/bands.cpp`:

```cpp
// Frequency ranges at 48kHz, 2048 FFT (23.4Hz per bin)
#define BAND_BASS_LOW   1    // ~23Hz
#define BAND_BASS_HIGH  8    // ~188Hz
#define BAND_MID_LOW    9    // ~211Hz
#define BAND_MID_HIGH   85   // ~2kHz
#define BAND_TREB_LOW   86   // ~2kHz
#define BAND_TREB_HIGH  512  // ~12kHz

typedef struct BandEnergies {
    float bass;         // RMS energy 20-200Hz
    float mid;          // RMS energy 200-2000Hz
    float treb;         // RMS energy 2000-12000Hz
    float bassSmooth;   // Attack/release smoothed
    float midSmooth;
    float trebSmooth;
} BandEnergies;
```

**Implementation**: Compute RMS over each bin range in `SpectralProcessorUpdate()` after magnitude calculation (`src/spectral.cpp:88-93`).

### 1.2 Dual Attack/Release Smoothing

Replace single-rate exponential smoothing with asymmetric envelope follower:

```cpp
// In bands.cpp
void SmoothBandEnergy(float* smoothed, float raw, float dt) {
    float alpha;
    if (raw > *smoothed) {
        alpha = 1.0f - expf(-dt / ATTACK_TIME);   // ATTACK_TIME = 0.01s (10ms)
    } else {
        alpha = 1.0f - expf(-dt / RELEASE_TIME);  // RELEASE_TIME = 0.15s (150ms)
    }
    *smoothed = *smoothed + alpha * (raw - *smoothed);
}
```

**Rationale**: Fast attack (10ms) captures transients. Slow release (150ms) prevents jitter. These values match TouchDesigner's Lag CHOP defaults.

### 1.3 Integration Points

- Add `BandEnergies` to `AppContext` (`src/main.cpp`)
- Call band extraction after `SpectralProcessorUpdate()` in main loop
- Expose `bassSmooth`, `midSmooth`, `trebSmooth` to UI and visualization systems

**Files modified**: `src/spectral.cpp`, `src/main.cpp`
**Files created**: `src/bands.h`, `src/bands.cpp`

---

## Phase 2: Beat Detection Improvements

**Goal**: Increase onset detection accuracy with log compression.

### 2.1 Add Log Compression

Modify `BeatProcessorUpdate()` (`src/beat.cpp:47-61`):

```cpp
// Before flux calculation, apply log compression
// Current:  float mag = spectrum->magnitude[i];
// Proposed: float mag = logf(1.0f + 100.0f * spectrum->magnitude[i]);
```

**Rationale**: Log compression (γ=100) normalizes dynamic range. Quiet onsets become detectable without loud passages saturating the detector. Cited in Böck & Widmer DAFx-13.

### 2.2 Consider Median-Based Threshold

Current threshold: `mean + sensitivity * stddev` (`src/beat.cpp:90`)

Research suggests: `δ + λ·median + α·mean`

**Decision**: TBD. Current approach works well. Profile false positive rate before changing. Median requires sorting history buffer (O(n log n) vs O(n)).

**Files modified**: `src/beat.cpp`

---

## Phase 3: GPU-Driven Spectral Visualization

**Goal**: Pass FFT data to GPU as 1D texture for shader-driven effects.

### 3.1 Create Audio Texture

Add to `src/visualizer.cpp`:

```cpp
#define AUDIO_TEX_WIDTH 512

typedef struct AudioTexture {
    Texture2D texture;      // 512x2, R32 float format
    float data[512 * 2];    // Row 0: FFT magnitude, Row 1: waveform
} AudioTexture;

void AudioTextureUpdate(AudioTexture* at, float* fft, float* wave) {
    // Copy and normalize FFT bins 0-511 to row 0
    // Copy waveform samples to row 1
    UpdateTexture(at->texture, at->data);
}
```

**Texture format**: `PIXELFORMAT_UNCOMPRESSED_R32` (32-bit float per texel). Matches Shadertoy convention: row 0 = spectrum, row 1 = waveform.

### 3.2 Spectral Ring Shader

Create `shaders/spectral_rings.fs`:

```glsl
uniform sampler2D audioTex;
uniform vec2 resolution;
uniform float time;

void main() {
    vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;
    float angle = atan(uv.y, uv.x);
    float dist = length(uv);

    // Map angle to FFT bin
    float normalizedAngle = (angle + 3.14159) / (2.0 * 3.14159);
    float fft = texture(audioTex, vec2(normalizedAngle, 0.0)).r;

    // Draw ring at radius proportional to FFT magnitude
    float ringRadius = 0.3 + fft * 0.4;
    float ring = smoothstep(0.02, 0.0, abs(dist - ringRadius));

    gl_FragColor = vec4(vec3(ring), 1.0);
}
```

### 3.3 Integration

- Create `AudioTexture` in `VisualizerInit()`
- Update texture each frame before drawing
- Bind to shaders via `SetShaderValueTexture()`

**Files modified**: `src/visualizer.cpp`, `src/visualizer.h`
**Files created**: `shaders/spectral_rings.fs`

---

## Phase 4: Enhanced Audio-Reactive Mappings

**Goal**: Map multiple audio features to distinct visual parameters.

### 4.1 Mapping Table

| Audio Feature | Visual Parameter | Location | Mapping |
|---------------|------------------|----------|---------|
| `bassSmooth` | Waveform scale | `waveform.cpp:220` | `scale = base + bass * 0.3` |
| `midSmooth` | Color saturation | `shaders/chromatic.fs` | `sat = 0.5 + mid * 0.5` |
| `trebSmooth` | Bloom intensity | `visualizer.cpp:118` | `bloom = base + treb * 2.0` |
| `beatIntensity` | Chromatic offset | `visualizer.cpp:152` | Existing |
| Spectral centroid | Hue rotation | TBD | `hue = centroid / 5000.0` |

### 4.2 Spectral Centroid Extraction

Add to `src/bands.cpp`:

```cpp
// Spectral centroid = weighted mean of frequencies
// centroid = Σ(f * mag) / Σ(mag)
float ComputeSpectralCentroid(float* magnitude, int binCount, float binHz) {
    float weightedSum = 0.0f;
    float magSum = 0.0f;
    for (int i = 1; i < binCount; i++) {
        float freq = i * binHz;
        weightedSum += freq * magnitude[i];
        magSum += magnitude[i];
    }
    return (magSum > 0.0001f) ? weightedSum / magSum : 0.0f;
}
```

Returns frequency in Hz. Typical range: 500-5000Hz for music. Map to hue (0-360°) with `hue = (centroid - 500) / 12.5` (clamped).

**Files modified**: `src/bands.cpp`, `src/visualizer.cpp`, `shaders/chromatic.fs`

---

## Phase 5: Multi-Pass Bloom (Optional)

**Goal**: Replace 5-tap blur with 6-level downsample chain for wider, higher-quality bloom.

### 5.1 Mip Chain Structure

```
Level 0: 1920x1080 (source)
Level 1: 960x540
Level 2: 480x270
Level 3: 240x135
Level 4: 120x68
Level 5: 60x34
```

Downsample with 13-tap filter (CoD method). Upsample with bilinear + additive blend.

### 5.2 Implementation Complexity

Requires 6 additional render textures and 12 shader passes (6 down + 6 up). Current 5-tap blur runs in 2 passes.

**Decision**: Defer. Current bloom is adequate. Implement only if visual quality becomes a priority.

---

## Implementation Order

| Priority | Phase | Estimated Effort | Dependencies |
|----------|-------|------------------|--------------|
| 1 | 1.1 Band extraction | 2-3 hours | None |
| 2 | 1.2 Attack/release smoothing | 1-2 hours | 1.1 |
| 3 | 2.1 Log compression | 30 minutes | None |
| 4 | 4.2 Spectral centroid | 1 hour | 1.1 |
| 5 | 4.1 Visual mappings | 2-3 hours | 1.2, 4.2 |
| 6 | 3.1-3.3 GPU texture pipeline | 4-6 hours | None |
| 7 | 5.1-5.2 Multi-pass bloom | 6-8 hours | 3.x |

**Total for phases 1-4**: 8-12 hours
**Phase 5 (optional)**: 6-8 hours additional

---

## Validation Criteria

- [ ] `bassSmooth`, `midSmooth`, `trebSmooth` values display in UI
- [ ] Beat detection triggers reliably on kick drums without false positives on sustained bass
- [ ] Waveform scale visibly responds to bass energy
- [ ] Color shifts perceptibly with spectral centroid changes
- [ ] FFT texture renders correctly in spectral ring shader
- [ ] No audio glitches or frame drops during visualization

---

## References

- `docs/research/deep-research1.md` — Source research document
- `docs/architecture.md` — Current system architecture
- Böck & Widmer, "Maximum Filter Vibrato Suppression" (DAFx-13) — SuperFlux algorithm
- MilkDrop preset authoring guide — Band smoothing conventions
