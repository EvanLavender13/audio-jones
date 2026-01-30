#include <stdio.h>
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/modulatable_slider.h"
#include "config/lfo_config.h"
#include "automation/lfo.h"

// Persistent section open states
static bool sectionLFO[NUM_LFOS] = {false};

// Rolling history of actual LFO outputs for visualization
static const int LFO_HISTORY_SIZE = 64;
static float lfoHistory[NUM_LFOS][LFO_HISTORY_SIZE] = {};
static int lfoHistoryIndex[NUM_LFOS] = {};

// Waveform names for tooltips
static const char* waveformNames[] = {"Sine", "Triangle", "Sawtooth", "Square", "Sample & Hold", "Smooth Random"};

// Accent colors cycling through theme palette (solid for lines/fills)
static ImU32 GetLFOAccentColor(int index)
{
    return Theme::GetSectionAccent(index);
}

static const float PREVIEW_WIDTH = 140.0f;
static const float PREVIEW_HEIGHT = 36.0f;
static const float ICON_SIZE = 24.0f;

// Draw a small waveform icon for the selector
static bool DrawWaveformIcon(int lfoIndex, int waveform, bool isSelected, ImU32 accentColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImVec2(ICON_SIZE, ICON_SIZE);

    // Interaction - unique ID per LFO and waveform
    ImGui::PushID(lfoIndex * LFO_WAVE_COUNT + waveform);
    const bool clicked = ImGui::InvisibleButton("##waveicon", size);
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    // Background
    const ImU32 bgColor = isSelected ? SetColorAlpha(accentColor, 60) : Theme::WIDGET_BG_BOTTOM;
    const ImU32 borderColor = isSelected ? accentColor : (hovered ? Theme::ACCENT_CYAN_U32 : Theme::WIDGET_BORDER);
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

// Draw live output history as scrolling waveform
static void DrawLFOHistoryPreview(ImVec2 size, int lfoIndex, bool enabled, ImU32 accentColor)
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

    // Draw history from oldest to newest (scrolling left)
    const int writeIdx = lfoHistoryIndex[lfoIndex];
    ImVec2 points[LFO_HISTORY_SIZE];
    for (int i = 0; i < LFO_HISTORY_SIZE; i++) {
        // Read from oldest to newest
        const int readIdx = (writeIdx + i) % LFO_HISTORY_SIZE;
        const float value = lfoHistory[lfoIndex][readIdx];
        const float normY = (value + 1.0f) * 0.5f;
        const float t = (float)i / (float)(LFO_HISTORY_SIZE - 1);
        points[i] = ImVec2(
            graphMin.x + t * graphW,
            graphMax.y - normY * graphH
        );
    }

    // Waveform glow and line
    const ImU32 waveColor = enabled ? accentColor : Theme::TEXT_SECONDARY_U32;
    const ImU32 glowColor = SetColorAlpha(waveColor, 30);
    draw->AddPolyline(points, LFO_HISTORY_SIZE, glowColor, ImDrawFlags_None, 4.0f);
    draw->AddPolyline(points, LFO_HISTORY_SIZE, waveColor, ImDrawFlags_None, 1.5f);

    // Current value indicator at right edge
    if (enabled) {
        const int latestIdx = (writeIdx + LFO_HISTORY_SIZE - 1) % LFO_HISTORY_SIZE;
        const float currentVal = lfoHistory[lfoIndex][latestIdx];
        const float dotY = centerY - currentVal * (graphH * 0.5f);
        draw->AddCircleFilled(ImVec2(graphMax.x, dotY), 4.0f, accentColor);
        draw->AddCircle(ImVec2(graphMax.x, dotY), 4.0f, Theme::TEXT_PRIMARY_U32, 0, 1.0f);
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

    // Record current outputs to history buffers
    for (int i = 0; i < NUM_LFOS; i++) {
        lfoHistory[i][lfoHistoryIndex[i]] = states[i].currentOutput;
        lfoHistoryIndex[i] = (lfoHistoryIndex[i] + 1) % LFO_HISTORY_SIZE;
    }

    DrawGroupHeader("LFOS", Theme::ACCENT_ORANGE_U32);

    for (int i = 0; i < NUM_LFOS; i++) {
        char sectionLabel[16];
        (void)snprintf(sectionLabel, sizeof(sectionLabel), "LFO %d", i + 1);

        const ImU32 accentColor = GetLFOAccentColor(i);

        if (DrawSectionBegin(sectionLabel, Theme::GetSectionGlow(i), &sectionLFO[i])) {
            char enabledLabel[32];
            char rateLabel[32];
            char paramId[16];
            (void)snprintf(enabledLabel, sizeof(enabledLabel), "##enabled_lfo%d", i);
            (void)snprintf(rateLabel, sizeof(rateLabel), "Rate##lfo%d", i);
            (void)snprintf(paramId, sizeof(paramId), "lfo%d.rate", i + 1);

            // Row 1: Enable toggle + Rate slider
            ImGui::Checkbox(enabledLabel, &configs[i].enabled);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            ModulatableSlider(rateLabel, &configs[i].rate, paramId, "%.2f Hz", sources);

            // Row 2: Waveform icons + Preview + Output meter
            ImGui::Spacing();

            // Waveform selector icons
            for (int w = 0; w < LFO_WAVE_COUNT; w++) {
                if (w > 0) { ImGui::SameLine(0, 2.0f); }
                if (DrawWaveformIcon(i, w, configs[i].waveform == w, accentColor)) {
                    configs[i].waveform = w;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", waveformNames[w]);
                }
            }

            ImGui::SameLine(0, 8.0f);

            // Live history preview (actual output over time)
            DrawLFOHistoryPreview(
                ImVec2(PREVIEW_WIDTH, PREVIEW_HEIGHT),
                i,
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
