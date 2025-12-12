# Audio/Visual Update Separation

Decouple audio analysis rate from visual refresh rate to fix beat detection timing.

## Goal

The current implementation runs `UpdateWaveformAudio()` at 20Hz (every 50ms), which was
intended to limit visual updates. However, this also throttles FFT analysis and beat
detection to 20Hz. This causes:

1. **Incorrect history sizing**: `BEAT_HISTORY_SIZE = 43` assumes 20Hz updates for ~860ms
   window, but should match actual FFT update rate
2. **Missed transients**: 50ms between analysis frames loses beat information
3. **Choppy analysis**: Audio accumulates during the 50ms gap, causing burst processing

Beat detection research recommends 43-100Hz analysis rates with 75-87.5% overlap. The
current FFT already uses 512-sample hop (75% overlap) at 48kHz = ~94Hz potential rate,
but the 20Hz gating prevents this.

## Current State

Audio analysis is gated by `waveformAccumulator`:

```cpp
// src/main.cpp:224
const float waveformUpdateInterval = 1.0f / 20.0f;

// src/main.cpp:239-242
if (ctx->waveformAccumulator >= waveformUpdateInterval) {
    UpdateWaveformAudio(ctx, waveformUpdateInterval);
    ctx->waveformAccumulator = 0.0f;
}
```

Inside `UpdateWaveformAudio()` (`src/main.cpp:132-192`):
- Drains all accumulated audio (up to 64ms worth)
- Feeds to FFT in a loop until exhausted
- Processes beat detection on each FFT update
- Updates waveform display buffers

The FFT processor (`src/analysis/fft.cpp:110-114`) uses 512-sample hop (75% overlap):
```cpp
const int keep = FFT_SIZE * 3 / 4;  // 1536 samples
const int hop = FFT_SIZE - keep;     // 512 samples
```

At 48kHz, hop of 512 samples = ~94Hz potential FFT update rate. But the 20Hz gating
means FFT only runs ~20 times/second with bursts of multiple FFT frames per update.

Beat detector history (`src/analysis/beat.h:7`):
```cpp
#define BEAT_HISTORY_SIZE 43     // ~860ms rolling average at 20Hz update rate
```

This comment is wrong. At 94Hz FFT rate, 43 samples = ~457ms. At actual 20Hz call rate
with ~4 FFT frames per call, history spans ~215ms (43/4/20 * 4 FFT frames).

## Algorithm

### Decouple Analysis from Visuals

Run audio analysis every frame (60Hz+), limit waveform visuals to 20Hz.

**Analysis rate**: Every frame, drain audio buffer and run FFT/beat detection
**Visual rate**: 20Hz accumulator controls only waveform rendering calculations

### Correct History Sizing

With 94Hz FFT rate, for ~860ms history:
```
BEAT_HISTORY_SIZE = 0.86 * 94 ≈ 81 samples
```

Round to 80 for cleaner math.

| Parameter | Old | New | Rationale |
|-----------|-----|-----|-----------|
| FFT update rate | 20Hz (gated) | ~94Hz (hop-limited) | Match audio sample rate |
| Beat history | 43 samples | 80 samples | ~850ms at 94Hz |
| Waveform visual | 20Hz | 20Hz | No change |

### Delta Time Handling

Currently `deltaTime` passed to `BeatDetectorProcess()` is fixed at 50ms (1/20Hz).
Change to actual frame delta for correct intensity decay timing.

## Integration

### Changes to main.cpp

1. **Split `UpdateWaveformAudio()` into two functions:**
   - `UpdateAudioAnalysis()`: FFT, beat detection, band energies (runs every frame)
   - `UpdateWaveformVisuals()`: Waveform buffer processing (runs at 20Hz)

2. **Main loop changes:**
   ```cpp
   // Every frame: audio analysis
   UpdateAudioAnalysis(ctx, deltaTime);

   // 20Hz: waveform visuals only
   if (ctx->waveformAccumulator >= waveformUpdateInterval) {
       UpdateWaveformVisuals(ctx);
       ctx->waveformAccumulator = 0.0f;
   }
   ```

### Changes to beat.h

1. **Update history size:**
   ```cpp
   #define BEAT_HISTORY_SIZE 80     // ~850ms rolling average at 94Hz FFT rate
   ```

## File Changes

| File | Change |
|------|--------|
| `src/main.cpp` | Split UpdateWaveformAudio into analysis and visual functions |
| `src/analysis/beat.h` | Update BEAT_HISTORY_SIZE from 43 to 80, fix comment |

## Validation

- [ ] Beat detection triggers on transients, not just 20Hz samples
- [ ] Waveform visuals remain at 20Hz (no visual change)
- [ ] Beat intensity decay uses correct delta time
- [ ] History window covers ~850ms regardless of frame rate
- [ ] No increase in CPU usage (same total FFT operations)

## References

- [Choice of Hop Size](https://www.dsprelated.com/freebooks/sasp/Choice_Hop_Size.html) - DSPRelated
- [OBTAIN: Real-Time Beat Tracking](https://arxiv.org/pdf/1704.02216) - 87.5% overlap recommendation
- [FFT Processing in JUCE](https://audiodev.blog/fft-processing/) - Frame rate independence patterns
- [A better FFT-based audio visualization](https://dlbeer.co.nz/articles/fftvis.html) - Smoothing across frames
