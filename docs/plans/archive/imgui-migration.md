# Replace raygui with Dear ImGui via rlImGui

> Status: Ready for implementation

## Goal

Replace the raygui-based UI system (24 files, ~1,200 LOC) with Dear ImGui via rlImGui to enable:
- **Dockable windows** - Drag-to-dock layouts with tab grouping
- **Dark minimal theme** - Modern aesthetic like professional audio tools
- **Layout persistence** - Save/restore window positions and dock state
- **Native widget handling** - No manual z-order, dropdown coordination, or scroll management

## Current State

### UI Architecture

24 files in `src/ui/` using raygui:

| Category | Files | LOC | Purpose |
|----------|-------|-----|---------|
| Infrastructure | `ui_main`, `ui_layout`, `ui_window`, `ui_common` | ~400 | Window management, z-order, layout math |
| Widgets | `ui_widgets`, `ui_color` | ~300 | Custom controls, beat graph, band meter |
| Panels | `ui_panel_*` (6 panels) | ~400 | Effects, waveforms, spectrum, audio, analysis, preset |
| **Total** | **24 files** | **~1,200** | |

### Pain Points Being Solved

- `ui_window.cpp:40-90` - Manual title bar drag detection with offset tracking
- `ui_main.cpp:248-254` - Z-order sorting with qsort
- `ui_common.cpp` - Dropdown coordination (only one open at a time)
- `ui_main.cpp:206-233` - Input blocking when mouse over floating windows
- Deferred dropdown rendering to handle z-ordering

All of this is handled natively by ImGui.

### Config Structures (Unchanged)

These remain exactly as-is - only UI bindings change:

- `config/effect_config.h` - `EffectConfig` (32 parameters)
- `config/waveform_config.h` - `WaveformConfig` (8 parameters per waveform)
- `config/spectrum_bars_config.h` - `SpectrumConfig` (6 parameters)
- `config/audio_config.h` - `AudioConfig` (channel mode enum)
- `config/band_config.h` - `BandConfig` (3 sensitivity values)

## Architecture Decision

**Pragmatic Balance: Functional Panel Organization**

Keep one file per domain (Effects, Waveforms, Spectrum, etc.) while deleting all raygui infrastructure:

```
src/ui/
├── imgui_panels.h        # Public interface + theme
├── imgui_panels.cpp      # Dockspace setup + theme
├── imgui_effects.cpp     # Effects panel (~100 LOC)
├── imgui_waveforms.cpp   # Waveforms panel + hue slider (~120 LOC)
├── imgui_spectrum.cpp    # Spectrum panel (~40 LOC)
├── imgui_analysis.cpp    # Beat graph, band meter (~100 LOC)
├── imgui_audio.cpp       # Audio channel dropdown (~20 LOC)
└── imgui_presets.cpp     # Preset save/load (~80 LOC)
```

**Rationale:**
- Each domain has its own file for maintainability and extensibility
- ImGui handles window/docking/scroll/z-order natively (eliminates 400+ LOC infrastructure)
- Config structs unchanged (preset compatibility preserved)
- New panels = new files, not modifications to a monolith

## Component Design

### imgui_panels.h - Public Interface

```cpp
#ifndef IMGUI_PANELS_H
#define IMGUI_PANELS_H

#include "config/app_configs.h"

// Call once after rlImGuiSetup()
void ImGuiApplyDarkTheme(void);

// Draw main dockspace covering viewport
void ImGuiDrawDockspace(void);

// Panel draw functions
void ImGuiDrawEffectsPanel(EffectConfig* cfg);
void ImGuiDrawWaveformsPanel(WaveformConfig* waveforms, int* count, int* selected);
void ImGuiDrawSpectrumPanel(SpectrumConfig* cfg);
void ImGuiDrawAnalysisPanel(BeatDetector* beat, BandEnergies* bands, BandConfig* cfg);
void ImGuiDrawAudioPanel(AudioConfig* cfg);
void ImGuiDrawPresetPanel(AppConfigs* configs);

#endif // IMGUI_PANELS_H
```

### imgui_panels.cpp - Dockspace and Theme

```cpp
#include "imgui.h"
#include "imgui_panels.h"

void ImGuiApplyDarkTheme(void)
{
    ImGuiStyle* style = &ImGui::GetStyle();
    style->WindowRounding = 0.0f;
    style->FrameRounding = 2.0f;
    style->FramePadding = ImVec2(6, 4);
    style->ItemSpacing = ImVec2(8, 4);
    style->WindowBorderSize = 1.0f;
    style->FrameBorderSize = 1.0f;

    ImVec4* colors = style->Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
}

void ImGuiDrawDockspace(void)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNavFocus |
                             ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", NULL, flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}
```

### imgui_effects.cpp - Effects Panel

```cpp
#include "imgui.h"
#include "imgui_panels.h"
#include "config/effect_config.h"

static void DrawColorMode(ColorConfig* color)
{
    const char* modes[] = {"Solid", "Rainbow"};
    int mode = (int)color->mode;
    if (ImGui::Combo("Mode", &mode, modes, 2)) {
        color->mode = (ColorMode)mode;
    }

    if (color->mode == COLOR_SOLID) {
        float col[4] = {color->r/255.0f, color->g/255.0f, color->b/255.0f, color->a/255.0f};
        if (ImGui::ColorEdit4("Color", col, ImGuiColorEditFlags_NoInputs)) {
            color->r = (unsigned char)(col[0] * 255);
            color->g = (unsigned char)(col[1] * 255);
            color->b = (unsigned char)(col[2] * 255);
            color->a = (unsigned char)(col[3] * 255);
        }
    } else {
        ImGui::SliderFloat("Hue Start", &color->hueStart, 0.0f, 360.0f, "%.0f");
        ImGui::SliderFloat("Hue End", &color->hueEnd, 0.0f, 360.0f, "%.0f");
        ImGui::SliderFloat("Speed", &color->hueSpeed, 0.0f, 100.0f);
    }
}

void ImGuiDrawEffectsPanel(EffectConfig* e)
{
    if (!ImGui::Begin("Effects")) {
        ImGui::End();
        return;
    }

    ImGui::SliderInt("Blur", &e->baseBlurScale, 0, 4);
    ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
    ImGui::SliderInt("Bloom", &e->beatBlurScale, 0, 5);
    ImGui::SliderInt("Chroma", &e->chromaticMaxOffset, 0, 50);
    ImGui::SliderFloat("Zoom", &e->feedbackZoom, 0.9f, 1.0f, "%.3f");
    ImGui::SliderFloat("Rotation", &e->feedbackRotation, -0.02f, 0.02f, "%.4f rad");
    ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);
    ImGui::SliderInt("Kaleido", &e->kaleidoSegments, 1, 12);

    if (ImGui::CollapsingHeader("Domain Warp")) {
        ImGui::SliderFloat("Strength", &e->warpStrength, 0.0f, 0.05f);
        if (e->warpStrength > 0.0f) {
            ImGui::SliderFloat("Scale##warp", &e->warpScale, 1.0f, 20.0f);
            ImGui::SliderInt("Octaves", &e->warpOctaves, 1, 5);
            ImGui::SliderFloat("Lacunarity", &e->warpLacunarity, 1.5f, 3.0f);
            ImGui::SliderFloat("Speed##warp", &e->warpSpeed, 0.1f, 2.0f);
        }
    }

    if (ImGui::CollapsingHeader("Voronoi")) {
        ImGui::SliderFloat("Intensity", &e->voronoiIntensity, 0.0f, 1.0f);
        if (e->voronoiIntensity > 0.0f) {
            ImGui::SliderFloat("Scale##vor", &e->voronoiScale, 5.0f, 50.0f);
            ImGui::SliderFloat("Speed##vor", &e->voronoiSpeed, 0.1f, 2.0f);
            ImGui::SliderFloat("Edge", &e->voronoiEdgeWidth, 0.01f, 0.1f);
        }
    }

    if (ImGui::CollapsingHeader("Physarum")) {
        ImGui::Checkbox("Enabled##phys", &e->physarum.enabled);
        if (e->physarum.enabled) {
            ImGui::SliderInt("Agents", &e->physarum.agentCount, 10000, 1000000);
            ImGui::SliderFloat("Sensor Dist", &e->physarum.sensorDistance, 1.0f, 100.0f);
            ImGui::SliderFloat("Sensor Angle", &e->physarum.sensorAngle, 0.0f, 6.28f, "%.2f rad");
            ImGui::SliderFloat("Turn Angle", &e->physarum.turningAngle, 0.0f, 6.28f, "%.2f rad");
            ImGui::SliderFloat("Step Size", &e->physarum.stepSize, 0.1f, 100.0f);
            ImGui::SliderFloat("Deposit", &e->physarum.depositAmount, 0.01f, 5.0f);
            ImGui::SliderFloat("Decay", &e->physarum.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion", &e->physarum.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost", &e->physarum.boostIntensity, 0.0f, 5.0f);
            ImGui::SliderFloat("Sense Blend", &e->physarum.accumSenseBlend, 0.0f, 1.0f);
            DrawColorMode(&e->physarum.color);
            ImGui::Checkbox("Debug", &e->physarum.debugOverlay);
        }
    }

    if (ImGui::CollapsingHeader("Rotation LFO")) {
        ImGui::Checkbox("Enabled##lfo", &e->rotationLFO.enabled);
        if (e->rotationLFO.enabled) {
            ImGui::SliderFloat("Rate", &e->rotationLFO.rate, 0.01f, 1.0f, "%.2f Hz");
            const char* waveforms[] = {"Sine", "Triangle", "Saw", "Square", "S&H"};
            ImGui::Combo("Waveform", &e->rotationLFO.waveform, waveforms, 5);
        }
    }

    ImGui::End();
}
```

### imgui_analysis.cpp - Custom Widgets

```cpp
#include "imgui.h"
#include "imgui_panels.h"
#include "analysis/beat.h"
#include "analysis/bands.h"
#include "config/band_config.h"

static void DrawBeatGraph(const BeatDetector* beat)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 60);

    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(30, 30, 30, 255));

    float barWidth = size.x / BEAT_HISTORY_SIZE;
    for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
        int idx = (beat->historyIndex + i) % BEAT_HISTORY_SIZE;
        float intensity = beat->history[idx];
        if (intensity < 0.0f) intensity = 0.0f;
        if (intensity > 1.0f) intensity = 1.0f;

        float barHeight = intensity * (size.y - 4);
        float x = pos.x + i * barWidth + 1;
        float y = pos.y + size.y - 2 - barHeight;

        unsigned char brightness = 80 + (unsigned char)(intensity * 175);
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + barWidth - 2, y + barHeight),
                            IM_COL32(brightness, brightness, brightness, 255));
    }

    ImGui::Dummy(size);
}

static void DrawBandMeter(const BandEnergies* bands, const BandConfig* cfg)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = 20;

    const char* labels[] = {"Bass", "Mid", "Treb"};
    float values[] = {
        bands->bassSmooth / (bands->bassAvg + 0.001f) * cfg->bassSensitivity,
        bands->midSmooth / (bands->midAvg + 0.001f) * cfg->midSensitivity,
        bands->trebSmooth / (bands->trebAvg + 0.001f) * cfg->trebSensitivity
    };
    ImU32 colors[] = {IM_COL32(200, 80, 80, 255), IM_COL32(80, 200, 80, 255), IM_COL32(80, 80, 200, 255)};

    for (int i = 0; i < 3; i++) {
        float y = pos.y + i * (height + 4);
        draw->AddRectFilled(ImVec2(pos.x, y), ImVec2(pos.x + width, y + height), IM_COL32(40, 40, 40, 255));

        float val = values[i];
        if (val > 2.0f) val = 2.0f;
        float barWidth = (val / 2.0f) * width;
        draw->AddRectFilled(ImVec2(pos.x, y), ImVec2(pos.x + barWidth, y + height), colors[i]);

        draw->AddText(ImVec2(pos.x + 4, y + 3), IM_COL32(255, 255, 255, 200), labels[i]);
    }

    ImGui::Dummy(ImVec2(width, 3 * (height + 4)));
}

void ImGuiDrawAnalysisPanel(BeatDetector* beat, BandEnergies* bands, BandConfig* cfg)
{
    if (!ImGui::Begin("Analysis")) {
        ImGui::End();
        return;
    }

    ImGui::Text("Beat Detection");
    DrawBeatGraph(beat);

    ImGui::Separator();

    ImGui::Text("Band Energy");
    DrawBandMeter(bands, cfg);

    ImGui::Separator();

    ImGui::SliderFloat("Bass Sens", &cfg->bassSensitivity, 0.1f, 3.0f);
    ImGui::SliderFloat("Mid Sens", &cfg->midSensitivity, 0.1f, 3.0f);
    ImGui::SliderFloat("Treb Sens", &cfg->trebSensitivity, 0.1f, 3.0f);

    ImGui::End();
}
```

### main.cpp Integration

```cpp
#include "rlImGui.h"
#include "ui/imgui_panels.h"

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "AudioJones");
    SetTargetFPS(60);

    // ImGui setup
    rlImGuiSetup(true);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().IniFilename = "audiojones_layout.ini";
    ImGuiApplyDarkTheme();

    AppContext* ctx = AppContextInit(1920, 1080);
    // ... existing init ...

    while (!WindowShouldClose())
    {
        // ... existing audio/render code ...

        BeginDrawing();
            ClearBackground(BLACK);
            PostEffectToScreen(ctx->postEffect, ctx->waveformPipeline.globalTick);
            DrawText(TextFormat("%d fps", GetFPS()), 10, 10, 16, GRAY);

            rlImGuiBegin();
                ImGuiDrawDockspace();
                ImGuiDrawEffectsPanel(&ctx->postEffect->effects);
                ImGuiDrawWaveformsPanel(ctx->waveforms, &ctx->waveformCount, &ctx->selectedWaveform);
                ImGuiDrawSpectrumPanel(&ctx->spectrum);
                ImGuiDrawAnalysisPanel(&ctx->analysis.beat, &ctx->analysis.bands, &ctx->bandConfig);
                ImGuiDrawAudioPanel(&ctx->audio);
                // ImGuiDrawPresetPanel(&configs);
            rlImGuiEnd();
        EndDrawing();
    }

    rlImGuiShutdown();
    AppContextUninit(ctx);
    CloseWindow();
    return 0;
}
```

### Widget Mapping Reference

| raygui | ImGui | Notes |
|--------|-------|-------|
| `GuiSliderBar()` | `ImGui::SliderFloat/Int()` | Built-in value display |
| `GuiToggle()` | `ImGui::Checkbox()` | Same semantics |
| `GuiDropDownBox()` | `ImGui::Combo()` | Auto-closes, no z-order issues |
| `GuiTextBox()` | `ImGui::InputText()` | Built-in edit mode |
| `GuiButton()` | `ImGui::Button()` | Returns bool |
| `GuiColorPicker()` | `ImGui::ColorEdit4()` | Better UX |
| `GuiWindowBox()` | `ImGui::Begin()/End()` | Native drag/dock |
| `GuiScrollPanel()` | Automatic in `Begin()` | No manual scissor |
| `DrawAccordionHeader()` | `ImGui::CollapsingHeader()` | Built-in |
| Manual z-order | Native | ImGui handles internally |

## File Changes

| File | Change |
|------|--------|
| `CMakeLists.txt` | Modify - Add rlImGui, replace UI sources, remove raygui |
| `src/main.cpp` | Modify - Add rlImGui lifecycle, replace UI calls |
| `src/ui/imgui_panels.h` | Create |
| `src/ui/imgui_panels.cpp` | Create - Dockspace + theme |
| `src/ui/imgui_effects.cpp` | Create - Effects panel |
| `src/ui/imgui_waveforms.cpp` | Create - Waveforms panel + hue slider |
| `src/ui/imgui_spectrum.cpp` | Create - Spectrum panel |
| `src/ui/imgui_analysis.cpp` | Create - Beat graph, band meter |
| `src/ui/imgui_audio.cpp` | Create - Channel dropdown |
| `src/ui/imgui_presets.cpp` | Create - Preset save/load |
| `src/ui/ui_main.h` | Delete |
| `src/ui/ui_main.cpp` | Delete |
| `src/ui/ui_layout.h` | Delete |
| `src/ui/ui_layout.cpp` | Delete |
| `src/ui/ui_window.h` | Delete |
| `src/ui/ui_window.cpp` | Delete |
| `src/ui/ui_common.h` | Delete |
| `src/ui/ui_common.cpp` | Delete |
| `src/ui/ui_color.h` | Delete |
| `src/ui/ui_color.cpp` | Delete |
| `src/ui/ui_widgets.h` | Delete |
| `src/ui/ui_widgets.cpp` | Delete |
| `src/ui/ui_panel_effects.h` | Delete |
| `src/ui/ui_panel_effects.cpp` | Delete |
| `src/ui/ui_panel_waveform.h` | Delete |
| `src/ui/ui_panel_waveform.cpp` | Delete |
| `src/ui/ui_panel_spectrum.h` | Delete |
| `src/ui/ui_panel_spectrum.cpp` | Delete |
| `src/ui/ui_panel_audio.h` | Delete |
| `src/ui/ui_panel_audio.cpp` | Delete |
| `src/ui/ui_panel_analysis.h` | Delete |
| `src/ui/ui_panel_analysis.cpp` | Delete |
| `src/ui/ui_panel_preset.h` | Delete |
| `src/ui/ui_panel_preset.cpp` | Delete |

## Build Sequence

### Phase 1: rlImGui Integration

- Add rlImGui to CMakeLists.txt via FetchContent
- Add rlImGui initialization in main.cpp
- Create `imgui_panels.h/cpp` with dockspace and theme
- Test: ImGui renders, docking works

**Validation:**
- [x] Build succeeds with rlImGui dependency
- [x] Empty dockspace visible over visualization
- [x] Dark theme applied

### Phase 2: Effects Panel

- Create `imgui_effects.cpp` with all 29+ controls
- Port conditional sections (Warp, Voronoi, Physarum, LFO)
- Port color mode widget

**Validation:**
- [x] All effect parameters editable
- [x] CollapsingHeaders work
- [x] Values bind to EffectConfig correctly

### Phase 3: Waveforms Panel

- Create `imgui_waveforms.cpp`
- Implement waveform list with Selectable
- Port add/remove buttons
- Port per-waveform settings
- Create hue range slider widget

**Validation:**
- [x] Create/select/delete waveforms
- [x] All waveform parameters editable
- [x] Color mode switching works

### Phase 4: Simple Panels

- Create `imgui_spectrum.cpp` (6 controls)
- Create `imgui_audio.cpp` (channel dropdown)

**Validation:**
- [x] Spectrum controls functional
- [x] Channel mode dropdown works

### Phase 5: Custom Widgets (Analysis)

- Create `imgui_analysis.cpp`
- Implement beat graph with ImDrawList
- Implement band meter with ImDrawList
- Add band sensitivity sliders

**Validation:**
- [x] Beat graph shows real-time data
- [x] Band meters animate correctly
- [x] Sensitivity sliders work

### Phase 6: Preset Panel

- Create `imgui_presets.cpp`
- Port file list and name input
- Integrate with existing `preset.cpp` serialization

**Validation:**
- [x] Preset list displays files
- [x] Save creates JSON file
- [x] Load restores parameters

### Phase 7: Cleanup

- Remove raygui from CMakeLists.txt
- Delete all 24 `src/ui/ui_*.cpp` and `ui_*.h` files
- Remove UIState/PresetPanelState from AppContext
- Update docs/modules/ui.md

**Validation:**
- [ ] Clean build with no raygui references
- [ ] Layout persists in audiojones_layout.ini
- [ ] All panels dock/undock correctly
- [ ] No performance regression (60fps)

## Validation Checklist

### Functional
- [x] All 8 waveform configs editable
- [x] Waveform list selection works
- [x] Add/remove waveforms functional
- [x] Color mode switching (solid/rainbow)
- [x] All 29 effect parameters editable
- [x] Spectrum controls functional
- [x] Audio channel dropdown works
- [x] Beat graph displays correctly
- [x] Band meters display correctly
- [x] Preset save/load works

### Docking
- [ ] All panels dockable via drag-and-drop
- [ ] Tab bar appears when panels share dock node
- [ ] Layout saved on exit
- [ ] Layout restored on launch

### Visual
- [ ] Dark theme consistent
- [ ] Sliders responsive
- [ ] Custom widgets render correctly
- [ ] No visual glitches on resize

### Performance
- [ ] 60fps maintained
- [ ] No input lag

## References

- rlImGui: https://github.com/raylib-extras/rlImGui
- ImGui Docking: https://github.com/ocornut/imgui/wiki/Docking
- ImDrawList: https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-how-can-i-display-custom-shapes
- `src/ui/ui_panel_effects.cpp:10-90` - Effects controls to port
- `src/ui/ui_widgets.cpp:60-253` - Custom widgets to reimplement
- `src/config/effect_config.h` - EffectConfig structure
