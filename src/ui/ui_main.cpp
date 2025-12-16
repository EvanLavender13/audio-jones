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

// Dropdown rectangles and visibility state for deferred rendering
struct DeferredDropdowns {
    Rectangle colorDropdown;
    Rectangle spectrumColorDropdown;
    Rectangle channelDropdown;
    Rectangle lfoWaveformDropdown;
    Rectangle physarumColorDropdown;
    bool waveformVisible;
    bool spectrumVisible;
    bool audioVisible;
    bool effectsVisible;
    bool lfoEnabled;
    bool physarumEnabled;
};

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

static void DrawAllDeferredDropdowns(PanelState* panel, DeferredDropdowns* dd, AppConfigs* configs)
{
    if (dd->waveformVisible && *configs->selectedWaveform >= 0 &&
        *configs->selectedWaveform < *configs->waveformCount) {
        WaveformConfig* sel = &configs->waveforms[*configs->selectedWaveform];
        int colorMode = (int)sel->color.mode;
        DrawDeferredDropdown(dd->colorDropdown, true, "Solid;Rainbow",
                             &colorMode, &panel->colorModeDropdownOpen);
        sel->color.mode = (ColorMode)colorMode;
    }

    int spectrumColorMode = (int)configs->spectrum->color.mode;
    DrawDeferredDropdown(dd->spectrumColorDropdown, dd->spectrumVisible, "Solid;Rainbow",
                         &spectrumColorMode, &panel->spectrumColorModeDropdownOpen);
    configs->spectrum->color.mode = (ColorMode)spectrumColorMode;

    int channelMode = (int)configs->audio->channelMode;
    DrawDeferredDropdown(dd->channelDropdown, dd->audioVisible, "Left;Right;Max;Mix;Side;Interleaved",
                         &channelMode, &panel->channelModeDropdownOpen);
    configs->audio->channelMode = (ChannelMode)channelMode;

    DrawDeferredDropdown(dd->lfoWaveformDropdown, dd->effectsVisible && dd->lfoEnabled,
                         "Sine;Triangle;Saw;Square;S&&H",
                         &configs->effects->rotationLFO.waveform,
                         &panel->lfoWaveformDropdownOpen);

    int physarumColorMode = (int)configs->effects->physarum.color.mode;
    DrawDeferredDropdown(dd->physarumColorDropdown, dd->effectsVisible && dd->physarumEnabled,
                         "Solid;Rainbow",
                         &physarumColorMode,
                         &panel->physarumColorModeDropdownOpen);
    configs->effects->physarum.color.mode = (ColorMode)physarumColorMode;
}

int UIDrawWaveformPanel(UIState* state, int startY, AppConfigs* configs)
{
    UILayout l = UILayoutBegin(10, startY, 180, 8, 4);
    DeferredDropdowns dd = {0};

    if (DrawAccordionHeader(&l, "Analysis", &state->analysisSectionExpanded)) {
        UIDrawAnalysisPanel(&l, configs->beat, configs->bandEnergies, configs->bands);
    }

    dd.waveformVisible = DrawAccordionHeader(&l, "Waveforms", &state->waveformSectionExpanded);
    if (dd.waveformVisible) {
        UIDrawWaveformListGroup(&l, state->waveformPanel, configs->waveforms,
                                configs->waveformCount, configs->selectedWaveform);
        if (*configs->selectedWaveform < 0) {
            *configs->selectedWaveform = 0;
        }
        WaveformConfig* sel = &configs->waveforms[*configs->selectedWaveform];
        dd.colorDropdown = UIDrawWaveformSettingsGroup(&l, &state->panel, sel, *configs->selectedWaveform);
    }

    dd.spectrumVisible = DrawAccordionHeader(&l, "Spectrum", &state->spectrumSectionExpanded);
    if (dd.spectrumVisible) {
        dd.spectrumColorDropdown = UIDrawSpectrumPanel(&l, &state->panel, configs->spectrum);
    }

    dd.audioVisible = DrawAccordionHeader(&l, "Audio", &state->audioSectionExpanded);
    if (dd.audioVisible) {
        dd.channelDropdown = UIDrawAudioPanel(&l, &state->panel, configs->audio);
    }

    dd.effectsVisible = DrawAccordionHeader(&l, "Effects", &state->effectsSectionExpanded);
    if (dd.effectsVisible) {
        const EffectsPanelDropdowns effectsDropdowns = UIDrawEffectsPanel(&l, &state->panel, configs->effects);
        dd.lfoWaveformDropdown = effectsDropdowns.lfoWaveform;
        dd.physarumColorDropdown = effectsDropdowns.physarumColor;
        dd.lfoEnabled = configs->effects->rotationLFO.enabled;
        dd.physarumEnabled = configs->effects->physarum.enabled;
    }

    DrawAllDeferredDropdowns(&state->panel, &dd, configs);

    return l.y;
}
