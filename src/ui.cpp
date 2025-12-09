#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "ui.h"
#include "ui/ui_common.h"
#include "ui/ui_color.h"
#include "ui/ui_panel_effects.h"
#include "ui/ui_panel_audio.h"
#include "ui/ui_panel_spectrum.h"
#include "ui/ui_panel_waveform.h"
#include <stdlib.h>

// UI State

struct UIState {
    // Dropdown coordination (shared across panels)
    PanelState panel;

    // Waveform panel state
    WaveformPanelState* waveformPanel;

    // Accordion section expansion state
    bool waveformSectionExpanded;
    bool spectrumSectionExpanded;
    bool audioSectionExpanded;
    bool effectsSectionExpanded;
};

UIState* UIStateInit(void)
{
    UIState* state = (UIState*)calloc(1, sizeof(UIState));
    if (state != NULL) {
        state->waveformPanel = WaveformPanelInit();

        // Default expansion state
        state->waveformSectionExpanded = false;
        state->spectrumSectionExpanded = false;
        state->audioSectionExpanded = false;
        state->effectsSectionExpanded = false;
    }
    return state;
}

void UIStateUninit(UIState* state)
{
    if (state != NULL) {
        WaveformPanelUninit(state->waveformPanel);
    }
    free(state);
}

int UIDrawWaveformPanel(UIState* state, int startY, WaveformConfig* waveforms,
                        int* waveformCount, int* selectedWaveform,
                        EffectsConfig* effects, AudioConfig* audio,
                        SpectrumConfig* spectrum, BeatDetector* beat)
{
    UILayout l = UILayoutBegin(10, startY, 180, 8, 4);
    Rectangle colorDropdownRect = {0};
    Rectangle spectrumColorDropdownRect = {0};
    Rectangle channelDropdownRect = {0};

    // Waveforms section (accordion)
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f),
              state->waveformSectionExpanded ? "[-] Waveforms" : "[+] Waveforms",
              &state->waveformSectionExpanded);
    if (state->waveformSectionExpanded) {
        UIDrawWaveformListGroup(&l, state->waveformPanel, waveforms, waveformCount, selectedWaveform);
        WaveformConfig* sel = &waveforms[*selectedWaveform];
        colorDropdownRect = UIDrawWaveformSettingsGroup(&l, &state->panel, sel, *selectedWaveform);
    }

    // Spectrum section (accordion)
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f),
              state->spectrumSectionExpanded ? "[-] Spectrum" : "[+] Spectrum",
              &state->spectrumSectionExpanded);
    if (state->spectrumSectionExpanded) {
        spectrumColorDropdownRect = UIDrawSpectrumPanel(&l, &state->panel, spectrum);
    }

    // Audio section (accordion)
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f),
              state->audioSectionExpanded ? "[-] Audio" : "[+] Audio",
              &state->audioSectionExpanded);
    if (state->audioSectionExpanded) {
        channelDropdownRect = UIDrawAudioPanel(&l, &state->panel, audio);
    }

    // Effects section (accordion)
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f),
              state->effectsSectionExpanded ? "[-] Effects" : "[+] Effects",
              &state->effectsSectionExpanded);
    if (state->effectsSectionExpanded) {
        UIDrawEffectsPanel(&l, &state->panel, effects, beat);
    }

    // Draw dropdowns last so they appear on top when open
    WaveformConfig* sel = &waveforms[*selectedWaveform];
    if (state->waveformSectionExpanded && colorDropdownRect.width > 0) {
        int colorMode = (int)sel->color.mode;
        if (GuiDropdownBox(colorDropdownRect, "Solid;Rainbow", &colorMode, state->panel.colorModeDropdownOpen) != 0) {
            state->panel.colorModeDropdownOpen = !state->panel.colorModeDropdownOpen;
        }
        sel->color.mode = (ColorMode)colorMode;
    }

    if (state->spectrumSectionExpanded && spectrumColorDropdownRect.width > 0) {
        int spectrumColorMode = (int)spectrum->color.mode;
        if (GuiDropdownBox(spectrumColorDropdownRect, "Solid;Rainbow", &spectrumColorMode, state->panel.spectrumColorModeDropdownOpen) != 0) {
            state->panel.spectrumColorModeDropdownOpen = !state->panel.spectrumColorModeDropdownOpen;
        }
        spectrum->color.mode = (ColorMode)spectrumColorMode;
    }

    if (state->audioSectionExpanded && channelDropdownRect.width > 0) {
        int channelMode = (int)audio->channelMode;
        if (GuiDropdownBox(channelDropdownRect, "Left;Right;Max;Mix;Side;Interleaved",
                           &channelMode, state->panel.channelModeDropdownOpen) != 0) {
            state->panel.channelModeDropdownOpen = !state->panel.channelModeDropdownOpen;
        }
        audio->channelMode = (ChannelMode)channelMode;
    }

    return l.y;
}
