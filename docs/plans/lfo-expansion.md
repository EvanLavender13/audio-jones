# LFO Expansion to 8

Expand LFO count from 4 to 8 and reorganize the `ModSource` enum so LFOs remain contiguous when adding new audio analysis sources.

## Current State

- `src/automation/mod_sources.h:8-19` - `ModSource` enum: audio (0-3), LFO1-4 (4-7), centroid (8)
- `src/automation/mod_sources.cpp` - name/color switch statements, `ModSourcesUpdate()` takes `lfoOutputs[4]`
- `src/main.cpp:38-39` - `LFOState[4]`, `LFOConfig[4]` arrays
- `src/config/preset.h:23` - `LFOConfig lfos[4]` in Preset struct
- `src/config/preset.cpp:371-399` - LFO serialization (hardcoded 4)
- `src/ui/imgui_lfo.cpp:10,16` - section states and colors arrays (hardcoded 4)

## Phase 1: Define Constant and Reorder Enum

**Goal**: Establish `NUM_LFOS` constant and reorganize enum for future extensibility.

**Build**:
- `src/config/lfo_config.h` - add `#define NUM_LFOS 8`
- `src/automation/mod_sources.h` - reorder enum:
  ```
  MOD_SOURCE_BASS = 0,
  MOD_SOURCE_MID,
  MOD_SOURCE_TREB,
  MOD_SOURCE_BEAT,
  MOD_SOURCE_LFO1,  // 4
  MOD_SOURCE_LFO2,  // 5
  MOD_SOURCE_LFO3,  // 6
  MOD_SOURCE_LFO4,  // 7
  MOD_SOURCE_LFO5,  // 8
  MOD_SOURCE_LFO6,  // 9
  MOD_SOURCE_LFO7,  // 10
  MOD_SOURCE_LFO8,  // 11
  MOD_SOURCE_CENTROID,  // 12 (moved from 8)
  MOD_SOURCE_COUNT
  ```
- `src/automation/mod_sources.h` - change `ModSourcesUpdate` signature to `const float lfoOutputs[NUM_LFOS]`

**Done when**: Code compiles (with warnings about unused LFO5-8).

---

## Phase 2: Update mod_sources.cpp

**Goal**: Handle 8 LFOs in source updates and switch statements.

**Build**:
- `src/automation/mod_sources.cpp:35-37` - change loop from `4` to `NUM_LFOS`
- `src/automation/mod_sources.cpp:48-52` - add cases for LFO5-8 in `ModSourceGetName()`
- `src/automation/mod_sources.cpp:64-75` - extend color interpolation for 8 LFOs (index 0-7 maps to cyan→magenta gradient)

**Done when**: `ModSourceGetName()` and `ModSourceGetColor()` return valid values for all 13 sources.

---

## Phase 3: Update Modulation Slider Popup

**Goal**: Add LFO5-8 to the source selection popup.

**Build**:
- `src/ui/modulatable_slider.cpp:1` - add `#include "config/lfo_config.h"`
- `src/ui/modulatable_slider.cpp:294` - expand LFO array and split into two rows:
  ```cpp
  static const ModSource lfoSources1[] = { MOD_SOURCE_LFO1, MOD_SOURCE_LFO2, MOD_SOURCE_LFO3, MOD_SOURCE_LFO4 };
  static const ModSource lfoSources2[] = { MOD_SOURCE_LFO5, MOD_SOURCE_LFO6, MOD_SOURCE_LFO7, MOD_SOURCE_LFO8 };
  ```
- `src/ui/modulatable_slider.cpp:305-306` - draw both rows:
  ```cpp
  ImGui::TextDisabled("LFOs");
  DrawSourceButtonRow(lfoSources1, 4, selectedSource, route, paramId, hasRoute, sources, buttonWidth);
  DrawSourceButtonRow(lfoSources2, 4, selectedSource, route, paramId, hasRoute, sources, buttonWidth);
  ```

**Done when**: Clicking the modulation diamond shows all 8 LFOs in the popup.

---

## Phase 4: Expand AppContext Arrays

**Goal**: Increase LFO arrays from 4 to 8 in main application state.

**Build**:
- `src/main.cpp:38-39` - change `modLFOs[4]` and `modLFOConfigs[4]` to `[NUM_LFOS]`
- `src/main.cpp:94-97` - change LFO init loop from `4` to `NUM_LFOS`
- `src/main.cpp:100-103` - register params for lfo5-8.rate (extend loop or add 4 more lines)
- `src/main.cpp:178-181` - change LFO process loop from `4` to `NUM_LFOS`

**Done when**: 8 LFOs process each frame.

---

## Phase 5: Update Preset Serialization

**Goal**: Save/load 8 LFOs in presets.

**Build**:
- `src/config/preset.h:23` - change `lfos[4]` to `lfos[NUM_LFOS]`
- `src/config/preset.cpp:372-373` - change serialization loop from `4` to `NUM_LFOS`
- `src/config/preset.cpp:396-397` - change deserialization loop from `4` to `NUM_LFOS`
- `src/config/preset.cpp:408-409` - change default init loop from `4` to `NUM_LFOS`

**Done when**: Saving a preset includes 8 LFO configs; loading old presets (with 4 LFOs) still works (extras get defaults).

---

## Phase 6: Redesign LFO UI Panel

**Goal**: Expand to 8 LFOs and match the visual polish of the Effects panel.

**Build**:

1. `src/ui/imgui_panels.h` - add forward declaration and update signature:
```cpp
struct LFOState;
// ... later in file:
void ImGuiDrawLFOPanel(LFOConfig* configs, const LFOState* states, const ModSources* sources);
```

2. `src/main.cpp:232` - pass LFO states:
```cpp
ImGuiDrawLFOPanel(ctx->modLFOConfigs, ctx->modLFOs, &ctx->modSources);
```

3. `src/ui/imgui_lfo.cpp` - complete replacement:
```cpp
#include <stdio.h>
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/modulatable_slider.h"
#include "config/lfo_config.h"
#include "automation/lfo.h"

// Persistent section open states
static bool sectionLFO[NUM_LFOS] = {false};

// Waveform names for tooltips
static const char* waveformNames[] = {"Sine", "Triangle", "Sawtooth", "Square", "Sample & Hold", "Smooth Random"};

// Accent colors cycling through theme palette
static ImU32 GetLFOAccentColor(int index)
{
    return Theme::GetSectionGlow(index);
}

static const int WAVEFORM_SAMPLE_COUNT = 48;
static const float PREVIEW_WIDTH = 140.0f;
static const float PREVIEW_HEIGHT = 36.0f;
static const float ICON_SIZE = 24.0f;

// Draw a small waveform icon for the selector
static bool DrawWaveformIcon(int waveform, bool isSelected, ImU32 accentColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImVec2(ICON_SIZE, ICON_SIZE);

    // Interaction
    ImGui::PushID(waveform);
    const bool clicked = ImGui::InvisibleButton("##waveicon", size);
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    // Background
    ImU32 bgColor = isSelected ? SetColorAlpha(accentColor, 60) : Theme::WIDGET_BG_BOTTOM;
    ImU32 borderColor = isSelected ? accentColor : (hovered ? Theme::ACCENT_CYAN_U32 : Theme::WIDGET_BORDER);
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor, 3.0f);
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), borderColor, 3.0f);

    // Draw mini waveform (8 samples)
    const float padX = 4.0f;
    const float padY = 6.0f;
    const float graphW = size.x - padX * 2;
    const float graphH = size.y - padY * 2;
    const int sampleCount = 8;

    ImVec2 points[8];
    for (int i = 0; i < sampleCount; i++) {
        const float phase = (float)i / (float)(sampleCount - 1);
        const float value = LFOEvaluateWaveform(waveform, phase);
        const float normY = (value + 1.0f) * 0.5f;
        points[i] = ImVec2(
            pos.x + padX + phase * graphW,
            pos.y + padY + graphH - normY * graphH
        );
    }

    const ImU32 lineColor = isSelected ? accentColor : Theme::TEXT_SECONDARY_U32;
    draw->AddPolyline(points, sampleCount, lineColor, ImDrawFlags_None, 1.5f);

    return clicked;
}

// Draw the main waveform preview with live phase indicator
static void DrawLFOWaveformPreview(ImVec2 size, int waveform, float phase, float currentOutput,
                                    bool enabled, ImU32 accentColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::Dummy(size);

    const float padX = 6.0f;
    const float padY = 4.0f;
    const ImVec2 graphMin = ImVec2(pos.x + padX, pos.y + padY);
    const ImVec2 graphMax = ImVec2(pos.x + size.x - padX, pos.y + size.y - padY);
    const float graphW = graphMax.x - graphMin.x;
    const float graphH = graphMax.y - graphMin.y;

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                        Theme::WIDGET_BG_BOTTOM, 4.0f);
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                  Theme::WIDGET_BORDER, 4.0f);

    // Center line (zero crossing)
    const float centerY = graphMin.y + graphH * 0.5f;
    draw->AddLine(ImVec2(graphMin.x, centerY), ImVec2(graphMax.x, centerY),
                  Theme::GUIDE_LINE, 1.0f);

    // Sample waveform
    ImVec2 points[WAVEFORM_SAMPLE_COUNT];
    for (int i = 0; i < WAVEFORM_SAMPLE_COUNT; i++) {
        const float p = (float)i / (float)(WAVEFORM_SAMPLE_COUNT - 1);
        const float value = LFOEvaluateWaveform(waveform, p);
        const float normY = (value + 1.0f) * 0.5f;
        points[i] = ImVec2(
            graphMin.x + p * graphW,
            graphMax.y - normY * graphH
        );
    }

    // Waveform glow and line
    const ImU32 waveColor = enabled ? accentColor : Theme::TEXT_SECONDARY_U32;
    const ImU32 glowColor = SetColorAlpha(waveColor, 30);
    draw->AddPolyline(points, WAVEFORM_SAMPLE_COUNT, glowColor, ImDrawFlags_None, 4.0f);
    draw->AddPolyline(points, WAVEFORM_SAMPLE_COUNT, waveColor, ImDrawFlags_None, 1.5f);

    // Live phase playhead (only when enabled)
    if (enabled) {
        const float playX = graphMin.x + phase * graphW;
        const ImU32 playheadColor = SetColorAlpha(accentColor, 200);
        draw->AddLine(ImVec2(playX, graphMin.y), ImVec2(playX, graphMax.y), playheadColor, 2.0f);

        // Current output dot on the waveform
        const float outputY = centerY - currentOutput * (graphH * 0.5f);
        draw->AddCircleFilled(ImVec2(playX, outputY), 4.0f, accentColor);
        draw->AddCircle(ImVec2(playX, outputY), 4.0f, Theme::TEXT_PRIMARY_U32, 0, 1.0f);
    }
}

// Draw vertical output meter
static void DrawOutputMeter(float currentOutput, bool enabled, ImU32 accentColor, float height)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = 8.0f;

    ImGui::Dummy(ImVec2(width, height));

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                        Theme::WIDGET_BG_BOTTOM, 2.0f);
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + height),
                  Theme::WIDGET_BORDER, 2.0f);

    if (!enabled) { return; }

    // Center line
    const float centerY = pos.y + height * 0.5f;
    draw->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + width, centerY),
                  Theme::GUIDE_LINE, 1.0f);

    // Fill from center based on output value
    const float fillHeight = fabsf(currentOutput) * (height * 0.5f - 2.0f);
    if (currentOutput > 0.0f) {
        draw->AddRectFilled(
            ImVec2(pos.x + 1, centerY - fillHeight),
            ImVec2(pos.x + width - 1, centerY),
            accentColor, 1.0f);
    } else {
        draw->AddRectFilled(
            ImVec2(pos.x + 1, centerY),
            ImVec2(pos.x + width - 1, centerY + fillHeight),
            accentColor, 1.0f);
    }
}

void ImGuiDrawLFOPanel(LFOConfig* configs, const LFOState* states, const ModSources* sources)
{
    if (!ImGui::Begin("LFOs")) {
        ImGui::End();
        return;
    }

    DrawGroupHeader("LFOs", Theme::ACCENT_ORANGE_U32);

    for (int i = 0; i < NUM_LFOS; i++) {
        char sectionLabel[16];
        (void)snprintf(sectionLabel, sizeof(sectionLabel), "LFO %d", i + 1);

        const ImU32 accentColor = GetLFOAccentColor(i);

        if (DrawSectionBegin(sectionLabel, accentColor, &sectionLFO[i])) {
            char enabledLabel[32];
            char rateLabel[32];
            char paramId[16];
            (void)snprintf(enabledLabel, sizeof(enabledLabel), "##enabled_lfo%d", i);
            (void)snprintf(rateLabel, sizeof(rateLabel), "Rate##lfo%d", i);
            (void)snprintf(paramId, sizeof(paramId), "lfo%d.rate", i + 1);

            // Row 1: Enable toggle + Rate slider
            ImGui::Checkbox(enabledLabel, &configs[i].enabled);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 20.0f);
            ModulatableSlider(rateLabel, &configs[i].rate, paramId, "%.3f Hz", sources);

            // Row 2: Waveform icons + Preview + Output meter
            ImGui::Spacing();

            // Waveform selector icons
            for (int w = 0; w < LFO_WAVE_COUNT; w++) {
                if (w > 0) { ImGui::SameLine(0, 2.0f); }
                if (DrawWaveformIcon(w, configs[i].waveform == w, accentColor)) {
                    configs[i].waveform = w;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", waveformNames[w]);
                }
            }

            ImGui::SameLine(0, 8.0f);

            // Waveform preview with live playhead
            DrawLFOWaveformPreview(
                ImVec2(PREVIEW_WIDTH, PREVIEW_HEIGHT),
                configs[i].waveform,
                states[i].phase,
                states[i].currentOutput,
                configs[i].enabled,
                accentColor
            );

            ImGui::SameLine(0, 4.0f);

            // Output meter
            DrawOutputMeter(states[i].currentOutput, configs[i].enabled, accentColor, PREVIEW_HEIGHT);

            DrawSectionEnd();
        }

        if (i < NUM_LFOS - 1) {
            ImGui::Spacing();
        }
    }

    ImGui::End();
}
```

**Done when**: LFO panel shows 8 sections with group header, live animated waveform previews with playhead, visual waveform selector icons, and output meter.

---

## Phase 7: Build and Test

**Goal**: Verify all changes work correctly.

**Build**:
- Run `cmake --build build`
- Fix any compiler errors
- Load existing preset (WOBBYBOB uses LFO1/LFO2) - verify routes still work
- Enable LFO5-8, assign to parameters, verify modulation applies

**Done when**: Build succeeds, old presets work, new LFOs modulate parameters.
