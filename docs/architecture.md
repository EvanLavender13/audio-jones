# AudioJones Architecture

> Auto-generated via `/sync-architecture`. Last sync: 2025-12-05

## Overview

Real-time audio visualizer that captures system audio via WASAPI loopback and renders circular or linear waveforms with physarum-inspired trail effects. Supports up to 8 concurrent waveforms with per-waveform configuration and preset save/load.

## System Diagram

```mermaid
flowchart TD
    subgraph AudioThread[Audio Thread]
        WASAPI[WASAPI Loopback] -->|f32 stereo 48kHz| CB[audio_data_callback]
        CB -->|write frames| RB[(ma_pcm_rb<br/>4096 frames)]
    end

    subgraph MainLoop[Main Thread]
        RB -->|lock-free read| READ[AudioCaptureRead]
        READ -->|f32 x 2048| BASE[ProcessWaveformBase]
        BASE -->|f32 x 1024 normalized| SMOOTH[ProcessWaveformSmooth x8]
        SMOOTH -->|f32 x 2048 palindrome| EXT[(waveformExtended)]
    end

    subgraph RenderPipe[Render Pipeline 60fps]
        EXT -->|samples per waveform| DRAW[DrawWaveformCircular]
        CFG[WaveformConfig] -->|color, radius, thickness| DRAW
        DRAW -->|line segments| ACC[(accumTexture)]
        ACC -->|sample texture| BH[blur_h.fs]
        BH -->|H blurred| TMP[(tempTexture)]
        TMP -->|sample texture| BV[blur_v.fs]
        HL[halfLife] -->|decay rate| BV
        BV -->|V blurred and decayed| ACC
        ACC -->|blit| SCR[Screen]
    end

    subgraph UILayer[UI System]
        PNL[raygui panels] -->|slider values| CFG
        PNL -->|slider value| HL
        PNL -->|PresetSave| DISK[(presets folder)]
        DISK -->|PresetLoad| CFG
        DISK -->|PresetLoad| HL
    end
```

**Legend:** Arrows show data flow with payload type. `[(name)]` = persistent buffer. `[name]` = processing step.

## Modules

### audio.c / audio.h

Captures system audio via miniaudio WASAPI loopback device.

| Function | Purpose |
|----------|---------|
| `audio_data_callback` | Writes incoming stereo samples to ring buffer (audio thread) |
| `AudioCaptureInit` | Creates loopback device at 48kHz, allocates 4096-frame ring buffer |
| `AudioCaptureStart` | Starts the loopback device |
| `AudioCaptureStop` | Stops the loopback device |
| `AudioCaptureUninit` | Frees device and ring buffer |
| `AudioCaptureRead` | Reads up to N frames from ring buffer (main thread, lock-free) |
| `AudioCaptureAvailable` | Returns frame count available in ring buffer |

**Constants:**
- `AUDIO_SAMPLE_RATE`: 48000 Hz
- `AUDIO_CHANNELS`: 2 (stereo)
- `AUDIO_BUFFER_FRAMES`: 1024 frames per read
- `AUDIO_RING_BUFFER_FRAMES`: 4096 frames capacity

### waveform.c / waveform.h

Transforms raw audio samples into display-ready waveform data and renders as circular or linear visualizations.

| Function | Purpose |
|----------|---------|
| `WaveformConfigDefault` | Returns default config: radius=0.25, thickness=2, smoothness=5 |
| `ProcessWaveformBase` | Copies samples, zero-pads if short, normalizes to peak=1.0 |
| `ProcessWaveformSmooth` | Creates palindrome, applies O(N) sliding window smoothing |
| `SmoothWaveform` | Static helper: sliding window moving average |
| `CubicInterp` | Static helper: cubic interpolation between 4 points |
| `DrawWaveformLinear` | Renders horizontal oscilloscope-style waveform |
| `DrawWaveformCircular` | Renders circular waveform with cubic interpolation |

**WaveformConfig fields:**
- `amplitudeScale`: Height relative to min(width, height), range 0.05-0.5, default 0.35
- `thickness`: Line width in pixels, range 1-10, default 2
- `smoothness`: Smoothing window radius, range 0-50, default 5
- `radius`: Base radius fraction, range 0.05-0.45, default 0.25
- `rotationSpeed`: Radians per update, range -0.05 to 0.05, default 0.01
- `rotation`: Current angle in radians
- `color`: RGBA color, default WHITE

**Constants:**
- `WAVEFORM_SAMPLES`: 1024
- `WAVEFORM_EXTENDED`: 2048 (palindrome for seamless loop)
- `INTERPOLATION_MULT`: 1 (points per sample)
- `MAX_WAVEFORMS`: 8

### visualizer.c / visualizer.h

Manages accumulation buffer and two-pass separable blur for physarum-style trail diffusion.

| Function | Purpose |
|----------|---------|
| `VisualizerInit` | Loads blur shaders, creates ping-pong RenderTextures |
| `VisualizerUninit` | Frees shaders and render textures |
| `VisualizerResize` | Recreates render textures at new dimensions |
| `VisualizerBeginAccum` | Runs horizontal blur, then vertical blur + decay |
| `VisualizerEndAccum` | Ends texture mode |
| `VisualizerToScreen` | Blits accumulation texture to screen |

**Visualizer struct fields:**
- `accumTexture`: Main accumulation buffer
- `tempTexture`: Intermediate buffer for separable blur
- `blurHShader`, `blurVShader`: Separable Gaussian blur shaders
- `halfLife`: Trail persistence in seconds (0.1-2.0), default 0.5

### ui.c / ui.h

Immediate-mode UI panels using raygui with auto-stacking layout.

| Function | Purpose |
|----------|---------|
| `UILayoutBegin` | Starts layout context at position |
| `UILayoutRow` | Advances to next row with given height |
| `UILayoutSlot` | Returns Rectangle for slot width as fraction |
| `UILayoutEnd` | Returns final Y position |
| `UIStateInit` | Allocates UIState, loads preset file list |
| `UIStateUninit` | Frees UIState |
| `UIBeginPanels` | Sets starting Y for auto-stacking panels |
| `UIDrawWaveformPanel` | Renders waveform list, New button, per-waveform sliders, trails slider |
| `UIDrawPresetPanel` | Renders preset name input, Save button, preset list with auto-load |

**UIState fields (opaque struct):**
- `panelY`: Current Y position for panel stacking
- `waveformScrollIndex`: Waveform list scroll state
- `presetFiles[32]`: Cached preset filenames
- `presetFileCount`, `selectedPreset`, `presetScrollIndex`: Preset list state
- `presetName[64]`, `presetNameEditMode`: Text input state
- `prevSelectedPreset`: Tracks selection changes for auto-load

**Layout constants:**
- Panel width: 180px
- Group spacing: 8px
- Row height: 20px
- Color picker: 62×62px (reserves 24px for hue bar)

### preset.cpp / preset.h

Serializes visualizer configurations as JSON files using nlohmann/json.

| Function | Purpose |
|----------|---------|
| `PresetDefault` | Returns default Preset with one waveform |
| `PresetSave` | Writes Preset to JSON file, returns false on failure |
| `PresetLoad` | Reads Preset from JSON file, returns false on failure |
| `PresetListFiles` | Enumerates `presets/` directory, returns up to 32 filenames |

**Preset struct fields:**
- `name[64]`: Display name
- `halfLife`: Trail decay parameter
- `waveforms[8]`: WaveformConfig array
- `waveformCount`: Active waveform count (1-8)

**Implementation notes:**
- C++ with `extern "C"` wrapper for C compatibility
- Uses `<filesystem>` for directory enumeration
- Creates `presets/` directory if missing

### main.c

Application entry point. Consolidates runtime state in `AppContext` and coordinates module lifecycle.

| Function | Purpose |
|----------|---------|
| `AppContextInit` | Allocates context, initializes Visualizer/AudioCapture/UIState in order |
| `AppContextUninit` | Frees resources in reverse order, NULL-safe |
| `UpdateWaveformAudio` | Reads audio, calls ProcessWaveformBase once, ProcessWaveformSmooth per waveform, advances rotation |
| `RenderWaveforms` | Dispatches to DrawWaveformLinear or DrawWaveformCircular based on mode |
| `main` | Creates window, runs 60fps loop with 20fps waveform updates |

**AppContext struct fields:**
- `vis`: Visualizer instance
- `capture`: AudioCapture instance
- `ui`: UIState instance
- `audioBuffer[2048]`: Raw samples from ring buffer
- `waveform[1024]`: Base normalized waveform
- `waveformExtended[8][2048]`: Per-waveform smoothed palindromes
- `waveforms[8]`: Per-waveform configuration
- `waveformCount`, `selectedWaveform`: Active waveform tracking
- `mode`: WAVEFORM_LINEAR or WAVEFORM_CIRCULAR
- `waveformAccumulator`: Fixed-timestep accumulator for 20fps updates

## Data Flow

1. **Audio Callback** (`audio.c`): miniaudio triggers callback with system audio at 48kHz stereo
2. **Ring Buffer Write** (`audio.c`): Callback writes to `ma_pcm_rb` (lock-free)
3. **Ring Buffer Read** (`main.c`): Main loop reads up to 1024 frames every 50ms (20fps)
4. **Base Processing** (`waveform.c`): Normalizes samples to peak=1.0, zero-pads if fewer frames
5. **Per-Waveform Processing** (`waveform.c`): Creates palindrome, applies smoothing window
6. **Rotation Update** (`main.c`): Advances each waveform's rotation by its rotationSpeed
7. **Interpolation** (`waveform.c`): Cubic interpolation for smooth curves during render
8. **Draw** (`waveform.c`): Renders line segments with per-waveform color
9. **Blur Pass 1** (`visualizer.c`): Horizontal 5-tap Gaussian blur
10. **Blur Pass 2 + Decay** (`visualizer.c`): Vertical blur + exponential decay based on halfLife
11. **Composite** (`visualizer.c`): New waveform drawn on blurred background
12. **Display** (`main.c`): Accumulated texture blitted to screen
13. **UI Overlay** (`ui.c`): Panels modify WaveformConfig array and halfLife

## Shaders

Separable Gaussian blur with physarum-style diffusion:

| Shader | Purpose |
|--------|---------|
| `shaders/blur_h.fs` | Horizontal 5-tap Gaussian `[1,4,6,4,1]/16` |
| `shaders/blur_v.fs` | Vertical 5-tap Gaussian + framerate-independent exponential decay |

**Decay formula (blur_v.fs):**
```glsl
float decayMultiplier = exp(-0.693147 * deltaTime / halfLife);
```
Includes 0.008 floor subtraction to overcome 8-bit quantization artifacts.

**Ping-pong pipeline:**
```
accumTexture --[blur_h]--> tempTexture --[blur_v + decay]--> accumTexture
```

## Thread Model

```
┌─────────────────────────────────┐
│ Audio Thread (miniaudio)        │
│ - audio_data_callback           │
│ - Writes to ring buffer         │
└──────────────┬──────────────────┘
               │ ma_pcm_rb (lock-free)
               ▼
┌─────────────────────────────────┐
│ Main Thread (raylib)            │
│ - Reads from ring buffer        │
│ - Updates waveform @ 20fps      │
│ - Renders @ 60fps               │
│ - Handles UI input              │
└─────────────────────────────────┘
```

## Configuration

| Parameter | Value | Location |
|-----------|-------|----------|
| Window size | 1920×1080 (resizable) | `main.c` |
| Render FPS | 60 | `main.c` |
| Waveform update rate | 20fps (50ms interval) | `main.c` |
| Max waveforms | 8 | `waveform.h` |
| Trail half-life | 0.1-2.0s (UI slider) | `visualizer.c` |
| Blur kernel | 5-tap Gaussian [1,4,6,4,1]/16 | `blur_h.fs`, `blur_v.fs` |
| Rotation speed range | -0.05 to 0.05 rad/update | `ui.c` |
| Preset directory | `presets/` | `preset.cpp` |
| Max preset files | 32 | `preset.h` |
| UI panel width | 180px | `ui.c` |

---

*Run `/sync-architecture` to regenerate this document from current code.*
