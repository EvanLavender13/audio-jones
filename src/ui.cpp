#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "ui.h"
#include "ui_widgets.h"
#include "spectrum_config.h"
#include <stdlib.h>
#include <math.h>

// UI State

struct UIState {
    // Waveform panel state
    int waveformScrollIndex;
    bool colorModeDropdownOpen;
    bool spectrumColorModeDropdownOpen;
    bool channelModeDropdownOpen;
    int hueRangeDragging;  // 0=none, 1=left handle, 2=right handle

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
    free(state);
}

// Preset colors for new waveforms
static const Color presetColors[] = {
    {255, 255, 255, 255},  // White
    {230, 41, 55, 255},    // Red
    {0, 228, 48, 255},     // Green
    {0, 121, 241, 255},    // Blue
    {253, 249, 0, 255},    // Yellow
    {255, 0, 255, 255},    // Magenta
    {255, 161, 0, 255},    // Orange
    {102, 191, 255, 255}   // Sky blue
};

// Draws color controls for ColorConfig. Returns dropdown rect for deferred z-order drawing.
static Rectangle UIDrawColorControls(UILayout* l, UIState* state, ColorConfig* color)
{
    const int rowH = 20;
    const int colorPickerSize = 62;
    const float labelRatio = 0.38f;

    // Color mode dropdown (reserve space, return rect for deferred draw)
    UILayoutRow(l, rowH);
    DrawText("Mode", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    Rectangle dropdownRect = UILayoutSlot(l, 1.0f);

    // Disable controls behind dropdown when open
    bool anyDropdownOpen = state->colorModeDropdownOpen ||
                           state->spectrumColorModeDropdownOpen ||
                           state->channelModeDropdownOpen;
    if (anyDropdownOpen) {
        GuiSetState(STATE_DISABLED);
    }

    if (color->mode == COLOR_MODE_SOLID) {
        UILayoutRow(l, colorPickerSize);
        DrawText("Color", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        const Rectangle colorSlot = UILayoutSlot(l, 1.0f);
        GuiColorPicker({colorSlot.x, colorSlot.y, colorSlot.width - 24, colorSlot.height}, NULL, &color->solid);

        UILayoutRow(l, rowH);
        DrawText("Alpha", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        float alpha = color->solid.a / 255.0f;
        GuiColorBarAlpha(UILayoutSlot(l, 1.0f), NULL, &alpha);
        color->solid.a = (unsigned char)(alpha * 255.0f);
    } else {
        // Rainbow controls
        UILayoutRow(l, rowH);
        DrawText("Hue", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        float hueEnd = fminf(color->rainbowHue + color->rainbowRange, 360.0f);
        if (!anyDropdownOpen) {
            GuiHueRangeSlider(UILayoutSlot(l, 1.0f), &color->rainbowHue, &hueEnd, &state->hueRangeDragging);
            color->rainbowRange = hueEnd - color->rainbowHue;
        } else {
            int noDrag = 0;
            GuiHueRangeSlider(UILayoutSlot(l, 1.0f), &color->rainbowHue, &hueEnd, &noDrag);
        }

        UILayoutRow(l, rowH);
        DrawText("Sat", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &color->rainbowSat, 0.0f, 1.0f);

        UILayoutRow(l, rowH);
        DrawText("Bright", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &color->rainbowVal, 0.0f, 1.0f);
    }

    if (anyDropdownOpen) {
        GuiSetState(STATE_NORMAL);
    }

    return dropdownRect;
}

// Draws spectrum bar controls. Returns dropdown rect for deferred z-order drawing.
static Rectangle UIDrawSpectrumControls(UILayout* l, UIState* state, SpectrumConfig* config)
{
    const int rowH = 20;
    const float labelRatio = 0.38f;

    UILayoutGroupBegin(l, NULL);

    // Enable toggle
    UILayoutRow(l, rowH);
    GuiCheckBox(UILayoutSlot(l, 1.0f), "Enabled", &config->enabled);

    // Disable controls if any dropdown is open
    bool anyDropdownOpen = state->colorModeDropdownOpen ||
                           state->spectrumColorModeDropdownOpen ||
                           state->channelModeDropdownOpen;
    if (anyDropdownOpen) {
        GuiSetState(STATE_DISABLED);
    }

    // Geometry sliders
    UILayoutRow(l, rowH);
    DrawText("Radius", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->innerRadius, 0.05f, 0.4f);

    UILayoutRow(l, rowH);
    DrawText("Height", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->barHeight, 0.1f, 0.5f);

    UILayoutRow(l, rowH);
    DrawText("Width", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->barWidth, 0.3f, 1.0f);

    // Dynamics
    UILayoutRow(l, rowH);
    DrawText("Smooth", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->smoothing, 0.0f, 0.95f);

    UILayoutRow(l, rowH);
    DrawText("Min dB", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->minDb, 0.0f, 40.0f);

    UILayoutRow(l, rowH);
    DrawText("Max dB", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->maxDb, 20.0f, 60.0f);

    // Rotation
    UILayoutRow(l, rowH);
    DrawText(TextFormat("Rot %.3f", config->rotationSpeed), l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->rotationSpeed, -0.05f, 0.05f);

    UILayoutRow(l, rowH);
    DrawText("Offset", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->rotationOffset, 0.0f, 2.0f * PI);

    if (anyDropdownOpen) {
        GuiSetState(STATE_NORMAL);
    }

    // Color controls (reuse extracted function)
    Rectangle dropdownRect = UIDrawColorControls(l, state, &config->color);

    UILayoutGroupEnd(l);
    return dropdownRect;
}

static void DrawWaveformListGroup(UILayout* l, UIState* state, WaveformConfig* waveforms,
                                   int* waveformCount, int* selectedWaveform)
{
    const int rowH = 20;
    const int listHeight = 80;

    UILayoutGroupBegin(l, NULL);

    UILayoutRow(l, rowH);
    GuiSetState((*waveformCount >= MAX_WAVEFORMS) ? STATE_DISABLED : STATE_NORMAL);
    if (GuiButton(UILayoutSlot(l, 1.0f), "New") != 0) {
        waveforms[*waveformCount] = WaveformConfig{};
        waveforms[*waveformCount].color.solid = presetColors[*waveformCount % 8];
        *selectedWaveform = *waveformCount;
        (*waveformCount)++;
    }
    GuiSetState(STATE_NORMAL);

    UILayoutRow(l, listHeight);
    static char itemNames[MAX_WAVEFORMS][16];
    const char* listItems[MAX_WAVEFORMS];
    for (int i = 0; i < *waveformCount; i++) {
        (void)snprintf(itemNames[i], sizeof(itemNames[i]), "Waveform %d", i + 1);
        listItems[i] = itemNames[i];
    }
    int focus = -1;
    GuiListViewEx(UILayoutSlot(l, 1.0f), listItems, *waveformCount,
                  &state->waveformScrollIndex, selectedWaveform, &focus);

    UILayoutGroupEnd(l);
}

static Rectangle DrawWaveformSettingsGroup(UILayout* l, UIState* state,
                                            WaveformConfig* sel, int selectedIndex)
{
    const int rowH = 20;
    const float labelRatio = 0.38f;

    UILayoutGroupBegin(l, TextFormat("Waveform %d", selectedIndex + 1));

    UILayoutRow(l, rowH);
    DrawText("Radius", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->radius, 0.05f, 0.45f);

    UILayoutRow(l, rowH);
    DrawText("Height", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->amplitudeScale, 0.05f, 0.5f);

    UILayoutRow(l, rowH);
    DrawText("Thickness", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    float thicknessFloat = (float)sel->thickness;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &thicknessFloat, 1.0f, 25.0f);
    sel->thickness = lroundf(thicknessFloat);

    UILayoutRow(l, rowH);
    DrawText("Smooth", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->smoothness, 0.0f, 100.0f);

    UILayoutRow(l, rowH);
    DrawText(TextFormat("Rot %.3f", sel->rotationSpeed), l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->rotationSpeed, -0.05f, 0.05f);

    UILayoutRow(l, rowH);
    DrawText("Offset", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->rotationOffset, 0.0f, 2.0f * PI);

    // Color controls (reuse extracted function)
    Rectangle dropdownRect = UIDrawColorControls(l, state, &sel->color);

    UILayoutGroupEnd(l);
    return dropdownRect;
}

static void DrawEffectsGroup(UILayout* l, UIState* state, EffectsConfig* effects, BeatDetector* beat)
{
    const int rowH = 20;
    const float labelRatio = 0.38f;

    // Disable controls if any dropdown is open
    bool anyDropdownOpen = state->colorModeDropdownOpen ||
                           state->spectrumColorModeDropdownOpen ||
                           state->channelModeDropdownOpen;
    if (anyDropdownOpen) {
        GuiSetState(STATE_DISABLED);
    }

    UILayoutGroupBegin(l, NULL);

    // Blur scale (int, 0-4 pixels)
    UILayoutRow(l, rowH);
    DrawText("Blur", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    float blurFloat = (float)effects->baseBlurScale;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &blurFloat, 0.0f, 4.0f);
    effects->baseBlurScale = lroundf(blurFloat);

    UILayoutRow(l, rowH);
    DrawText("Half-life", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &effects->halfLife, 0.1f, 2.0f);

    // Beat sensitivity slider (threshold = mean + N × stddev)
    UILayoutRow(l, rowH);
    DrawText("Sens", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &effects->beatSensitivity, 1.0f, 3.0f);

    // Beat blur scale (int, 0-5 pixels)
    UILayoutRow(l, rowH);
    DrawText("Bloom", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    float bloomFloat = (float)effects->beatBlurScale;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &bloomFloat, 0.0f, 5.0f);
    effects->beatBlurScale = lroundf(bloomFloat);

    UILayoutRow(l, rowH);
    DrawText("Chroma", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    float chromaFloat = (float)effects->chromaticMaxOffset;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &chromaFloat, 0.0f, 50.0f);
    effects->chromaticMaxOffset = lroundf(chromaFloat);

    UILayoutRow(l, 40);
    GuiBeatGraph(UILayoutSlot(l, 1.0f), beat->graphHistory, BEAT_GRAPH_SIZE, beat->graphIndex);

    UILayoutGroupEnd(l);

    if (anyDropdownOpen) {
        GuiSetState(STATE_NORMAL);
    }
}

static Rectangle DrawAudioGroup(UILayout* l, UIState* state, AudioConfig* /* audio */)
{
    const int rowH = 20;
    const float labelRatio = 0.38f;

    // Disable controls if any dropdown is open
    bool anyDropdownOpen = state->colorModeDropdownOpen ||
                           state->spectrumColorModeDropdownOpen ||
                           state->channelModeDropdownOpen;
    if (anyDropdownOpen) {
        GuiSetState(STATE_DISABLED);
    }

    UILayoutGroupBegin(l, NULL);

    UILayoutRow(l, rowH);
    DrawText("Channel", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    Rectangle dropdownRect = UILayoutSlot(l, 1.0f);

    UILayoutGroupEnd(l);

    if (anyDropdownOpen) {
        GuiSetState(STATE_NORMAL);
    }

    return dropdownRect;
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
        DrawWaveformListGroup(&l, state, waveforms, waveformCount, selectedWaveform);
        WaveformConfig* sel = &waveforms[*selectedWaveform];
        colorDropdownRect = DrawWaveformSettingsGroup(&l, state, sel, *selectedWaveform);
    }

    // Spectrum section (accordion)
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f),
              state->spectrumSectionExpanded ? "[-] Spectrum" : "[+] Spectrum",
              &state->spectrumSectionExpanded);
    if (state->spectrumSectionExpanded) {
        spectrumColorDropdownRect = UIDrawSpectrumControls(&l, state, spectrum);
    }

    // Audio section (accordion)
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f),
              state->audioSectionExpanded ? "[-] Audio" : "[+] Audio",
              &state->audioSectionExpanded);
    if (state->audioSectionExpanded) {
        channelDropdownRect = DrawAudioGroup(&l, state, audio);
    }

    // Effects section (accordion)
    UILayoutRow(&l, 20);
    GuiToggle(UILayoutSlot(&l, 1.0f),
              state->effectsSectionExpanded ? "[-] Effects" : "[+] Effects",
              &state->effectsSectionExpanded);
    if (state->effectsSectionExpanded) {
        DrawEffectsGroup(&l, state, effects, beat);
    }

    // Draw dropdowns last so they appear on top when open
    WaveformConfig* sel = &waveforms[*selectedWaveform];
    if (state->waveformSectionExpanded && colorDropdownRect.width > 0) {
        int colorMode = (int)sel->color.mode;
        if (GuiDropdownBox(colorDropdownRect, "Solid;Rainbow", &colorMode, state->colorModeDropdownOpen) != 0) {
            state->colorModeDropdownOpen = !state->colorModeDropdownOpen;
        }
        sel->color.mode = (ColorMode)colorMode;
    }

    if (state->spectrumSectionExpanded && spectrumColorDropdownRect.width > 0) {
        int spectrumColorMode = (int)spectrum->color.mode;
        if (GuiDropdownBox(spectrumColorDropdownRect, "Solid;Rainbow", &spectrumColorMode, state->spectrumColorModeDropdownOpen) != 0) {
            state->spectrumColorModeDropdownOpen = !state->spectrumColorModeDropdownOpen;
        }
        spectrum->color.mode = (ColorMode)spectrumColorMode;
    }

    if (state->audioSectionExpanded && channelDropdownRect.width > 0) {
        int channelMode = (int)audio->channelMode;
        if (GuiDropdownBox(channelDropdownRect, "Left;Right;Max;Mix;Side;Interleaved",
                           &channelMode, state->channelModeDropdownOpen) != 0) {
            state->channelModeDropdownOpen = !state->channelModeDropdownOpen;
        }
        audio->channelMode = (ChannelMode)channelMode;
    }

    return l.y;
}
