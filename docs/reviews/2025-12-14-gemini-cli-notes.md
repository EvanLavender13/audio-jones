# Code Review Notes - 2025-12-14

## Phase 2: Systematic Exploration

### Group 1: Config and Common Headers

#### `audio/audio_config.h`
- Simple struct, no dependencies.
- `channelMode` enum.

#### `config/band_config.h`
- Simple struct.

#### `config/lfo_config.h`
- Simple struct, `LFOType` enum.

#### `config/spectrum_bars_config.h`
- Includes `render/color_config.h`.

#### `render/color_config.h`
- `ColorGradient` struct.
- `MAX_GRADIENT_STOPS` defined.

#### `render/render_context.h`
- `RenderContext` struct for screen dimensions and center.

#### `ui/ui_common.h`
- `UIState` struct (forward declared?).

#### `ui/ui_layout.h`
- Layout helper functions.

### Group 2: Core Systems

#### `audio/audio.cpp` / `audio.h`
- Uses `miniaudio.h`.
- `AudioCapture` struct.
- `ma_device`, `ma_pcm_rb`.
- Thread safety: `ma_pcm_rb` is lock-free. Correctly implemented.

#### `analysis/fft.cpp` / `fft.h`
- Uses `kiss_fftr`.
- `FFTProcessor` struct.
- Resource management: Init/Uninit used correctly.

#### `automation/lfo.cpp` / `lfo.h`
- `LFO` struct.
- `LFOUpdate`, `LFOGetValue`.
- Uses `rand()` for sample & hold, which is acceptable.

#### `render/physarum.cpp` / `physarum.h`
- `PhysarumConfig` struct.
- Uses Compute Shaders (OpenGL 4.3).
- Correctly checks for support.
- Dispatches compute shader and uses `glMemoryBarrier`.

### Group 3: Intermediate Systems

#### `analysis/beat.cpp` / `beat.h`
- `BeatDetector` struct.
- Logic: Energy based + kick drum band flux. Includes history and simple statistics.

#### `analysis/bands.cpp` / `bands.h`
- `BandAnalyzer` struct.
- Standard RMS + Envelope Follower.

#### `render/waveform.cpp` / `waveform.h`
- `WaveformRenderer` struct.
- O(N) smoothing implementation.
- Linear and Circular drawing modes.

#### `ui/ui_widgets.cpp` / `ui_widgets.h`
- Custom widgets helpers.

### Group 4: Pipelines & High Level

#### `analysis/analysis_pipeline.cpp` / `analysis_pipeline.h`
- Orchestrates audio -> FFT -> Beat/Bands.
- Thread/Buffer safety: Reads from ring buffer, normalizes. Safe.

#### `render/waveform_pipeline.cpp` / `waveform_pipeline.h`
- Orchestrates waveform processing.
- Handles multiple waveforms and smoothing.

#### `render/spectrum_bars.cpp` / `spectrum_bars.h`
- Renders spectrum bars.

#### `render/post_effect.cpp` / `post_effect.h`
- `PostEffect` struct.
- Manages HDR textures and Shaders.
- `PostEffectBeginAccum`/`EndAccum` maintain state.

### Group 5: UI & Main

#### `ui/ui_panel_*.cpp`
- Immediate mode UI logic.
- Returns deferred dropdowns.

#### `ui/ui_main.cpp`
- Main UI orchestration.
- Handles accordion state.

#### `config/preset.cpp`
- JSON serialization.
- Uses STL (permitted deviation).

#### `main.cpp`
- Main loop.
- Calls `Uninit` on exit.
- Updates visuals at 20Hz, renders at 60Hz.

## Phase 3: Evaluation

- **Conventions**: Strictly followed.
- **Logic**: Sound.
- **Resources**: Safely managed.
- **Threading**: Correct usage of lock-free buffer.