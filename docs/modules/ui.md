# UI Module

> Part of [AudioJones](../architecture.md)

## Purpose

Provides real-time parameter editing via raygui panels with declarative layout.

## Files

- `src/ui/ui_main.h/cpp` - Main panel orchestration
- `src/ui/ui_layout.h/cpp` - Declarative row/slot layout system
- `src/ui/ui_widgets.h/cpp` - Custom widgets (sliders, beat graph, band meter)
- `src/ui/ui_common.h/cpp` - Shared utilities
- `src/ui/ui_color.h/cpp` - Color picker utilities
- `src/ui/ui_panel_preset.h/cpp` - Preset save/load panel
- `src/ui/ui_panel_audio.h/cpp` - Channel mode selection
- `src/ui/ui_panel_waveform.h/cpp` - Per-layer waveform settings
- `src/ui/ui_panel_spectrum.h/cpp` - Spectrum bar settings
- `src/ui/ui_panel_effects.h/cpp` - Post-effect controls
- `src/ui/ui_panel_analysis.h/cpp` - Beat graph and band energy meters

## Function Reference

### Core

| Function | Purpose |
|----------|---------|
| `UIStateInit` | Allocates UI state |
| `UIStateUninit` | Frees UI state |
| `UIDrawWaveformPanel` | Draws main settings panel, returns bottom Y |

### Layout

| Function | Purpose |
|----------|---------|
| `UILayoutBegin` | Creates layout context at position |
| `UILayoutRow` | Advances to next row with height |
| `UILayoutSlot` | Returns Rectangle for width ratio |
| `UILayoutEnd` | Returns final Y position |
| `UILayoutGroupBegin/End` | Deferred group box drawing |

### Widgets

| Function | Purpose |
|----------|---------|
| `DrawLabeledSlider` | Float slider with label using standard layout |
| `DrawIntSlider` | Int slider with label using standard layout |
| `GuiHueRangeSlider` | Dual-handle slider with rainbow gradient |
| `GuiBeatGraph` | Scrolling 64-sample intensity bar graph |
| `DrawAccordionHeader` | Collapsible section toggle with +/- indicator |
| `GuiBandMeter` | 3-bar bass/mid/treb energy display |

### Analysis Panel

| Function | Purpose |
|----------|---------|
| `UIDrawAnalysisPanel` | Draws beat graph and band energy meter |

### Preset Panel

| Function | Purpose |
|----------|---------|
| `PresetPanelInit` | Allocates state, loads file list |
| `PresetPanelUninit` | Frees state |
| `UIDrawPresetPanel` | Draws preset panel, returns bottom Y |

## Types

### UIState (opaque)

| Field | Description |
|-------|-------------|
| `panel` | PanelState for dropdown coordination |
| `waveformPanel` | WaveformPanelState with scroll position |
| `waveformSectionExpanded` | Waveform accordion state |
| `spectrumSectionExpanded` | Spectrum accordion state |
| `audioSectionExpanded` | Audio accordion state |
| `effectsSectionExpanded` | Effects accordion state |
| `analysisSectionExpanded` | Analysis accordion state |

### PanelState

| Field | Description |
|-------|-------------|
| `colorModeDropdownOpen` | Color dropdown z-order state |
| `spectrumColorModeDropdownOpen` | Spectrum color dropdown state |
| `channelModeDropdownOpen` | Channel dropdown z-order state |
| `lfoWaveformDropdownOpen` | LFO waveform dropdown z-order state |
| `physarumColorModeDropdownOpen` | Physarum color dropdown z-order state |
| `waveformHueRangeDragging` | Waveform hue slider drag state (0/1/2) |
| `spectrumHueRangeDragging` | Spectrum hue slider drag state (0/1/2) |
| `physarumHueRangeDragging` | Physarum hue slider drag state (0/1/2) |

### WaveformPanelState

| Field | Description |
|-------|-------------|
| `scrollIndex` | Waveform list scroll position |

### PresetPanelState (opaque)

| Field | Description |
|-------|-------------|
| `presetFiles[32]` | Cached filenames |
| `presetFileCount` | Files found |
| `selectedPreset`, `presetScrollIndex` | List state |
| `presetName[64]`, `presetNameEditMode` | Text input state |

## Layout Constants

| Constant | Value |
|----------|-------|
| Panel width | 180px |
| Group spacing | 8px |
| Row height | 20px |
| Padding | 8px |

## Data Flow

1. **Entry:** Pointers to config structs passed to draw functions
2. **Transform:** raygui controls modify config values in-place
3. **Exit:** No explicit output; configs update immediately
