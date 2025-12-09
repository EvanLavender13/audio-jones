# Real-time audio visualization architecture for C++20/raylib projects

**Your AudioJones pipeline can achieve sub-15ms latency** by combining a lock-free producer-consumer architecture with spectral flux beat detection and GPU-accelerated radial visualizations. The optimal configuration uses **1024-sample FFT windows with 50% overlap**, a Hann window function, and separate threads for audio capture, FFT analysis, and rendering—the same architecture powering professional tools like MilkDrop, Resolume, and TouchDesigner.

This report synthesizes research on system architecture, beat detection algorithms, audio-reactive visual patterns, and practical implementation techniques specifically tailored for a WASAPI loopback + miniaudio + raylib stack.

## Producer-consumer architecture dominates professional implementations

The canonical pattern across MilkDrop, Resolume Arena, and TouchDesigner separates concerns into three decoupled threads communicating via lock-free data structures:

```
[Audio Capture Thread] → [Lock-Free SPSC Queue] → [Analysis Thread] → [Triple Buffer] → [Render Thread]
     (WASAPI loopback)         (ring buffer)           (FFT/beat)        (atomic swap)      (OpenGL)
```

**Critical constraint**: The audio thread must never block. Ross Bencina's foundational work on real-time audio programming establishes that `malloc()`, `std::mutex::lock()`, file I/O, and any unbounded-time operations are forbidden in audio callbacks. Even `mutex::unlock()` isn't real-time safe on all platforms.

For single-value communication (current beat state, peak levels), use `std::atomic<T>`. For streaming FFT data, implement a lock-free single-producer single-consumer (SPSC) queue—PortAudio's ring buffer implementation provides a battle-tested reference. For state snapshots consumed by the render thread, the **triple-buffer pattern** works well: one buffer being written, one "latest complete" buffer available atomically, one being read.

**WASAPI loopback specifics**: Minimum achievable latency is approximately **10ms** (the Windows audio engine period). Buffer sizes of 128-480 samples (2.66-10ms at 48kHz) are supported on Windows 10+ with HDAudio drivers. Loopback operates only in shared mode (`AUDCLNT_SHAREMODE_SHARED`) and requires handling `AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY` when audio playback stops and restarts.

## MilkDrop and TouchDesigner reveal proven mapping strategies

MilkDrop presets expose three smoothed frequency bands—`bass`, `mid`, `treb`—plus their attenuated versions (`bass_att`, `mid_att`, `treb_att`) for temporal smoothing. Values center around **1.0** (normal), with <0.7 indicating quiet and >1.3 indicating loud passages. Beat detection uses simple thresholds: `if bass > 1.5` triggers kick events.

The preset system flows through four stages: **init code** (once per preset load), **per-frame equations** (global parameters like zoom, rotation, warp), **per-vertex equations** (spatial warping via polar coordinates), and **pixel shaders** (per-pixel effects). Q-variables (q1-q32) bridge equation pools to shaders, enabling complex audio-reactive behaviors.

TouchDesigner uses CHOPs (Channel Operators) with a recommended chain:
```
AudioDeviceIn → AudioBandEQ (low/mid/high) → Envelope → Lag → Math → Visual Parameters
```
The **Lag CHOP** is essential—different lag values create distinct character: **0.1s** for transients, **0.5s** for bass breathing effects.

## Spectral flux offers the best latency-accuracy tradeoff for real-time visualization

| Algorithm | Latency | Accuracy | Complexity | Best Application |
|-----------|---------|----------|------------|------------------|
| Energy-based | Very low | Low | O(N) | Simple triggering |
| High Frequency Content (HFC) | Low | Medium (percussive) | O(N) | Drums, EDM |
| **Spectral Flux** | Low-medium | Good | O(N log N) | **General purpose** |
| SuperFlux | Medium | Very good | O(N log N) | Vibrato-heavy music |
| Complex Domain | Medium | Very good | O(N log N) | Polyphonic content |
| RNN/LSTM | High | Excellent | O(N²) | Offline analysis |

**Spectral flux** computes the half-wave rectified difference between consecutive magnitude spectra:

```
SF(n) = Σ_{k=0}^{K} H(|X(n,k)| - |X(n-1,k)|)
```

where `H(x) = max(0, x)` (half-wave rectifier). Log compression improves onset detection: `Y = log(1 + γ · |X|)` with **γ = 100** providing good sensitivity.

**SuperFlux** enhances this by adding a maximum filter for vibrato suppression, reducing false positives by up to 60% on music with vibrato. The key parameters from Böck & Widmer's DAFx-13 paper: `lag = 2` frames, `max_bins = 3` frequency bins for the maximum filter, and a **mel-scale spectrogram** with 138 bands.

For adaptive thresholding, use:
```
threshold(n) = δ + λ · median(ODF[n-w:n+w]) + α · mean(ODF[n-w:n+w])
```
with a window of approximately **100ms** on each side.

### FFT parameters for different use cases

| FFT Size | Frequency Resolution (44.1kHz) | Time Resolution | Best For |
|----------|-------------------------------|-----------------|----------|
| 512 | 86 Hz | 11.6 ms | Fast transients, gaming |
| **1024** | 43 Hz | 23.2 ms | **General visualization** |
| 2048 | 21 Hz | 46.4 ms | Better bass resolution |
| 4096 | 11 Hz | 92.8 ms | Music analysis |

**Recommended default**: 1024 samples, **Hann window**, **50% overlap** (hop size 512). The Hann window offers -32dB sidelobe suppression with reasonable 2-bin main lobe width—a good balance between Hamming's better sidelobes and rectangular's temporal precision.

Use **mel-scale** frequency binning (32-64 bands) for perceptually uniform spacing, linear below 1kHz and logarithmic above: `m = 1127 · ln(1 + f/700)`.

## Audio feature mapping determines visual impact

The mapping between audio features and visual parameters follows established perceptual principles:

| Audio Feature | Recommended Visual Properties | Mapping Approach |
|---------------|------------------------------|------------------|
| Bass energy | Scale, zoom, blur, gravity | Heavy smoothing (α = 0.05) |
| Bass transients | Flash, pulse, particle burst | Onset detection + fast decay |
| Mid-range | Color saturation, rotation speed | Moderate smoothing |
| Treble energy | Brightness, grain, sparkle | Fast response (α = 0.3) |
| Spectral centroid | Hue (pitch-to-color) | Linear/logarithmic mapping |
| Spectral spread | Complexity, texture density | Normalized 0-1 |
| RMS volume | Master intensity, opacity | Logarithmic (dB scale) |

**Envelope follower** with separate attack/release rates prevents jittery visualization:
```cpp
if (raw > smoothed) alpha = attack_rate;  // 0.3 = fast attack
else alpha = release_rate;                 // 0.05 = slow release
smoothed = lerp(smoothed, raw, alpha);
```

### Physarum, reaction-diffusion, and metaballs for organic visuals

**Physarum simulation** (based on Jeff Jones's multi-agent model): Particles carry position and heading, sample three forward sensors for chemical concentration, rotate toward the highest concentration, and deposit chemicals. A trail map with box-blur diffusion and per-frame decay creates organic flowing patterns. Map `decay_factor ← bass_smoothed` for bass-reactive trail persistence.

**Reaction-diffusion** (Gray-Scott model):
```
A_new = A + (dA · ∇²A - A·B² + f·(1-A)) · dt
B_new = B + (dB · ∇²B + A·B² - (k+f)·B) · dt
```
Parameters `f` (feed rate, 0.01-0.1) and `k` (kill rate, 0.045-0.07) control pattern type: mitosis (f≈0.0545, k≈0.062), coral growth (f≈0.06, k≈0.0625), or traveling waves (f≈0.014, k≈0.045). Map `f ← bass` for growth speed, `k ← treble` for pattern dissolution.

**Metaballs** use inverse-distance fields with smooth minimum blending:
```glsl
float smin(float a, float b, float k) {
    float h = max(k - abs(a-b), 0.0) / k;
    return min(a, b) - h*h*k*0.25;
}
```
Map ball radii to frequency band energies and blend smoothness to overall energy level.

## GPU shader implementation for raylib follows Shadertoy patterns

**Pass FFT data as a 1D texture** rather than uniform arrays—this scales to 512+ frequency bins without hitting uniform limits:

```cpp
// C++20 / raylib
const int FFT_SIZE = 512;
float fftData[FFT_SIZE * 2];  // Row 0: FFT, Row 1: Waveform

Image fftImage = {
    .data = fftData,
    .width = FFT_SIZE,
    .height = 2,
    .format = PIXELFORMAT_UNCOMPRESSED_R32,
    .mipmaps = 1
};
Texture2D audioTexture = LoadTextureFromImage(fftImage);

// Update each frame (no texture recreation)
UpdateTexture(audioTexture, fftData);
SetShaderValueTexture(shader, GetShaderLocation(shader, "audioTex"), audioTexture);
```

**Radial waveform shader**:
```glsl
uniform sampler2D audioTex;
uniform vec2 resolution;

void main() {
    vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;
    float angle = atan(uv.y, uv.x);
    float dist = length(uv);
    
    float normalizedAngle = (angle + 3.14159) / (2.0 * 3.14159);
    float fft = texture(audioTex, vec2(normalizedAngle, 0.0)).r;
    
    float waveRadius = 0.3 + fft * 0.2;
    float ring = smoothstep(0.02, 0.0, abs(dist - waveRadius));
    ring += smoothstep(0.08, 0.0, abs(dist - waveRadius)) * 0.3;  // Glow
    
    gl_FragColor = vec4(vec3(ring), 1.0);
}
```

**Feedback trails** use ping-pong render textures:
```cpp
RenderTexture2D feedback[2];
int current = 0;

BeginTextureMode(feedback[current]);
    DrawTextureRec(feedback[1-current].texture, sourceRect, origin, 
        (Color){255, 255, 255, 245});  // 96% opacity = trails
    DrawNewParticles();
EndTextureMode();

current = 1 - current;  // Swap
```

**Efficient bloom** uses a downsample-upsample chain (6 mip levels) rather than large-kernel blur. The 13-tap filter from Call of Duty's presentation provides quality results at each downsample stage.

## Prioritized implementation roadmap maximizes impact

| Phase | Feature | Complexity | Visual Impact | Implementation Time |
|-------|---------|------------|---------------|---------------------|
| **1 (MVP)** | FFT texture binding | 2/5 | Foundation | 2-4 hours |
| | Radial waveform | 2/5 | 5/5 | 4-6 hours |
| | Audio-reactive vignette | 1/5 | 3/5 | 1-2 hours |
| **2 (Core)** | Feedback buffer trails | 3/5 | 5/5 | 4-6 hours |
| | Spectral rings (multi-band) | 3/5 | 4/5 | 3-4 hours |
| | Simple one-pass bloom | 2/5 | 4/5 | 2-3 hours |
| **3 (Polish)** | Color grading from audio | 2/5 | 3/5 | 2-3 hours |
| | Scanlines/CRT effect | 1/5 | 3/5 | 1-2 hours |
| **4 (Advanced)** | Multi-pass bloom chain | 4/5 | 5/5 | 6-8 hours |
| | Particle systems | 4/5 | 5/5 | 8-12 hours |
| | Physarum/reaction-diffusion | 4/5 | 5/5 | 8-12 hours |

**Photosensitive safety**: Limit flashes to **maximum 3 per second**, especially red flashes. Keep flash area under 25% of screen. Use `smoothstep()` for all transitions rather than hard cuts.

## Key references and implementations

**Architecture and threading**: Ross Bencina's "Real-time Audio Programming 101"; Timur Doumler's CppCon talks and timur.audio articles on lock-free audio; Microsoft WASAPI documentation.

**Beat detection algorithms**: Bello et al., "A Tutorial on Onset Detection in Music Signals" (IEEE 2005); Böck & Widmer, "Maximum Filter Vibrato Suppression for Onset Detection" (DAFx-13); Sebastian Böck's madmom library.

**Open-source implementations**: **aubio** (C/Python, GPL-3.0)—best for real-time with low latency; **BTrack** (C++, GPL-3.0)—~0.15ms per 1024-sample frame; **essentia** (C++/Python, AGPL-3.0)—comprehensive including SuperFlux; **projectM** (C++, LGPL)—cross-platform MilkDrop implementation; **GLava** (C, GPL-3.0)—professional OpenGL visualizer with modular GLSL.

**Shader techniques**: Inigo Quilez's signed distance function articles (iquilezles.org); LearnOpenGL's physically-based bloom tutorial; Ronja's polar coordinate shader tutorials; the Shadertoy audio FFT texture specification (512×2, 8-bit normalized, row 0 = spectrum, row 1 = waveform).

**Visual patterns**: MilkDrop preset authoring guide (geisswerks.com); reaction-diffusion-playground (Jason Webb, GitHub); TouchDesigner audio-reactive tutorials (derivative.ca); physarum implementations by nicoptere (GitHub).

## Conclusion

The AudioJones architecture should follow the established producer-consumer pattern with lock-free SPSC queues between audio capture and FFT analysis, and triple-buffered state for the render thread. **Spectral flux with mel-scale binning** provides the best balance of latency and accuracy for real-time beat detection, while the **radial waveform with feedback trails** delivers maximum visual impact for minimal implementation complexity.

Start with Phase 1 features to establish the pipeline, then iterate. The combination of 1024-sample FFT windows, 50% overlap, Hann windowing, and adaptive thresholding will produce responsive, musically coherent visualizations. Pass FFT data as textures rather than uniforms, use envelope followers with separate attack/release rates, and maintain photosensitive safety with smooth transitions and controlled flash rates.