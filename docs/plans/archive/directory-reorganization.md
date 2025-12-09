# Directory Reorganization Plan

Reorganize `src/` from flat structure (13 root files) into domain-based subdirectories that communicate AudioJones architecture at a glance. Includes file renames to establish consistent naming patterns.

## Current State

```
src/
├── 13 files in root (audio, spectral, beat, spectrum, waveform, visualizer, preset, configs, main)
└── ui/                    # 11 files - already modularized
```

**Problems**:
1. Flat directory obscures module boundaries
2. `spectral` vs `spectrum` naming causes confusion
3. Inconsistent naming: `effects_config` (plural), `visualizer` (agent noun)

## Target State

```
src/
├── main.cpp                        # Entry point, AppContext orchestration
├── audio/                          # Audio capture pipeline
│   ├── audio.h/cpp                 # WASAPI loopback via miniaudio
│   └── audio_config.h              # ChannelMode enum, capture settings
├── analysis/                       # Signal processing
│   ├── fft.h/cpp                   # 2048-point FFT via kiss_fft (was: spectral)
│   └── beat.h/cpp                  # Spectral flux beat detection
├── render/                         # Visual output
│   ├── render_context.h            # RenderContext typedef (extracted from waveform.h)
│   ├── waveform.h/cpp              # Waveform processing and circular drawing
│   ├── spectrum_bars.h/cpp         # 32-band spectrum bar visualization (was: spectrum)
│   ├── post_effect.h/cpp           # Blur trails, bloom, chromatic aberration (was: visualizer)
│   └── color_config.h              # ColorConfig struct, ColorMode enum
├── config/                         # Serializable state
│   ├── effect_config.h             # EffectsConfig (was: effects_config, now singular)
│   ├── spectrum_bars_config.h      # SpectrumConfig (was: spectrum_config)
│   └── preset.h/cpp                # JSON serialization via nlohmann/json
└── ui/                             # Already organized - no changes
    └── (11 files)
```

## Naming Changes

| Current | New | Rationale |
|---------|-----|-----------|
| `spectral.*` | `fft.*` | Describes function: FFT computation |
| `spectrum.*` | `spectrum_bars.*` | Matches struct `SpectrumBars`, disambiguates from FFT |
| `spectrum_config.h` | `spectrum_bars_config.h` | Follows module name |
| `visualizer.*` | `post_effect.*` | Plain noun matching peers; describes blur/chromatic |
| `effects_config.h` | `effect_config.h` | Singular like `audio_config`, `color_config` |
| (extracted) | `render_context.h` | Shared `RenderContext` typedef for render modules |

### Resulting Naming Patterns

- **Modules**: `audio`, `fft`, `beat`, `waveform`, `spectrum_bars`, `post_effect`
- **Configs**: `audio_config`, `color_config`, `effect_config`, `spectrum_bars_config`
- **UI panels**: `ui_panel_audio`, `ui_panel_effects`, `ui_panel_preset`, `ui_panel_spectrum`, `ui_panel_waveform`

## Rationale

| Directory | Contents | Architecture Signal |
|-----------|----------|---------------------|
| `audio/` | WASAPI capture, ring buffer | "Audio input lives here" |
| `analysis/` | FFT, beat detection | "Signal processing algorithms" |
| `render/` | Waveform, spectrum bars, effects | "All drawing code" |
| `config/` | *_config.h, preset I/O | "Serializable parameters" |
| `ui/` | Panels, widgets, layout | "User interface" (unchanged) |

## Dependency Analysis

### No Circular Dependencies

Dependency graph flows one direction:
```
audio → analysis → render → ui
          ↓          ↓
        config ←────┘
```

### Module Coupling Assessment

| Module | External Dependencies | Internal Dependencies | Move Risk |
|--------|----------------------|----------------------|-----------|
| `audio.*` | miniaudio | none | Low |
| `beat.*` | math.h only | none | Very Low |
| `fft.*` | kiss_fft | `audio.h` (constants only) | Low |
| `spectrum_bars.*` | raylib | `fft.h`, `render_context.h` | Medium |
| `waveform.*` | raylib | `color_config.h`, `render_context.h` | Low |
| `post_effect.*` | raylib | `effect_config.h` | Low |
| `preset.*` | nlohmann/json, filesystem | all config headers | Low |

### Coupling Requiring Attention

1. **`spectrum_bars.cpp` → `fft.h`**: Uses `FFT_SIZE`, `FFT_BIN_COUNT` constants
   - Resolution: Cross-directory include (`#include "analysis/fft.h"`)

2. **`RenderContext` shared by `waveform.h` and `spectrum_bars.h`**
   - Resolution: Extract to `render/render_context.h`

## Implementation Phases

### Phase 0: Extract RenderContext

Create `src/render_context.h` with the `RenderContext` typedef before moving files. Update `waveform.h` and `spectrum.h` to include it.

```c
// src/render_context.h (new file)
#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

typedef struct {
    int screenWidth;
    int screenHeight;
} RenderContext;

#endif
```

### Phase 1: Rename Files (Before Moving)

Rename in place to preserve git history, then move:

```
src/spectral.h       → src/fft.h
src/spectral.cpp     → src/fft.cpp
src/spectrum.h       → src/spectrum_bars.h
src/spectrum.cpp     → src/spectrum_bars.cpp
src/spectrum_config.h→ src/spectrum_bars_config.h
src/visualizer.h     → src/post_effect.h
src/visualizer.cpp   → src/post_effect.cpp
src/effects_config.h → src/effect_config.h
```

### Phase 2: Create Directories and Move Files

```
src/audio.h              → src/audio/audio.h
src/audio.cpp            → src/audio/audio.cpp
src/audio_config.h       → src/audio/audio_config.h

src/fft.h                → src/analysis/fft.h
src/fft.cpp              → src/analysis/fft.cpp
src/beat.h               → src/analysis/beat.h
src/beat.cpp             → src/analysis/beat.cpp

src/render_context.h     → src/render/render_context.h
src/waveform.h           → src/render/waveform.h
src/waveform.cpp         → src/render/waveform.cpp
src/spectrum_bars.h      → src/render/spectrum_bars.h
src/spectrum_bars.cpp    → src/render/spectrum_bars.cpp
src/post_effect.h        → src/render/post_effect.h
src/post_effect.cpp      → src/render/post_effect.cpp
src/color_config.h       → src/render/color_config.h

src/effect_config.h      → src/config/effect_config.h
src/spectrum_bars_config.h → src/config/spectrum_bars_config.h
src/preset.h             → src/config/preset.h
src/preset.cpp           → src/config/preset.cpp
```

**Unchanged:**
- `src/main.cpp` - stays at root as entry point
- `src/ui/*` - already organized

### Phase 3: Update Include Paths and Symbols

**Include path changes:**
```c
// Before (in main.cpp)
#include "audio.h"
#include "spectral.h"
#include "beat.h"
#include "spectrum.h"
#include "visualizer.h"

// After
#include "audio/audio.h"
#include "analysis/fft.h"
#include "analysis/beat.h"
#include "render/spectrum_bars.h"
#include "render/post_effect.h"
```

**Symbol renames (find/replace):**

| Old Symbol | New Symbol |
|------------|------------|
| `SpectralProcessor` | `FFTProcessor` |
| `SpectralProcessorInit` | `FFTProcessorInit` |
| `SpectralProcessorUninit` | `FFTProcessorUninit` |
| `SpectralProcessorFeed` | `FFTProcessorFeed` |
| `SpectralProcessorUpdate` | `FFTProcessorUpdate` |
| `SpectralProcessorGetMagnitude` | `FFTProcessorGetMagnitude` |
| `SpectralProcessorGetBinCount` | `FFTProcessorGetBinCount` |
| `SpectralProcessorGetBinFrequency` | `FFTProcessorGetBinFrequency` |
| `SPECTRAL_FFT_SIZE` | `FFT_SIZE` |
| `SPECTRAL_BIN_COUNT` | `FFT_BIN_COUNT` |
| `VisualizerInit` | `PostEffectInit` |
| `VisualizerUninit` | `PostEffectUninit` |
| `VisualizerResize` | `PostEffectResize` |
| `VisualizerBeginAccum` | `PostEffectBeginAccum` |
| `VisualizerEndAccum` | `PostEffectEndAccum` |
| `VisualizerToScreen` | `PostEffectToScreen` |
| `Visualizer` | `PostEffect` |
| `EffectsConfig` | `EffectConfig` |

**Files requiring updates:**

| File | Changes |
|------|---------|
| `main.cpp` | All includes, all renamed symbols |
| `analysis/fft.cpp` | Header guard, struct/function names |
| `analysis/fft.h` | Header guard, constants, struct/function names |
| `render/spectrum_bars.cpp` | Include `analysis/fft.h`, use new constants |
| `render/spectrum_bars.h` | Header guard, include `render_context.h` |
| `render/waveform.h` | Remove `RenderContext`, include `render_context.h` |
| `render/post_effect.cpp` | Include paths, struct/function names |
| `render/post_effect.h` | Header guard, struct/function names |
| `config/effect_config.h` | Header guard, struct name |
| `config/spectrum_bars_config.h` | Header guard |
| `config/preset.cpp` | Include paths, `EffectConfig` references |
| `ui/ui_main.cpp` | Include paths |
| `ui/ui_panel_effects.cpp` | Include path, `EffectConfig` references |
| `ui/ui_panel_spectrum.cpp` | Include path for config |

### Phase 4: Update CMakeLists.txt

```cmake
set(AUDIO_SOURCES
    src/audio/audio.cpp
)

set(ANALYSIS_SOURCES
    src/analysis/fft.cpp
    src/analysis/beat.cpp
)

set(RENDER_SOURCES
    src/render/waveform.cpp
    src/render/spectrum_bars.cpp
    src/render/post_effect.cpp
)

set(CONFIG_SOURCES
    src/config/preset.cpp
)

set(UI_SOURCES
    src/ui/ui_main.cpp
    src/ui/ui_layout.cpp
    src/ui/ui_widgets.cpp
    src/ui/ui_common.cpp
    src/ui/ui_color.cpp
    src/ui/ui_panel_preset.cpp
    src/ui/ui_panel_effects.cpp
    src/ui/ui_panel_audio.cpp
    src/ui/ui_panel_spectrum.cpp
    src/ui/ui_panel_waveform.cpp
)

add_executable(AudioJones
    src/main.cpp
    ${AUDIO_SOURCES}
    ${ANALYSIS_SOURCES}
    ${RENDER_SOURCES}
    ${CONFIG_SOURCES}
    ${UI_SOURCES}
)
```

### Phase 5: Verify Build

```bash
cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake.exe --build build
./build/AudioJones.exe
```

### Phase 6: Update Documentation

1. Run `/sync-architecture` to regenerate `docs/architecture.md`
2. Update `CLAUDE.md` Key Files section

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Broken includes | Compile after each phase; fix errors immediately |
| Git history fragmentation | Use `git mv` for renames and moves |
| Symbol mismatch | Use find/replace with word boundaries |
| IDE confusion | Rebuild CMake cache after moves |
| Missed reference | `grep -r 'Spectral\|Visualizer\|effects_config' src/` |

## Rollback

If build fails:
```bash
git checkout -- src/
```

All changes occur within `src/` directory. External dependencies (shaders, presets, libs) remain untouched.

## Success Criteria

1. `cmake --build build` succeeds with zero errors
2. `AudioJones.exe` launches and renders waveforms
3. `ls src/` shows 6 items: `main.cpp`, `audio/`, `analysis/`, `render/`, `config/`, `ui/`
4. No files named `spectral`, `spectrum`, `visualizer`, or `effects_config` remain
5. `grep -r 'SpectralProcessor\|Visualizer' src/` returns no matches
6. `/sync-architecture` generates updated documentation

## Estimated Changes

- **Files renamed**: 8
- **Files moved**: 18 (including new `render_context.h`)
- **Files modified**: ~20 (include paths + symbol renames)
- **New files**: 1 (`render_context.h`)
- **Deleted files**: 0

## Decisions Made

1. **`color_config.h` placement**: Keep in `render/` since only render code uses it.

2. **`RenderContext` extraction**: Create `render/render_context.h` as shared dependency for `waveform.h` and `spectrum_bars.h`.

3. **Header-only configs**: Keep separate files for clarity and minimal merge conflicts.

4. **UI panel naming**: Keep `ui_panel_effects` (not `ui_panel_effect`) since it controls multiple effects and matches user-facing terminology.
