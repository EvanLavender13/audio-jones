# Cymatics (True Audio)

Cymatics visualizes sound as interference patterns by placing virtual "speakers" in 2D space and sampling an audio history buffer at distance-based delays. Each pixel receives audio from multiple sources at different times, creating standing wave patterns that directly represent the waveform shape. Drums produce sharp expanding rings; sustained tones create stable interference grids; complex audio yields intricate organic patterns.

## Classification

- **Category**: SIMULATIONS (alongside Physarum, Curl Flow)
- **Core Operation**: Sample audio ring buffer at `delay = distance * waveSpeed` for multiple point sources, sum interference
- **Pipeline Position**: Runs during simulation stage, outputs to accumTexture or dedicated trail map

## References

- [Paul Bourke - Chladni Plate Interference Surfaces](https://paulbourke.net/geometry/chladni/) - Mathematical foundation for nodal patterns, equations for rectangular/circular plates
- [Jai Veilleux - Chladni Plate Particle Simulation](https://didgety.github.io/blog/2023/chladni_plate_simulation/) - GPU particle approach using normal maps for velocity fields
- [ChladniPlate2 GitHub](https://github.com/flutomax/ChladniPlate2) - Parameter ranges: amplitude (-1 to 1), frequency ratio (0.1-20), phase (-360 to 360)
- [Mark Serena - UE Chladni Material](https://www.markserena.com/post/ue_chladni_material/) - Direct UV shader implementation with n/m frequency control
- Shadertoy "Cymatics V1" (user-provided) - Audio buffer delay sampling pattern

## Algorithm

### Core Pattern: Distance-Based Audio Delay

Each pixel samples the audio buffer at an offset proportional to distance from virtual sources:

```glsl
// Sample audio at distance-based delay
float getAudioSample(sampler1D buffer, float sampleOffset, int bufferSize) {
    int idx = int(sampleOffset) % bufferSize;
    return texelFetch(buffer, idx, 0).r * 2.0 - 1.0;  // Unpack from [0,1] to [-1,1]
}

// For each source, calculate delay and fetch sample
float delay = length(uv - sourcePos) * waveSpeed;
float wave = getAudioSample(waveformBuffer, delay, BUFFER_SIZE);
```

### Multi-Source Interference

Place multiple virtual speakers and sum their contributions:

```glsl
// Define virtual speaker positions (normalized coordinates)
vec2 sources[5] = vec2[](
    vec2(0.0, 0.0),      // Center
    vec2(-0.5, 0.0),     // Left
    vec2(0.5, 0.0),      // Right
    vec2(0.0, -0.5),     // Bottom
    vec2(0.0, 0.5)       // Top
);

float totalWave = 0.0;
for (int i = 0; i < 5; i++) {
    float dist = length(uv - sources[i]);
    float delay = dist * waveSpeed;
    float amplitude = 1.0 / (1.0 + dist * falloff);  // Distance attenuation
    totalWave += getAudioSample(buffer, delay) * amplitude;
}
```

### Chladni Equation (Alternative/Hybrid)

For parametric nodal patterns without raw audio:

```glsl
// Classic Chladni equation for rectangular plate
float chladni(vec2 uv, float n, float m, float L) {
    float PI = 3.14159265;
    return cos(n * PI * uv.x / L) * cos(m * PI * uv.y / L)
         - cos(m * PI * uv.x / L) * cos(n * PI * uv.y / L);
}

// Gradient for particle steering or UV displacement
vec2 chladniGradient(vec2 uv, float n, float m, float L) {
    float dx = 0.001;
    float dy = 0.001;
    return vec2(
        (chladni(uv + vec2(dx, 0), n, m, L) - chladni(uv - vec2(dx, 0), n, m, L)) / (2.0 * dx),
        (chladni(uv + vec2(0, dy), n, m, L) - chladni(uv - vec2(0, dy), n, m, L)) / (2.0 * dy)
    );
}
```

### Visualization Enhancement

```glsl
// Contour lines emphasize wave structure
float contours = abs(fract(totalWave * contourCount + 0.5) - 0.5);
float pattern = smoothstep(0.1, 0.2, contours);

// Circular boundary mask
float boundaryMask = 1.0 - smoothstep(boundaryRadius - 0.05, boundaryRadius, length(uv));

// Final intensity with tanh compression
float intensity = tanh(totalWave * visualGain);
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| waveSpeed | float | 1.0 - 50.0 | Propagation speed (samples per unit distance). Higher = tighter rings |
| sourceCount | int | 1 - 8 | Number of virtual speakers |
| sourceSpacing | float | 0.1 - 1.0 | Distance of outer sources from center (normalized) |
| falloff | float | 0.0 - 5.0 | Distance attenuation strength. 0 = no falloff |
| visualGain | float | 0.5 - 5.0 | Output intensity multiplier |
| contourCount | float | 0.0 - 10.0 | Number of contour bands. 0 = smooth gradient |
| boundaryRadius | float | 0.5 - 2.0 | Circular mask radius (normalized) |
| bufferSize | int | 512 - 2048 | Audio history length in samples |

### Chladni Mode Parameters (if hybrid)

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| chladniN | float | 1.0 - 10.0 | X-axis frequency parameter |
| chladniM | float | 1.0 - 10.0 | Y-axis frequency parameter |
| chladniBlend | float | 0.0 - 1.0 | Blend between audio (0) and parametric Chladni (1) |

## Coloring

Uses existing `ColorLUT` pattern from Curl Flow - shader outputs normalized value, samples LUT texture for final color. Completely color-mode agnostic.

### LUT Sampling

```glsl
layout(binding = 3) uniform sampler2D colorLUT;
uniform float value;  // Brightness from ColorConfig

// Map wave amplitude [-1, 1] → [0, 1]
float t = totalWave * 0.5 + 0.5;

// Sample LUT for color (same pattern as curl_flow_agents.glsl:188-190)
vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb * value;
```

### CPU Side

```cpp
// In CymaticsInit:
cymatics->colorLUT = ColorLUTInit(&config->color);

// In CymaticsApplyConfig:
ColorLUTUpdate(cymatics->colorLUT, &newConfig->color);

// In CymaticsUpdate (bind before dispatch):
glActiveTexture(GL_TEXTURE3);
glBindTexture(GL_TEXTURE_2D, ColorLUTGetTexture(cymatics->colorLUT).id);
```

### Mapping Options

| What to map | `t` calculation | Visual effect |
|-------------|-----------------|---------------|
| Wave amplitude | `totalWave * 0.5 + 0.5` | Interference intensity colors peaks/troughs |
| Contour bands | `fract(totalWave * contourCount)` | Discrete color bands at wave levels |
| Distance from center | `length(uv) / boundaryRadius` | Radial color gradient |

## Audio Mapping Ideas

| Parameter | Audio Source | Mapping |
|-----------|--------------|---------|
| waveSpeed | Dominant frequency | Higher pitch = faster propagation (tighter patterns) |
| visualGain | Bass energy | Louder bass = brighter output |
| sourceSpacing | Mid energy | Active mids = wider source spread |
| chladniN/M | FFT peak bins | Map to resonant frequency patterns |

## Implementation Notes

### Audio History Buffer

The existing `AudioCaptureRead` drains frames from the capture ring buffer. Cymatics needs a **persistent sliding window**:

```cpp
// In AnalysisPipeline or new CymaticsState struct
float waveformHistory[CYMATICS_BUFFER_SIZE];  // e.g., 2048 samples
int waveformWriteIndex = 0;

// After AudioCaptureRead, append mono-mixed samples:
for (uint32_t i = 0; i < framesRead; i++) {
    float mono = (audioBuffer[i * 2] + audioBuffer[i * 2 + 1]) * 0.5f;
    waveformHistory[waveformWriteIndex] = mono * 0.5f + 0.5f;  // Pack to [0,1]
    waveformWriteIndex = (waveformWriteIndex + 1) % CYMATICS_BUFFER_SIZE;
}
```

### Texture Upload

Follow `fftTexture` pattern in `post_effect.cpp`:

```cpp
// Create 1D texture (width = buffer size, height = 1)
Image waveformImage = { .data = NULL, .width = CYMATICS_BUFFER_SIZE, .height = 1,
                        .format = PIXELFORMAT_UNCOMPRESSED_R32, .mipmaps = 1 };
Texture2D waveformTexture = LoadTextureFromImage(waveformImage);

// Each frame, upload history buffer
UpdateTexture(waveformTexture, waveformHistory);
```

### Shader Sampling

The shader must handle ring buffer wrap-around. Pass `writeIndex` as uniform so shader knows current "now" position:

```glsl
uniform sampler1D waveformBuffer;
uniform int bufferSize;
uniform int writeIndex;  // Current write position = "now"

float sampleAudio(float delay) {
    int idx = (writeIndex - int(delay) + bufferSize) % bufferSize;
    return texelFetch(waveformBuffer, idx, 0).r * 2.0 - 1.0;
}
```

### Output Options

1. **Direct render**: Output interference pattern as color/luminance to accumTexture
2. **Trail map**: Use interference value to modulate deposit (like Physarum trails)
3. **UV displacement**: Use gradient of interference field to warp existing content (transform mode)

### Simulation Infrastructure (Matching Existing Patterns)

Cymatics should follow the same patterns as Curl Flow, Physarum, etc.:

**TrailMap** (optional - depends on desired visual):
- If cymatics renders fresh each frame: No TrailMap needed, output directly
- If trails/persistence desired: Use TrailMap with diffusion/decay like other sims

**BlendCompositor** for output-stage compositing:
```cpp
// In RenderPipelineApplyOutput (render_pipeline.cpp pattern)
if (pe->cymatics != NULL && pe->blendCompositor != NULL &&
    pe->effects.cymatics.enabled && pe->effects.cymatics.boostIntensity > 0.0f) {
    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blendCompositor->shader, SetupCymaticsBoost);
    // ...
}

// SetupCymaticsBoost in shader_setup.cpp
void SetupCymaticsBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         CymaticsGetOutputTexture(pe->cymatics),
                         pe->effects.cymatics.boostIntensity,
                         pe->effects.cymatics.blendMode);
}
```

**Config parameters** (matching other sims):
```cpp
struct CymaticsConfig {
    bool enabled = false;
    float boostIntensity = 1.0f;           // Blend strength in output stage
    EffectBlendMode blendMode = EFFECT_BLEND_BOOST;
    bool debugOverlay = false;             // Draw directly to accumTexture
    ColorConfig color;
    // ... cymatics-specific params
};
```

**Lifecycle functions** (standard pattern):
- `CymaticsInit(width, height, config)` - create textures, load shaders
- `CymaticsUninit(cymatics)` - cleanup
- `CymaticsUpdate(cymatics, deltaTime, waveformBuffer)` - run compute/render
- `CymaticsResize(cymatics, width, height)` - handle window resize
- `CymaticsReset(cymatics)` - clear state
- `CymaticsApplyConfig(cymatics, newConfig)` - hot-reload config
- `CymaticsDrawDebug(cymatics)` - debug visualization

**PostEffect integration**:
```cpp
// In PostEffect struct (post_effect.h)
Cymatics* cymatics;

// In PostEffectInit
pe->cymatics = CymaticsInit(screenWidth, screenHeight, &pe->effects.cymatics);

// ApplyCymaticsPass follows same pattern as ApplyCurlFlowPass
```

### Architecture Decision: TrailMap vs Direct Render

**Option A: No TrailMap (fresh each frame)**
- Shader computes interference pattern directly to output texture
- No persistence between frames - pattern updates instantly with audio
- Simpler implementation, lower memory
- Good for: reactive, real-time visualization

**Option B: With TrailMap (persistent trails)**
- Compute shader deposits to TrailMap like Physarum/Curl Flow
- Diffusion spreads patterns, decay creates trails
- Patterns persist and blend over time
- Good for: organic, flowing visualization

Recommendation: Start with **Option A** (direct render) for true real-time audio reactivity. Add TrailMap later if persistence is desired.

### Performance

- Buffer size 2048 at 48kHz = ~43ms of audio history
- 5 sources = 5 texture fetches per pixel (acceptable)
- Contour calculation adds ~2 ALU ops per pixel
- Main cost: texture bandwidth for waveform sampling
