#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "ui.h"
#include "preset.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Layout helper implementation

UILayout UILayoutBegin(int x, int y, int width, int padding, int spacing)
{
    return (UILayout){
        .x = x,
        .y = y,
        .width = width,
        .padding = padding,
        .spacing = spacing,
        .rowHeight = 0,
        .slotX = x + padding
    };
}

void UILayoutRow(UILayout* l, int height)
{
    l->y += l->rowHeight + l->spacing;
    l->rowHeight = height;
    l->slotX = l->x + l->padding;
}

Rectangle UILayoutSlot(UILayout* l, float widthRatio)
{
    int innerWidth = l->width - 2 * l->padding;
    int contentRight = l->x + l->padding + innerWidth;
    int remainingWidth = contentRight - l->slotX;

    int slotWidth;
    if (widthRatio >= 1.0f) {
        slotWidth = remainingWidth;
    } else {
        slotWidth = (int)(innerWidth * widthRatio);
    }

    Rectangle r = { (float)l->slotX, (float)l->y, (float)slotWidth, (float)l->rowHeight };
    l->slotX += slotWidth;
    return r;
}

int UILayoutEnd(UILayout* l)
{
    return l->y + l->rowHeight + l->spacing;
}

void UILayoutGroupBegin(UILayout* l, const char* title)
{
    l->groupStartY = l->y;
    l->groupTitle = title;
    l->y += 14;  // groupTitleH - space for title
}

void UILayoutGroupEnd(UILayout* l)
{
    int groupH = (l->y + l->rowHeight + l->padding) - l->groupStartY;
    GuiGroupBox((Rectangle){l->x, l->groupStartY, l->width, groupH}, l->groupTitle);
    l->y = l->groupStartY + groupH + l->spacing * 2;
    l->rowHeight = 0;
}

// UI State

struct UIState {
    // Panel layout
    int panelY;

    // Waveform panel state
    int waveformScrollIndex;
    bool colorModeDropdownOpen;
    int hueRangeDragging;  // 0=none, 1=left handle, 2=right handle

    // Preset panel state
    char presetFiles[MAX_PRESET_FILES][PRESET_PATH_MAX];
    int presetFileCount;
    int selectedPreset;
    int presetScrollIndex;
    int prevSelectedPreset;
    char presetName[PRESET_NAME_MAX];
    bool presetNameEditMode;
};

void UIBeginPanels(UIState* state, int startY)
{
    state->panelY = startY;
}

UIState* UIStateInit(void)
{
    UIState* state = calloc(1, sizeof(UIState));
    if (!state) {
        return NULL;
    }

    state->selectedPreset = -1;
    state->prevSelectedPreset = -1;
    strncpy(state->presetName, "Default", PRESET_NAME_MAX);
    state->presetFileCount = PresetListFiles("presets", state->presetFiles, MAX_PRESET_FILES);

    return state;
}

void UIStateUninit(UIState* state)
{
    free(state);
}

// Custom dual-handle range slider for hue selection
// Returns true if values changed
static bool GuiHueRangeSlider(Rectangle bounds, float* hueStart, float* hueEnd, int* dragging)
{
    const float handleW = 8.0f;
    const float barH = 6.0f;
    bool changed = false;

    // Calculate positions
    float barY = bounds.y + (bounds.height - barH) / 2;
    float usableW = bounds.width - handleW;
    float leftX = bounds.x + (*hueStart / 360.0f) * usableW;
    float rightX = bounds.x + (*hueEnd / 360.0f) * usableW;

    // Draw rainbow gradient background
    for (int i = 0; i < (int)bounds.width; i++) {
        float hue = (float)i / bounds.width * 360.0f;
        Color c = ColorFromHSV(hue, 1.0f, 0.7f);
        DrawRectangle((int)(bounds.x + i), (int)barY, 1, (int)barH, c);
    }

    // Draw selected range highlight
    DrawRectangle((int)(leftX + handleW/2), (int)barY - 1,
                  (int)(rightX - leftX), (int)barH + 2, Fade(WHITE, 0.3f));

    // Draw handles
    Rectangle leftHandle = { leftX, bounds.y, handleW, bounds.height };
    Rectangle rightHandle = { rightX, bounds.y, handleW, bounds.height };
    DrawRectangleRec(leftHandle, RAYWHITE);
    DrawRectangleRec(rightHandle, RAYWHITE);
    DrawRectangleLinesEx(leftHandle, 1, DARKGRAY);
    DrawRectangleLinesEx(rightHandle, 1, DARKGRAY);

    // Handle input
    Vector2 mouse = GetMousePosition();
    bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    if (mousePressed) {
        if (CheckCollisionPointRec(mouse, leftHandle)) {
            *dragging = 1;
        } else if (CheckCollisionPointRec(mouse, rightHandle)) {
            *dragging = 2;
        }
    }

    if (!mouseDown) {
        *dragging = 0;
    }

    if (*dragging > 0 && mouseDown) {
        float newHue = ((mouse.x - bounds.x - handleW/2) / usableW) * 360.0f;
        newHue = fmaxf(0.0f, fminf(360.0f, newHue));

        if (*dragging == 1) {
            if (newHue <= *hueEnd) {
                *hueStart = newHue;
                changed = true;
            }
        } else {
            if (newHue >= *hueStart) {
                *hueEnd = newHue;
                changed = true;
            }
        }
    }

    return changed;
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

// NOLINTNEXTLINE(readability-function-size) - cohesive UI panel, splitting fragments layout logic
void UIDrawWaveformPanel(UIState* state, WaveformConfig* waveforms,
                         int* waveformCount, int* selectedWaveform,
                         float* halfLife)
{
    const int rowH = 20;
    const int listHeight = 80;
    const int colorPickerSize = 62;
    const float labelRatio = 0.38f;

    UILayout l = UILayoutBegin(10, state->panelY, 180, 8, 4);

    // Waveforms group
    UILayoutGroupBegin(&l, "Waveforms");

    UILayoutRow(&l, rowH);
    GuiSetState((*waveformCount >= MAX_WAVEFORMS) ? STATE_DISABLED : STATE_NORMAL);
    if (GuiButton(UILayoutSlot(&l, 1.0f), "New")) {
        waveforms[*waveformCount] = WaveformConfigDefault();
        waveforms[*waveformCount].color = presetColors[*waveformCount % 8];
        *selectedWaveform = *waveformCount;
        (*waveformCount)++;
    }
    GuiSetState(STATE_NORMAL);

    UILayoutRow(&l, listHeight);
    static char itemNames[MAX_WAVEFORMS][16];
    const char* listItems[MAX_WAVEFORMS];
    for (int i = 0; i < *waveformCount; i++) {
        (void)snprintf(itemNames[i], sizeof(itemNames[i]), "Waveform %d", i + 1);
        listItems[i] = itemNames[i];
    }
    int focus = -1;
    GuiListViewEx(UILayoutSlot(&l, 1.0f), listItems, *waveformCount,
                  &state->waveformScrollIndex, selectedWaveform, &focus);

    UILayoutGroupEnd(&l);

    // Settings group
    WaveformConfig* sel = &waveforms[*selectedWaveform];
    UILayoutGroupBegin(&l, TextFormat("Waveform %d", *selectedWaveform + 1));

    UILayoutRow(&l, rowH);
    DrawText("Radius", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->radius, 0.05f, 0.45f);

    UILayoutRow(&l, rowH);
    DrawText("Height", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->amplitudeScale, 0.05f, 0.5f);

    UILayoutRow(&l, rowH);
    DrawText("Thickness", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->thickness, 1.0f, 10.0f);

    UILayoutRow(&l, rowH);
    DrawText("Smooth", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->smoothness, 0.0f, 50.0f);

    UILayoutRow(&l, rowH);
    DrawText(TextFormat("Rot %.3f", sel->rotationSpeed), l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->rotationSpeed, -0.05f, 0.05f);

    // Color mode - reserve space, draw dropdown last for z-order
    UILayoutRow(&l, rowH);
    DrawText("Mode", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    Rectangle dropdownRect = UILayoutSlot(&l, 1.0f);

    // Disable controls behind dropdown when open
    if (state->colorModeDropdownOpen) {
        GuiSetState(STATE_DISABLED);
    }

    if (sel->colorMode == COLOR_MODE_SOLID) {
        UILayoutRow(&l, colorPickerSize);
        DrawText("Color", l.x + l.padding, l.y + 4, 10, GRAY);
        (void)UILayoutSlot(&l, labelRatio);
        Rectangle colorSlot = UILayoutSlot(&l, 1.0f);
        GuiColorPicker((Rectangle){colorSlot.x, colorSlot.y, colorSlot.width - 24, colorSlot.height}, NULL, &sel->color);

        UILayoutRow(&l, rowH);
        DrawText("Alpha", l.x + l.padding, l.y + 4, 10, GRAY);
        (void)UILayoutSlot(&l, labelRatio);
        float alpha = sel->color.a / 255.0f;
        GuiColorBarAlpha(UILayoutSlot(&l, 1.0f), NULL, &alpha);
        sel->color.a = (unsigned char)(alpha * 255.0f);
    } else {
        // Hue range slider (convert between hue+range and start/end)
        UILayoutRow(&l, rowH);
        DrawText("Hue", l.x + l.padding, l.y + 4, 10, GRAY);
        (void)UILayoutSlot(&l, labelRatio);
        float hueEnd = fminf(sel->rainbowHue + sel->rainbowRange, 360.0f);
        if (!state->colorModeDropdownOpen) {
            GuiHueRangeSlider(UILayoutSlot(&l, 1.0f), &sel->rainbowHue, &hueEnd, &state->hueRangeDragging);
            sel->rainbowRange = hueEnd - sel->rainbowHue;
        } else {
            // Just draw, no interaction
            int noDrag = 0;
            GuiHueRangeSlider(UILayoutSlot(&l, 1.0f), &sel->rainbowHue, &hueEnd, &noDrag);
        }

        UILayoutRow(&l, rowH);
        DrawText("Sat", l.x + l.padding, l.y + 4, 10, GRAY);
        (void)UILayoutSlot(&l, labelRatio);
        GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->rainbowSat, 0.0f, 1.0f);

        UILayoutRow(&l, rowH);
        DrawText("Bright", l.x + l.padding, l.y + 4, 10, GRAY);
        (void)UILayoutSlot(&l, labelRatio);
        GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->rainbowVal, 0.0f, 1.0f);
    }

    if (state->colorModeDropdownOpen) {
        GuiSetState(STATE_NORMAL);
    }

    UILayoutGroupEnd(&l);

    // Trails group
    UILayoutGroupBegin(&l, "Trails");

    UILayoutRow(&l, rowH);
    DrawText("Half-life", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, halfLife, 0.1f, 2.0f);

    UILayoutGroupEnd(&l);

    // Draw dropdown last so it appears on top when open
    int mode = (int)sel->colorMode;
    if (GuiDropdownBox(dropdownRect, "Solid;Rainbow", &mode, state->colorModeDropdownOpen)) {
        state->colorModeDropdownOpen = !state->colorModeDropdownOpen;
    }
    sel->colorMode = (ColorMode)mode;

    state->panelY = l.y;
}

void UIDrawPresetPanel(UIState* state, WaveformConfig* waveforms,
                       int* waveformCount, float* halfLife)
{
    const int rowH = 20;
    const int listHeight = 48;
    const float labelRatio = 0.25f;

    UILayout l = UILayoutBegin(10, state->panelY, 180, 8, 4);

    UILayoutGroupBegin(&l, "Presets");

    // Name input
    UILayoutRow(&l, rowH);
    DrawText("Name", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    if (GuiTextBox(UILayoutSlot(&l, 1.0f), state->presetName, PRESET_NAME_MAX, state->presetNameEditMode)) {
        state->presetNameEditMode = !state->presetNameEditMode;
    }

    // Save button
    UILayoutRow(&l, rowH);
    if (GuiButton(UILayoutSlot(&l, 1.0f), "Save")) {
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, sizeof(filepath), "presets/%s.json", state->presetName);
        Preset p;
        strncpy(p.name, state->presetName, PRESET_NAME_MAX);
        p.halfLife = *halfLife;
        p.waveformCount = *waveformCount;
        for (int i = 0; i < *waveformCount; i++) {
            p.waveforms[i] = waveforms[i];
        }
        PresetSave(&p, filepath);
        state->presetFileCount = PresetListFiles("presets", state->presetFiles, MAX_PRESET_FILES);
    }

    // Preset list
    UILayoutRow(&l, listHeight);
    const char* listItems[MAX_PRESET_FILES];
    for (int i = 0; i < state->presetFileCount; i++) {
        listItems[i] = state->presetFiles[i];
    }
    int focus = -1;
    GuiListViewEx(UILayoutSlot(&l, 1.0f), listItems, state->presetFileCount,
                  &state->presetScrollIndex, &state->selectedPreset, &focus);

    UILayoutGroupEnd(&l);

    // Auto-load on selection change
    if (state->selectedPreset != state->prevSelectedPreset &&
        state->selectedPreset >= 0 &&
        state->selectedPreset < state->presetFileCount) {
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, sizeof(filepath), "presets/%s", state->presetFiles[state->selectedPreset]);
        Preset p;
        if (PresetLoad(&p, filepath)) {
            strncpy(state->presetName, p.name, PRESET_NAME_MAX);
            *halfLife = p.halfLife;
            *waveformCount = p.waveformCount;
            for (int i = 0; i < p.waveformCount; i++) {
                waveforms[i] = p.waveforms[i];
            }
        }
        state->prevSelectedPreset = state->selectedPreset;
    }

    state->panelY = l.y;
}
