# AudioJones Architecture

> Auto-generated via `/sync-architecture`. Last sync: 2025-12-06

## Overview

Real-time audio visualizer that captures system audio via WASAPI loopback and renders circular or linear waveforms with physarum-inspired trail effects. Features energy-based beat detection driving bloom pulse effects. Supports up to 8 concurrent waveforms with per-waveform configuration and preset save/load.

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
        READ -->|f32 x 2048| BD[BeatDetectorProcess]
        BASE -->|f32 x 1024 normalized| SMOOTH[ProcessWaveformSmooth x8]
        SMOOTH -->|f32 x 2048 palindrome| EXT[(waveformExtended)]
        BD -->|beatIntensity 0-1| BI[(beatIntensity)]
    end

    subgraph RenderPipe[Render Pipeline 60fps]
        EXT -->|samples per waveform| DRAW[DrawWaveformCircular]
        CFG[WaveformConfig] -->|color, radius, thickness| DRAW
        DRAW -->|line segments| ACC[(accumTexture)]
        BI -->|bloom pulse| BS[blurScale]
        EFX[EffectsConfig] -->|base + beat scale| BS
        ACC -->|sample texture| BH[blur_h.fs]
        BS -->|sampling distance| BH
        BH -->|H blurred| TMP[(tempTexture)]
        TMP -->|sample texture| BV[blur_v.fs]
        EFX -->|halfLife| BV
        BS -->|sampling distance| BV
        BV -->|V blurred and decayed| ACC
        ACC -->|blit| SCR[Screen]
    end

    subgraph UILayer[UI System]
        LAYOUT[ui_layout] -->|Rectangle slots| PNL[ui panels]
        WIDGETS[ui_widgets] -->|hue range, beat graph| PNL
        PNL -->|slider values| CFG
        PNL -->|slider values| EFX
        PNL -->|PresetSave| DISK[(presets folder)]
        DISK -->|PresetLoad| CFG
        DISK -->|PresetLoad| EFX
    end
```

**Legend:** Arrows show data flow with payload type. `[(name)]` = persistent buffer. `[name]` = processing step.

## Modules

### audio.cpp / audio.h

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

### beat.cpp / beat.h

Detects beats in audio using energy-based low-pass filtering.

| Function | Purpose |
|----------|---------|
| `BeatDetectorInit` | Initializes detector state and clears history buffers |
| `BeatDetectorProcess` | Applies IIR low-pass filter, computes energy, detects beats when energy exceeds rolling average × sensitivity |
| `BeatDetectorGetBeat` | Returns true if beat detected this frame |
| `BeatDetectorGetIntensity` | Returns current beat intensity (0.0-1.0, decays at 4.0/sec after beat) |

**BeatDetector struct fields:**
- `algorithm`: Detection method (currently `BEAT_ALGO_LOWPASS`)
- `energyHistory[43]`: Rolling window for ~860ms average at 20Hz update rate
- `historyIndex`: Circular buffer write position
- `averageEnergy`, `currentEnergy`: Computed energy values
- `lowPassState`: IIR filter state
- `beatDetected`: True if beat detected this frame
- `beatIntensity`: 0.0-1.0 intensity, decays after beat
- `timeSinceLastBeat`: Debounce timer
- `graphHistory[64]`: Intensity history for UI visualization
- `graphIndex`: Circular buffer write position for graph

**Constants:**
- `BEAT_HISTORY_SIZE`: 43 frames (~860ms at 20Hz)
- `BEAT_GRAPH_SIZE`: 64 samples for display
- `BEAT_DEBOUNCE_SEC`: 0.15s minimum between beats
- `LOW_PASS_ALPHA`: 0.026 (~200Hz cutoff at 48kHz)
- `INTENSITY_DECAY`: 4.0/sec

### effects_config.h

Consolidates effect parameters into a single struct for UI and preset serialization.

**EffectsConfig fields:**
- `halfLife`: Trail persistence in seconds (0.1-2.0), default 0.5
- `baseBlurScale`: Base blur sampling distance in pixels (0-4), default 1
- `beatBlurScale`: Additional blur on beats in pixels (0-5), default 2
- `beatSensitivity`: Beat detection threshold multiplier (1.0-3.0), default 1.0

### waveform.cpp / waveform.h

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
- `rotationSpeed`: Radians per update, range -0.05 to 0.05, default 0.0
- `rotationOffset`: Base rotation offset in radians (for staggered starts)
- `color`: RGBA color, default WHITE

**Constants:**
- `WAVEFORM_SAMPLES`: 1024
- `WAVEFORM_EXTENDED`: 2048 (palindrome for seamless loop)
- `INTERPOLATION_MULT`: 1 (points per sample)
- `MAX_WAVEFORMS`: 8

### visualizer.cpp / visualizer.h

Manages accumulation buffer and two-pass separable blur for physarum-style trail diffusion.

| Function | Purpose |
|----------|---------|
| `VisualizerInit` | Loads blur shaders, creates ping-pong RenderTextures |
| `VisualizerUninit` | Frees shaders and render textures |
| `VisualizerResize` | Recreates render textures at new dimensions |
| `VisualizerBeginAccum` | Computes blur scale from beat intensity, runs horizontal blur, then vertical blur + decay |
| `VisualizerEndAccum` | Ends texture mode |
| `VisualizerToScreen` | Blits accumulation texture to screen |

**Visualizer struct fields:**
- `accumTexture`: Main accumulation buffer
- `tempTexture`: Intermediate buffer for separable blur
- `blurHShader`, `blurVShader`: Separable Gaussian blur shaders
- `blurHResolutionLoc`, `blurVResolutionLoc`: Shader uniform locations for resolution
- `blurHScaleLoc`, `blurVScaleLoc`: Shader uniform locations for blur sampling distance
- `halfLifeLoc`, `deltaTimeLoc`: Shader uniform locations for decay parameters
- `effects`: EffectsConfig containing halfLife, blur scales, and beat sensitivity
- `screenWidth`, `screenHeight`: Current render dimensions

### ui_layout.cpp / ui_layout.h

Declarative layout system that eliminates manual coordinate math for raygui panels.

| Function | Purpose |
|----------|---------|
| `UILayoutBegin` | Creates layout context at (x,y) with width, padding, spacing |
| `UILayoutRow` | Advances to next row with specified height |
| `UILayoutSlot` | Returns Rectangle consuming widthRatio of row (1.0 = remaining) |
| `UILayoutEnd` | Returns final Y position after all rows |
| `UILayoutGroupBegin` | Starts labeled group box (deferred draw) |
| `UILayoutGroupEnd` | Draws group box around contained rows |

**UILayout fields:**
- `x`, `y`: Container origin
- `width`, `padding`, `spacing`: Dimensions
- `rowHeight`, `slotX`: Current row state
- `groupStartY`, `groupTitle`: Deferred group box state

### ui_widgets.cpp / ui_widgets.h

Custom raygui-style widgets extending base functionality.

| Function | Purpose |
|----------|---------|
| `GuiHueRangeSlider` | Dual-handle slider with rainbow gradient for hue range selection (0-360) |
| `GuiBeatGraph` | Scrolling bar graph displaying beat intensity history (64 samples) |

### ui.cpp / ui.h

Application-specific UI panels using ui_layout and ui_widgets.

| Function | Purpose |
|----------|---------|
| `UIStateInit` | Allocates UIState, loads preset file list |
| `UIStateUninit` | Frees UIState |
| `UIBeginPanels` | Sets starting Y for auto-stacking panels |
| `UIDrawWaveformPanel` | Renders waveform list, per-waveform settings, effects controls (blur, half-life, beat sensitivity, bloom), beat graph |
| `UIDrawPresetPanel` | Renders preset name input, Save button, preset list with auto-load |

**UIState fields (opaque struct):**
- `panelY`: Current Y position for panel stacking
- `waveformScrollIndex`: Waveform list scroll state
- `colorModeDropdownOpen`: Dropdown z-order state
- `hueRangeDragging`: Hue slider drag state (0=none, 1=left, 2=right)
- `presetFiles[32]`: Cached preset filenames
- `presetFileCount`, `selectedPreset`, `presetScrollIndex`: Preset list state
- `presetName[64]`, `presetNameEditMode`: Text input state
- `prevSelectedPreset`: Tracks selection changes for auto-load

**Layout constants:**
- Panel width: 180px
- Group spacing: 8px
- Row height: 20px
- Color picker: 62×62px

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

### main.cpp

Application entry point. Consolidates runtime state in `AppContext` and coordinates module lifecycle.

| Function | Purpose |
|----------|---------|
| `AppContextInit` | Allocates context, initializes Visualizer/AudioCapture/UIState/BeatDetector in order |
| `AppContextUninit` | Frees resources in reverse order, NULL-safe |
| `UpdateWaveformAudio` | Reads audio, calls ProcessWaveformBase, ProcessWaveformSmooth per waveform, BeatDetectorProcess, increments globalTick |
| `RenderWaveforms` | Dispatches to DrawWaveformLinear or DrawWaveformCircular based on mode |
| `main` | Creates window, runs 60fps loop with 20fps waveform updates, passes beat intensity to visualizer |

**AppContext struct fields:**
- `vis`: Visualizer instance
- `capture`: AudioCapture instance
- `ui`: UIState instance
- `beat`: BeatDetector instance
- `audioBuffer[2048]`: Raw samples from ring buffer
- `waveform[1024]`: Base normalized waveform
- `waveformExtended[8][2048]`: Per-waveform smoothed palindromes
- `waveforms[8]`: Per-waveform configuration
- `waveformCount`, `selectedWaveform`: Active waveform tracking
- `mode`: WAVEFORM_LINEAR or WAVEFORM_CIRCULAR
- `waveformAccumulator`: Fixed-timestep accumulator for 20fps updates
- `globalTick`: Shared counter for synchronized rotation across waveforms

## Data Flow

1. **Audio Callback** (`audio.cpp`): miniaudio triggers callback with system audio at 48kHz stereo
2. **Ring Buffer Write** (`audio.cpp`): Callback writes to `ma_pcm_rb` (lock-free)
3. **Ring Buffer Read** (`main.cpp`): Main loop reads up to 1024 frames every 50ms (20fps)
4. **Base Processing** (`waveform.cpp`): Normalizes samples to peak=1.0, zero-pads if fewer frames
5. **Beat Detection** (`beat.cpp`): Low-pass filters audio, computes energy, detects beats exceeding rolling average × sensitivity
6. **Per-Waveform Processing** (`waveform.cpp`): Creates palindrome, applies smoothing window
7. **Rotation Calculation** (`waveform.cpp`): Computes effective rotation as `rotationOffset + rotationSpeed * globalTick`
8. **Interpolation** (`waveform.cpp`): Cubic interpolation for smooth curves during render
9. **Draw** (`waveform.cpp`): Renders line segments with per-waveform color
10. **Blur Scale** (`visualizer.cpp`): Computes `baseBlurScale + beatIntensity * beatBlurScale` for bloom pulse
11. **Blur Pass 1** (`visualizer.cpp`): Horizontal N-tap Gaussian blur (scale determines sampling distance)
12. **Blur Pass 2 + Decay** (`visualizer.cpp`): Vertical blur + exponential decay based on halfLife
13. **Composite** (`visualizer.cpp`): New waveform drawn on blurred background
14. **Display** (`main.cpp`): Accumulated texture blitted to screen
15. **UI Overlay** (`ui.cpp`): Panels modify WaveformConfig array and EffectsConfig

## Shaders

Separable Gaussian blur with physarum-style diffusion:

| Shader | Purpose |
|--------|---------|
| `shaders/blur_h.fs` | Horizontal 5-tap Gaussian `[1,4,6,4,1]/16` with configurable sampling distance |
| `shaders/blur_v.fs` | Vertical 5-tap Gaussian + framerate-independent exponential decay with configurable sampling distance |

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
| Window size | 1920×1080 (resizable) | `main.cpp` |
| Render FPS | 60 | `main.cpp` |
| Waveform update rate | 20fps (50ms interval) | `main.cpp` |
| Max waveforms | 8 | `waveform.h` |
| Trail half-life | 0.1-2.0s (UI slider) | `effects_config.h` |
| Base blur scale | 0-4 pixels (UI slider) | `effects_config.h` |
| Beat blur scale | 0-5 pixels (UI slider) | `effects_config.h` |
| Beat sensitivity | 1.0-3.0 (UI slider) | `effects_config.h` |
| Beat debounce | 150ms minimum | `beat.h` |
| Beat history | 43 frames (~860ms rolling average) | `beat.h` |
| Blur kernel | 5-tap Gaussian [1,4,6,4,1]/16 | `blur_h.fs`, `blur_v.fs` |
| Rotation speed range | -0.05 to 0.05 rad/update | `ui.cpp` |
| Preset directory | `presets/` | `preset.cpp` |
| Max preset files | 32 | `preset.h` |
| UI panel width | 180px | `ui.cpp` |

---

*Run `/sync-architecture` to regenerate this document from current code.*
