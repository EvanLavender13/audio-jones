#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "ui_main.h"
#include "ui_common.h"
#include "ui_color.h"
#include "ui_widgets.h"
#include "ui_window.h"
#include "ui_panel_effects.h"
#include "ui_panel_audio.h"
#include "ui_panel_spectrum.h"
#include "ui_panel_waveform.h"
#include "ui_panel_analysis.h"
#include <stdlib.h>

// Dropdown rectangles and visibility state for deferred rendering
struct DeferredDropdowns {
    Rectangle channelDropdown;
    bool audioVisible;
};

// UI State

struct UIState {
    // Dropdown coordination (shared across panels)
    PanelState panel;

    // Waveform panel state
    WaveformPanelState* waveformPanel;

    // Floating window state
    WindowState effectsWindow;
    WindowState waveformsWindow;
    WindowState spectrumWindow;

    // Accordion section expansion state
    bool analysisSectionExpanded;
    bool audioSectionExpanded;

    // Panel background height (from previous frame)
    int lastPanelHeight;
};

UIState* UIStateInit(void)
{
    // Apply flat/minimal styling
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(DARKGRAY));
    GuiSetStyle(TOGGLE, BASE_COLOR_NORMAL, ColorToInt(DARKGRAY));

    UIState* state = (UIState*)calloc(1, sizeof(UIState));
    if (state != NULL) {
        state->waveformPanel = WaveformPanelInit();

        // Effects floating window
        state->effectsWindow.position = {400, 100};
        state->effectsWindow.size = {260, 600};
        state->effectsWindow.scroll = {0, 0};
        state->effectsWindow.visible = true;
        state->effectsWindow.contentHeight = 600;
        state->effectsWindow.zOrder = 1;

        // Waveforms floating window
        state->waveformsWindow.position = {680, 100};
        state->waveformsWindow.size = {240, 400};
        state->waveformsWindow.scroll = {0, 0};
        state->waveformsWindow.visible = true;
        state->waveformsWindow.contentHeight = 400;
        state->waveformsWindow.zOrder = 2;

        // Spectrum floating window
        state->spectrumWindow.position = {200, 100};
        state->spectrumWindow.size = {220, 350};
        state->spectrumWindow.scroll = {0, 0};
        state->spectrumWindow.visible = true;
        state->spectrumWindow.contentHeight = 350;
        state->spectrumWindow.zOrder = 3;

        // Default expansion state
        state->analysisSectionExpanded = false;
        state->audioSectionExpanded = false;

        // Initial background height estimate
        state->lastPanelHeight = 300;
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

void UIUpdateWindowHoverState(UIState* state)
{
    WindowState* windows[3] = {
        &state->effectsWindow,
        &state->waveformsWindow,
        &state->spectrumWindow
    };
    UIWindowUpdateHoverState(windows, 3);
}

static void DrawAllDeferredDropdowns(PanelState* panel, DeferredDropdowns* dd, AppConfigs* configs)
{
    int channelMode = (int)configs->audio->channelMode;
    DrawDeferredDropdown(dd->channelDropdown, dd->audioVisible, "Left;Right;Max;Mix;Side;Interleaved",
                         &channelMode, &panel->channelModeDropdownOpen);
    configs->audio->channelMode = (ChannelMode)channelMode;
}

static void DrawEffectsWindow(UIState* state, AppConfigs* configs)
{
    UILayout l;
    if (!UIWindowBegin(&state->effectsWindow, "Effects", &l)) {
        return;
    }

    EffectsPanelDropdowns dd = UIDrawEffectsPanel(&l, &state->panel, configs->effects);

    // Deferred dropdowns inside scissor
    DrawDeferredDropdown(dd.lfoWaveform, configs->effects->rotationLFO.enabled,
                         "Sine;Triangle;Saw;Square;S&&H",
                         &configs->effects->rotationLFO.waveform,
                         &state->panel.lfoWaveformDropdownOpen);

    int physarumColorMode = (int)configs->effects->physarum.color.mode;
    DrawDeferredDropdown(dd.physarumColor, configs->effects->physarum.enabled,
                         "Solid;Rainbow",
                         &physarumColorMode,
                         &state->panel.physarumColorModeDropdownOpen);
    configs->effects->physarum.color.mode = (ColorMode)physarumColorMode;

    UIWindowEnd(&state->effectsWindow, &l);
}

static void DrawWaveformsWindow(UIState* state, AppConfigs* configs)
{
    UILayout l;
    if (!UIWindowBegin(&state->waveformsWindow, "Waveforms", &l)) {
        return;
    }

    UIDrawWaveformListGroup(&l, state->waveformPanel, configs->waveforms,
                            configs->waveformCount, configs->selectedWaveform);

    if (*configs->selectedWaveform >= 0 &&
        *configs->selectedWaveform < *configs->waveformCount) {
        WaveformConfig* sel = &configs->waveforms[*configs->selectedWaveform];
        Rectangle colorDropdown = UIDrawWaveformSettingsGroup(&l, &state->panel, sel,
                                                               *configs->selectedWaveform);

        int colorMode = (int)sel->color.mode;
        DrawDeferredDropdown(colorDropdown, true, "Solid;Rainbow",
                             &colorMode, &state->panel.colorModeDropdownOpen);
        sel->color.mode = (ColorMode)colorMode;
    }

    UIWindowEnd(&state->waveformsWindow, &l);
}

static void DrawSpectrumWindow(UIState* state, AppConfigs* configs)
{
    UILayout l;
    if (!UIWindowBegin(&state->spectrumWindow, "Spectrum", &l)) {
        return;
    }

    Rectangle colorDropdown = UIDrawSpectrumPanel(&l, &state->panel, configs->spectrum);

    int spectrumColorMode = (int)configs->spectrum->color.mode;
    DrawDeferredDropdown(colorDropdown, true, "Solid;Rainbow",
                         &spectrumColorMode, &state->panel.spectrumColorModeDropdownOpen);
    configs->spectrum->color.mode = (ColorMode)spectrumColorMode;

    UIWindowEnd(&state->spectrumWindow, &l);
}

// Window draw function pointer type
typedef void (*WindowDrawFunc)(UIState*, AppConfigs*);

// Window entry for z-order sorting
struct WindowEntry {
    int zOrder;
    WindowDrawFunc draw;
};

static int CompareWindowEntries(const void* a, const void* b)
{
    return ((const WindowEntry*)a)->zOrder - ((const WindowEntry*)b)->zOrder;
}

int UIDrawWaveformPanel(UIState* state, int startY, AppConfigs* configs)
{
    UILayout l = UILayoutBegin(10, startY, 180, 8, 4);

    // Draw semi-transparent background using previous frame's height
    DrawRectangleRec({10.0f, (float)startY, 180.0f, (float)state->lastPanelHeight},
                     Fade(BLACK, 0.7f));

    // Disable sidebar controls while mouse is over any floating window
    bool blockInput = UIWindowAnyHovered();
    if (blockInput) {
        GuiSetState(STATE_DISABLED);
    }

    DeferredDropdowns dd = {0};

    if (DrawAccordionHeader(&l, "Analysis", &state->analysisSectionExpanded)) {
        UIDrawAnalysisPanel(&l, configs->beat, configs->bandEnergies, configs->bands);
    }

    dd.audioVisible = DrawAccordionHeader(&l, "Audio", &state->audioSectionExpanded);
    if (dd.audioVisible) {
        dd.channelDropdown = UIDrawAudioPanel(&l, &state->panel, configs->audio);
    }

    DrawAllDeferredDropdowns(&state->panel, &dd, configs);

    // Toggle buttons for floating windows
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f), "Effects", &state->effectsWindow.visible);
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f), "Waveforms", &state->waveformsWindow.visible);
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f), "Spectrum", &state->spectrumWindow.visible);

    if (blockInput) {
        GuiSetState(STATE_NORMAL);
    }

    // Update panel height for next frame's background
    state->lastPanelHeight = l.y - startY + l.spacing;

    // Find topmost window under mouse for input blocking
    WindowState* windowPtrs[3] = {
        &state->effectsWindow,
        &state->waveformsWindow,
        &state->spectrumWindow
    };
    WindowState* topmost = UIWindowFindTopmost(windowPtrs, 3);
    UIWindowSetActiveInput(topmost);

    // Draw floating windows sorted by z-order (lowest first, highest on top)
    WindowEntry windows[3] = {
        {state->effectsWindow.zOrder, DrawEffectsWindow},
        {state->waveformsWindow.zOrder, DrawWaveformsWindow},
        {state->spectrumWindow.zOrder, DrawSpectrumWindow}
    };
    qsort(windows, 3, sizeof(WindowEntry), CompareWindowEntries);

    for (int i = 0; i < 3; i++) {
        windows[i].draw(state, configs);
    }

    // Clear active input for next frame
    UIWindowSetActiveInput(NULL);

    return l.y;
}
