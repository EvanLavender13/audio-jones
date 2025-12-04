# Architecture Roadmap

## Current State

- `audio.c/h` - miniaudio loopback capture
- `main.c` - everything else (visualization, rendering, processing)

## Near-term Refactor

Split main.c into:
- `waveform.c/h` - Drawing functions, HsvToRgb, geometry
- `visualizer.c/h` - RenderTexture, fade shader, accumulation engine
- `main.c` - Init, loop, glue

## Audio Pipeline

```c
typedef void (*AudioProcessor)(float* samples, int count, void* config);

typedef struct {
    AudioProcessor process;
    void* config;
    bool enabled;
} PipelineStage;
```

Stages flow left-to-right, can reorder/toggle via UI.

Planned processors:
- Normalization (have)
- Smoothing (sliding window)
- Lowpass / Highpass / Bandpass (biquad)
- Envelope follower
- FFT (frequency domain viz)

## Synth Components

Future: AudioJones as a playable synthesizer + visualizer.

**Oscillators** (audio sources)
- Sine, saw, square, triangle
- Phase accumulator + waveshape function

**ADSR Envelope**
- Attack, Decay, Sustain, Release
- State machine with ramps

**Signal flow**
```
Sources (loopback | oscillators | file)
    ↓
  Mixer
    ↓
Pipeline [ADSR] → [Filter] → [Effects]
    ↓
 ┌──┴──┐
 ↓     ↓
Out   Viz
```

miniaudio handles both capture and playback - synth output can play through speakers while feeding visualizer.

## Thread Boundaries

1. **Audio callback** - Ring buffer only, no allocations
2. **Main thread processing** - Pipeline transforms, safe to allocate
3. **Rendering** - Consumes processed samples, owns GPU resources

## Guiding Principles

- Keep it simple C
- Flat arrays before fancy data structures
- Let design emerge as features are added
- Each piece should be useful standalone
