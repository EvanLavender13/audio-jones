#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "ui_main.h"
#include "ui_common.h"
#include "ui_color.h"
#include "ui_widgets.h"
#include "ui_panel_effects.h"
#include "ui_panel_audio.h"
#include "ui_panel_spectrum.h"
#include "ui_panel_waveform.h"
#include "ui_panel_analysis.h"
#include <stdlib.h>

// UI State

struct UIState {
    // Dropdown coordination (shared across panels)
    PanelState panel;

    // Waveform panel state
    WaveformPanelState* waveformPanel;

    // Accordion section expansion state
    bool analysisSectionExpanded;
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
        state->analysisSectionExpanded = false;
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

int UIDrawWaveformPanel(UIState* state, int startY, AppConfigs* configs)
{
    UILayout l = UILayoutBegin(10, startY, 180, 8, 4);
    Rectangle colorDropdownRect = {0};
    Rectangle spectrumColorDropdownRect = {0};
    Rectangle channelDropdownRect = {0};
    Rectangle lfoWaveformDropdownRect = {0};

    // Analysis section
    if (DrawAccordionHeader(&l, "Analysis", &state->analysisSectionExpanded)) {
        UIDrawAnalysisPanel(&l, configs->beat, configs->bandEnergies, configs->bands);
    }

    // Waveforms section
    if (DrawAccordionHeader(&l, "Waveforms", &state->waveformSectionExpanded)) {
        UIDrawWaveformListGroup(&l, state->waveformPanel, configs->waveforms,
                                configs->waveformCount, configs->selectedWaveform);
        // Prevent deselection (raygui sets to -1 on re-click)
        if (*configs->selectedWaveform < 0) {
            *configs->selectedWaveform = 0;
        }
        WaveformConfig* sel = &configs->waveforms[*configs->selectedWaveform];
        colorDropdownRect = UIDrawWaveformSettingsGroup(&l, &state->panel, sel, *configs->selectedWaveform);
    }

    // Spectrum section
    if (DrawAccordionHeader(&l, "Spectrum", &state->spectrumSectionExpanded)) {
        spectrumColorDropdownRect = UIDrawSpectrumPanel(&l, &state->panel, configs->spectrum);
    }

    // Audio section
    if (DrawAccordionHeader(&l, "Audio", &state->audioSectionExpanded)) {
        channelDropdownRect = UIDrawAudioPanel(&l, &state->panel, configs->audio);
    }

    // Effects section
    if (DrawAccordionHeader(&l, "Effects", &state->effectsSectionExpanded)) {
        EffectsPanelDropdowns effectsDropdowns = UIDrawEffectsPanel(&l, &state->panel, configs->effects);
        lfoWaveformDropdownRect = effectsDropdowns.lfoWaveform;
    }

    // Draw dropdowns last so they appear on top when open
    if (*configs->selectedWaveform >= 0 && *configs->selectedWaveform < *configs->waveformCount) {
        WaveformConfig* sel = &configs->waveforms[*configs->selectedWaveform];
        int colorMode = (int)sel->color.mode;
        DrawDeferredDropdown(colorDropdownRect, state->waveformSectionExpanded, "Solid;Rainbow",
                             &colorMode, &state->panel.colorModeDropdownOpen);
        sel->color.mode = (ColorMode)colorMode;
    }

    int spectrumColorMode = (int)configs->spectrum->color.mode;
    DrawDeferredDropdown(spectrumColorDropdownRect, state->spectrumSectionExpanded, "Solid;Rainbow",
                         &spectrumColorMode, &state->panel.spectrumColorModeDropdownOpen);
    configs->spectrum->color.mode = (ColorMode)spectrumColorMode;

    int channelMode = (int)configs->audio->channelMode;
    DrawDeferredDropdown(channelDropdownRect, state->audioSectionExpanded, "Left;Right;Max;Mix;Side;Interleaved",
                         &channelMode, &state->panel.channelModeDropdownOpen);
    configs->audio->channelMode = (ChannelMode)channelMode;

    DrawDeferredDropdown(lfoWaveformDropdownRect,
                         state->effectsSectionExpanded && configs->effects->rotationLFO.enabled,
                         "Sine;Triangle;Saw;Square;S&&H",
                         &configs->effects->rotationLFO.waveform,
                         &state->panel.lfoWaveformDropdownOpen);

    return l.y;
}
