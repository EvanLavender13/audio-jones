# UI Module

> Part of [AudioJones](../architecture.md)

## Purpose

Provides real-time parameter editing via Dear ImGui panels using rlImGui integration.

## Files

- `src/ui/imgui_panels.h` - Main panel API, theme, and dockspace
- `src/ui/imgui_panels.cpp` - Panel orchestration and theme setup
- `src/ui/imgui_effects.cpp` - Post-effect controls
- `src/ui/imgui_waveforms.cpp` - Per-layer waveform settings
- `src/ui/imgui_spectrum.cpp` - Spectrum bar settings
- `src/ui/imgui_audio.cpp` - Channel mode selection
- `src/ui/imgui_analysis.cpp` - Beat graph and band energy meters
- `src/ui/imgui_presets.cpp` - Preset save/load panel
- `src/ui/imgui_widgets.cpp` - Shared widget helpers (color mode, hue slider)

## Function Reference

### Core

| Function | Purpose |
|----------|---------|
| `ImGuiApplyDarkTheme` | Applies dark theme styling after rlImGuiSetup |
| `ImGuiDrawDockspace` | Draws transparent dockspace covering viewport with passthrough |

### Shared Widgets

| Function | Purpose |
|----------|---------|
| `ImGuiDrawColorMode` | Draws color mode controls (solid/rainbow) with hue range slider |

### Panels

| Function | Purpose |
|----------|---------|
| `ImGuiDrawEffectsPanel` | Draws post-effect controls (blur, trails, warp, voronoi, physarum, LFO) |
| `ImGuiDrawWaveformsPanel` | Draws waveform list and per-layer settings (radius, thickness, color) |
| `ImGuiDrawSpectrumPanel` | Draws spectrum bar settings (geometry, dynamics, color) |
| `ImGuiDrawAudioPanel` | Draws audio channel mode selection |
| `ImGuiDrawAnalysisPanel` | Draws beat graph and band energy meters with sensitivity controls |
| `ImGuiDrawPresetPanel` | Draws preset save/load panel with file list |

## Data Flow

1. **Entry:** Pointers to config structs passed to panel functions
2. **Transform:** Dear ImGui controls modify config values in-place
3. **Exit:** No explicit output; configs update immediately
