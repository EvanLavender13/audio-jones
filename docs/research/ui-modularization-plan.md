# UI Modularization Plan

Restructure `src/ui.cpp` (440 lines) into a `src/ui/` directory with per-panel modules. Extract shared color controls into a reusable component.

## Current State

| File | Lines | Role |
|------|-------|------|
| `src/ui.cpp` | 440 | Monolithic: waveform, spectrum, audio, effects panels |
| `src/ui_preset.cpp` | 113 | Already extracted |
| `src/ui_widgets.cpp` | 114 | Already extracted |
| `src/ui_layout.cpp` | 67 | Already extracted |

**Problem**: `ui.cpp` contains 4 distinct panels with shared dropdown coordination and duplicated color controls between waveform and spectrum sections.

## Target Structure

```
src/
├── ui/
│   ├── ui_common.h           # PanelState, dropdown coordination
│   ├── ui_common.cpp         # State lifecycle, anyDropdownOpen()
│   ├── ui_color.h            # ColorControlState + UIDrawColorControls
│   ├── ui_color.cpp          # Color picker/rainbow UI (~64 lines)
│   ├── ui_panel_waveform.h   # WaveformPanelState + draw function
│   ├── ui_panel_waveform.cpp # Waveform list + settings (~120 lines)
│   ├── ui_panel_spectrum.h   # (no state) + draw function
│   ├── ui_panel_spectrum.cpp # Spectrum controls (~72 lines)
│   ├── ui_panel_effects.h    # (no state) + draw function
│   ├── ui_panel_effects.cpp  # Effects controls (~60 lines)
│   ├── ui_panel_audio.h      # (no state) + draw function
│   ├── ui_panel_audio.cpp    # Audio controls (~30 lines)
│   ├── ui_main.h             # UIState + UIDrawWaveformPanel
│   └── ui_main.cpp           # Accordion orchestration (~80 lines)
├── ui_layout.h               # Unchanged
├── ui_layout.cpp             # Unchanged
├── ui_widgets.h              # Unchanged
├── ui_widgets.cpp            # Unchanged
├── ui_preset.h               # Unchanged
└── ui_preset.cpp             # Unchanged
```

## Module Specifications

### ui_common.h/cpp

Shared state and dropdown coordination.

```c
// ui_common.h
typedef struct PanelState {
    bool colorModeDropdownOpen;
    bool spectrumColorModeDropdownOpen;
    bool channelModeDropdownOpen;
    int hueRangeDragging;
} PanelState;

bool AnyDropdownOpen(PanelState* state);  // Returns true if any dropdown expanded
```

### ui_color.h/cpp

Extracted from `ui.cpp:58-121`. Shared by waveform and spectrum panels.

```c
// ui_color.h
#include "ui_layout.h"
#include "color_config.h"

// Renders color mode controls (solid picker or rainbow sliders).
// Returns dropdown rect for deferred z-order drawing.
Rectangle UIDrawColorControls(UILayout* l, PanelState* state, ColorConfig* color);
```

### ui_panel_waveform.h/cpp

Extracted from `ui.cpp:197-274`. Manages waveform list and per-waveform settings.

```c
// ui_panel_waveform.h
typedef struct WaveformPanelState {
    int scrollIndex;
} WaveformPanelState;

WaveformPanelState* WaveformPanelInit(void);
void WaveformPanelUninit(WaveformPanelState* state);

// Renders waveform list + selected waveform settings.
// Returns dropdown rect for deferred z-order drawing.
Rectangle UIDrawWaveformPanel(UILayout* l, WaveformPanelState* wfState, PanelState* state,
                              WaveformConfig* waveforms, int* waveformCount,
                              int* selectedWaveform);
```

### ui_panel_spectrum.h/cpp

Extracted from `ui.cpp:124-195`. Stateless (dropdown state in PanelState).

```c
// ui_panel_spectrum.h

// Renders spectrum bar controls (geometry, dynamics, rotation, color).
// Returns dropdown rect for deferred z-order drawing.
Rectangle UIDrawSpectrumPanel(UILayout* l, PanelState* state, SpectrumConfig* config);
```

### ui_panel_effects.h/cpp

Extracted from `ui.cpp:276-333`. Stateless.

```c
// ui_panel_effects.h

// Renders effects controls (blur, half-life, beat sensitivity, bloom, chroma, beat graph).
void UIDrawEffectsPanel(UILayout* l, PanelState* state, EffectsConfig* effects,
                        BeatDetector* beat);
```

### ui_panel_audio.h/cpp

Extracted from `ui.cpp:335-362`. Stateless.

```c
// ui_panel_audio.h

// Renders audio channel mode dropdown.
// Returns dropdown rect for deferred z-order drawing.
Rectangle UIDrawAudioPanel(UILayout* l, PanelState* state, AudioConfig* audio);
```

### ui_main.h/cpp

Orchestrates accordion sections and deferred dropdown drawing.

```c
// ui_main.h
typedef struct UIState UIState;

UIState* UIStateInit(void);
void UIStateUninit(UIState* state);

// Renders all accordion sections. Returns bottom Y position.
int UIDrawWaveformPanel(UIState* state, int startY, WaveformConfig* waveforms,
                        int* waveformCount, int* selectedWaveform,
                        EffectsConfig* effects, AudioConfig* audio,
                        SpectrumConfig* spectrum, BeatDetector* beat);
```

UIState contains:
- `PanelState panelState` - dropdown coordination
- `WaveformPanelState* waveformPanel` - list scroll state
- Accordion expansion bools (waveformSectionExpanded, spectrumSectionExpanded, etc.)

## Implementation Phases

### Phase 1: Create Directory and Extract Shared Components

1. Create `src/ui/` directory
2. Extract `ui_common.h/cpp` - PanelState struct and AnyDropdownOpen helper
3. Extract `ui_color.h/cpp` - color controls from `ui.cpp:58-121`
4. Verify build compiles

**Estimated lines**: ~100 new, ~64 moved

### Phase 2: Extract Individual Panels

Extract in dependency order:

1. `ui_panel_effects.h/cpp` - no dependencies on other panels
2. `ui_panel_audio.h/cpp` - no dependencies on other panels
3. `ui_panel_spectrum.h/cpp` - depends on ui_color
4. `ui_panel_waveform.h/cpp` - depends on ui_color

After each extraction, verify build compiles.

**Estimated lines**: ~280 moved

### Phase 3: Create Orchestrator

1. Create `ui_main.h/cpp` with accordion logic and deferred dropdown drawing
2. Update UIState to aggregate panel states
3. Delete original `src/ui.cpp` and `src/ui.h`
4. Update `main.cpp` includes: `#include "ui/ui_main.h"`
5. Update CMakeLists.txt source list

**Estimated lines**: ~80 new, ~100 deleted

### Phase 4: Finalize

1. Run `/sync-architecture` to regenerate architecture.md
2. Verify all presets still load correctly
3. Test accordion expand/collapse behavior
4. Test dropdown z-order (dropdowns appear above other controls)

## File Changes Summary

| Action | Count | Details |
|--------|-------|---------|
| Create | 14 | 7 headers + 7 implementations in `src/ui/` |
| Delete | 2 | `src/ui.h`, `src/ui.cpp` |
| Modify | 2 | `main.cpp` (includes), `CMakeLists.txt` (sources) |
| Total | 18 | |

## Line Count Estimate

| Category | Lines |
|----------|-------|
| Moved from ui.cpp | ~440 |
| New interface code | ~60 |
| New includes/guards | ~40 |
| Deleted redundant | ~50 |
| **Net change** | **+50** |

## Risk Assessment

**Low risk**: Pure restructure with no logic changes. Each phase produces a compiling build. Rollback by reverting directory creation.

**Testing**: Manual verification of:
- All 4 accordion sections expand/collapse
- Waveform list selection and scroll
- Color mode dropdown (solid/rainbow) on both waveform and spectrum
- Channel mode dropdown in audio section
- Preset save/load preserves all values

## Dependencies

- `ui_layout.h/cpp` - unchanged, used by all panels
- `ui_widgets.h/cpp` - unchanged, used by ui_color and ui_panel_effects
- `ui_preset.h/cpp` - unchanged, independent module
- `color_config.h` - already extracted, used by ui_color

## Sources

- [Patterns of Modular Architecture - DZone](https://dzone.com/refcardz/patterns-modular-architecture)
- [About the IMGUI Paradigm - Dear ImGui Wiki](https://github.com/ocornut/imgui/wiki/About-the-IMGUI-paradigm)
