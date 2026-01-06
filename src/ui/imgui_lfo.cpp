#include <stdio.h>
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/modulatable_slider.h"
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
            ImGui::Combo(waveformLabel, &configs[i].waveform, waveformNames, LFO_WAVE_COUNT);

            DrawSectionEnd();
        }

        if (i < 3) {
            ImGui::Spacing();
        }
    }

    ImGui::End();
}
