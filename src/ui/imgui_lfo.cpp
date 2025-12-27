#include <stdio.h>
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "config/lfo_config.h"

// Persistent section open states
static bool sectionLFO[4] = {false, false, false, false};

// Waveform names for dropdown
static const char* waveformNames[] = {"Sine", "Triangle", "Sawtooth", "Square", "Sample & Hold"};

// Accent colors cycling: cyan, magenta, orange, cyan
static const ImU32 lfoAccentColors[4] = {
    Theme::GLOW_CYAN,
    Theme::GLOW_MAGENTA,
    Theme::GLOW_ORANGE,
    Theme::GLOW_CYAN
};

void ImGuiDrawLFOPanel(LFOConfig* configs)
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
            (void)snprintf(enabledLabel, sizeof(enabledLabel), "Enabled##lfo%d", i);
            (void)snprintf(rateLabel, sizeof(rateLabel), "Rate##lfo%d", i);
            (void)snprintf(waveformLabel, sizeof(waveformLabel), "Waveform##lfo%d", i);

            ImGui::Checkbox(enabledLabel, &configs[i].enabled);
            ImGui::SliderFloat(rateLabel, &configs[i].rate, 0.001f, 20.0f, "%.3f Hz",
                               ImGuiSliderFlags_Logarithmic);
            ImGui::Combo(waveformLabel, &configs[i].waveform, waveformNames, LFO_WAVE_COUNT);

            DrawSectionEnd();
        }

        if (i < 3) {
            ImGui::Spacing();
        }
    }

    ImGui::End();
}
