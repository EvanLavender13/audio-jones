#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "audio/audio_config.h"

void ImGuiDrawAudioPanel(AudioConfig* cfg)
{
    if (!ImGui::Begin("Audio")) {
        ImGui::End();
        return;
    }

    // Audio input header - Orange accent
    ImGui::TextColored(Theme::ACCENT_ORANGE, "Audio Input");
    ImGui::Spacing();

    const char* modes[] = {"Left", "Right", "Max", "Mix", "Side", "Interleaved"};
    int mode = (int)cfg->channelMode;
    if (ImGui::Combo("Channel", &mode, modes, 6)) {
        cfg->channelMode = (ChannelMode)mode;
    }

    ImGui::Spacing();
    ImGui::TextColored(Theme::TEXT_SECONDARY, "WASAPI Loopback");

    ImGui::End();
}
