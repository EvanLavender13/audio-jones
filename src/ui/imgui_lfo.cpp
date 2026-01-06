#include <stdio.h>
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/modulatable_slider.h"
#include "config/lfo_config.h"
#include "automation/lfo.h"

// Persistent section open states
static bool sectionLFO[4] = {false, false, false, false};

// Waveform names for dropdown
static const char* waveformNames[] = {"Sine", "Triangle", "Sawtooth", "Square", "Sample & Hold", "Smooth Random"};

// Accent colors cycling: cyan, magenta, orange, cyan
static const ImU32 lfoAccentColors[4] = {
    Theme::GLOW_CYAN,
    Theme::GLOW_MAGENTA,
    Theme::GLOW_ORANGE,
    Theme::GLOW_CYAN
};

static const int WAVEFORM_SAMPLE_COUNT = 32;

static void DrawLFOWaveformPreview(ImVec2 size, int waveform, ImU32 color)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::Dummy(size);

    const float padX = 4.0f;
    const float padY = 3.0f;
    const ImVec2 graphMin = ImVec2(pos.x + padX, pos.y + padY);
    const ImVec2 graphMax = ImVec2(pos.x + size.x - padX, pos.y + size.y - padY);
    const float graphW = graphMax.x - graphMin.x;
    const float graphH = graphMax.y - graphMin.y;

    // Background with subtle gradient
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                        SetColorAlpha(Theme::WIDGET_BG_BOTTOM, 200), 3.0f);
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                  Theme::WIDGET_BORDER, 3.0f);

    // Center line at y=0 (LFO output range is -1 to +1)
    const float centerY = graphMin.y + graphH * 0.5f;
    draw->AddLine(ImVec2(graphMin.x, centerY), ImVec2(graphMax.x, centerY),
                  Theme::GUIDE_LINE, 1.0f);

    // Sample waveform and build polyline
    ImVec2 points[WAVEFORM_SAMPLE_COUNT];
    for (int i = 0; i < WAVEFORM_SAMPLE_COUNT; i++) {
        const float phase = (float)i / (float)(WAVEFORM_SAMPLE_COUNT - 1);
        const float value = LFOEvaluateWaveform(waveform, phase);
        // Map -1..+1 to graphMax.y..graphMin.y (inverted Y)
        const float normY = (value + 1.0f) * 0.5f;
        points[i] = ImVec2(
            graphMin.x + phase * graphW,
            graphMax.y - normY * graphH
        );
    }

    // Draw curve with glow effect
    const ImU32 glowColor = SetColorAlpha(color, 40);
    draw->AddPolyline(points, WAVEFORM_SAMPLE_COUNT, glowColor, ImDrawFlags_None, 3.0f);
    draw->AddPolyline(points, WAVEFORM_SAMPLE_COUNT, color, ImDrawFlags_None, 1.5f);
}

void ImGuiDrawLFOPanel(LFOConfig* configs, const ModSources* sources)
{
    if (!ImGui::Begin("LFOs")) {
        ImGui::End();
        return;
    }

    for (int i = 0; i < 4; i++) {
        char sectionLabel[16];
        (void)snprintf(sectionLabel, sizeof(sectionLabel), "LFO %d", i + 1);

        if (DrawSectionBegin(sectionLabel, lfoAccentColors[i], &sectionLFO[i])) {
            char enabledLabel[32];
            char rateLabel[32];
            char waveformLabel[32];
            char paramId[16];
            (void)snprintf(enabledLabel, sizeof(enabledLabel), "Enabled##lfo%d", i);
            (void)snprintf(rateLabel, sizeof(rateLabel), "Rate##lfo%d", i);
            (void)snprintf(waveformLabel, sizeof(waveformLabel), "Waveform##lfo%d", i);
            (void)snprintf(paramId, sizeof(paramId), "lfo%d.rate", i + 1);

            ImGui::Checkbox(enabledLabel, &configs[i].enabled);
            ModulatableSlider(rateLabel, &configs[i].rate, paramId, "%.3f Hz", sources);
            ImGui::SetNextItemWidth(100.0f);
            ImGui::Combo(waveformLabel, &configs[i].waveform, waveformNames, LFO_WAVE_COUNT);
            ImGui::SameLine();
            DrawLFOWaveformPreview(ImVec2(60.0f, 28.0f), configs[i].waveform, lfoAccentColors[i]);

            DrawSectionEnd();
        }

        if (i < 3) {
            ImGui::Spacing();
        }
    }

    ImGui::End();
}
