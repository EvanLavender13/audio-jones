# Architecture Roadmap

## Current State

- `audio.c/h` - miniaudio loopback capture
- `waveform.c/h` - Drawing functions, HsvToRgb, geometry
- `visualizer.c/h` - RenderTexture, fade shader, accumulation engine
- `main.c` - Init, loop, glue

## Feature Parity with AudioThing

### Enhanced Shader

Current `fade.fs` only does simple fade. AudioThing's `fade_blur.frag` includes:

```glsl
// Effect chain (single pass):
1. Gaussian blur (5×5 kernel)
2. Pixelation (8×8 jittered anti-aliased sampling)
3. Blend (mix blur and pixelation by factor)
4. Fade (exponential decay: color *= pow(fadeFactor, 1.5))
5. Saturation boost (HSV manipulation to preserve vividness)
6. Dithering (random noise prevents color banding)
7. Threshold (black out pixels below luminance cutoff)
```

Shader uniforms to expose:
- `fadeFactor` (0.95-0.99)
- `pixelSize` (1-16)
- `blendFactor` (0.0-1.0)
- `fadeThreshold` (0.0-0.1)
- `saturationBoost` (1.0-2.0)
- `ditherStrength` (0.0-0.05)

### Multiple Waveforms

Support N independent waveforms, each with:

```c
typedef struct {
    float displayHeight;    // amplitude multiplier
    float smoothness;       // sliding window size (1-50)
    float rotationSpeed;    // radians per update
    float radiusFactor;     // base radius multiplier
    float thickness;        // line width
    float hueOffset;        // color shift
    float alpha;            // thin line opacity
    float thickAlpha;       // thick line opacity
    bool enabled;
} WaveformConfig;
```

Dual rendering per waveform:
- Thin line: raw samples, lower alpha, `DrawLineStrip()`
- Thick line: smoothed samples, triangle strip mesh via `rlgl`

**Triangle strip for thick lines** (instead of multiple parallel strokes):
```c
// For each point, compute perpendicular offset from line direction
Vector2 dir = Vector2Normalize(Vector2Subtract(points[i+1], points[i]));
Vector2 perp = { -dir.y * halfWidth, dir.x * halfWidth };

vertices[i*2]     = Vector2Add(points[i], perp);      // outer edge
vertices[i*2 + 1] = Vector2Subtract(points[i], perp); // inner edge
```
Draw with `rlBegin(RL_TRIANGLE_STRIP)` - single draw call, proper line joints.

### Sliding Window Smoothing

O(N) smoothing algorithm per waveform:

```c
// Window size = 2 * smoothness + 1
float windowSum = 0;
for (int i = 0; i < windowSize; i++) windowSum += samples[i];

for (int i = smoothness; i < count - smoothness; i++) {
    smoothed[i] = windowSum / windowSize;
    windowSum -= samples[i - smoothness];
    windowSum += samples[i + smoothness + 1];
}
```

### Configuration GUI

raygui panels (toggle with Tab key):

| Panel | Controls |
|-------|----------|
| **Shader** | fadeFactor, pixelSize, blendFactor, fadeThreshold, saturationBoost, ditherStrength |
| **Waveforms** | List with add/remove, select active |
| **Waveform Settings** | All WaveformConfig fields for selected waveform |
| **Presets** | Save, Load, Delete buttons + name field |

### Preset System

JSON format for save/load:

```json
{
  "name": "PresetName",
  "shader": {
    "fadeFactor": 0.97,
    "pixelSize": 4,
    ...
  },
  "waveforms": [
    { "displayHeight": 250, "smoothness": 10, ... },
    { "displayHeight": 150, "smoothness": 30, ... }
  ]
}
```

Store in `presets/` directory. Use cJSON (single header) for parsing.

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
