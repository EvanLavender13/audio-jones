# UI Module

> Part of [AudioJones](../architecture.md)

## Purpose

Provides real-time parameter editing via raygui panels with declarative layout.

## Files

- `src/ui/ui_main.h/cpp` - Main panel orchestration
- `src/ui/ui_layout.h/cpp` - Declarative row/slot layout system
- `src/ui/ui_widgets.h/cpp` - Custom widgets (hue range slider, beat graph)
- `src/ui/ui_common.h/cpp` - Shared utilities
- `src/ui/ui_color.h/cpp` - Color picker utilities
- `src/ui/ui_panel_preset.h/cpp` - Preset save/load panel
- `src/ui/ui_panel_audio.h/cpp` - Channel mode selection
- `src/ui/ui_panel_waveform.h/cpp` - Per-layer waveform settings
- `src/ui/ui_panel_spectrum.h/cpp` - Spectrum bar settings
- `src/ui/ui_panel_effects.h/cpp` - Post-effect controls

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
| `GuiHueRangeSlider` | Dual-handle slider with rainbow gradient |
| `GuiBeatGraph` | Scrolling 64-sample intensity bar graph |

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
| `waveformScrollIndex` | Waveform list scroll position |
| `colorModeDropdownOpen` | Color dropdown z-order state |
| `channelModeDropdownOpen` | Channel dropdown z-order state |
| `hueRangeDragging` | Hue slider drag state (0/1/2) |

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
