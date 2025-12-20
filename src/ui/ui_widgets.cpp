#include "raygui.h"

#include "ui_widgets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const int ROW_HEIGHT = 20;
static const float LABEL_RATIO = 0.38f;

// Accordion title buffer size (includes prefix and null terminator)
static const int ACCORDION_BUF_SIZE = 64;

static void DrawSliderValueText(Rectangle sliderRect, int yPos, const char* text)
{
    const int textWidth = MeasureText(text, 10);
    const int textX = (int)(sliderRect.x + (sliderRect.width - textWidth) / 2);
    DrawRectangle(textX - 2, yPos + 2, textWidth + 4, 14, Fade(BLACK, 0.6f));
    DrawText(text, textX, yPos + 4, 10, WHITE);
}

void DrawLabeledSlider(UILayout* l, const char* label, float* value, float min, float max,
                       const char* unit)
{
    UILayoutRow(l, ROW_HEIGHT);
    DrawText(label, l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, LABEL_RATIO);
    const Rectangle sliderRect = UILayoutSlot(l, 1.0f);
    (void)GuiSliderBar(sliderRect, NULL, NULL, value, min, max);

    char valueText[32];
    if (unit != NULL && unit[0] != '\0') {
        (void)snprintf(valueText, sizeof(valueText), "%.2f %s", *value, unit);
    } else {
        (void)snprintf(valueText, sizeof(valueText), "%.2f", *value);
    }
    DrawSliderValueText(sliderRect, l->y, valueText);
}

void DrawIntSlider(UILayout* l, const char* label, int* value, int min, int max,
                   const char* unit)
{
    UILayoutRow(l, ROW_HEIGHT);
    DrawText(label, l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, LABEL_RATIO);
    float floatVal = (float)*value;
    const Rectangle sliderRect = UILayoutSlot(l, 1.0f);
    (void)GuiSliderBar(sliderRect, NULL, NULL, &floatVal, (float)min, (float)max);
    *value = lroundf(floatVal);

    char valueText[32];
    if (unit != NULL && unit[0] != '\0') {
        (void)snprintf(valueText, sizeof(valueText), "%d %s", *value, unit);
    } else {
        (void)snprintf(valueText, sizeof(valueText), "%d", *value);
    }
    DrawSliderValueText(sliderRect, l->y, valueText);
}

void GuiBeatGraph(Rectangle bounds, const float* history, int historySize, int currentIndex)
{
    // Background
    DrawRectangleRec(bounds, { 30, 30, 30, 255 });
    DrawRectangleLinesEx(bounds, 1, { 60, 60, 60, 255 });

    if (historySize <= 0) {
        return;
    }

    const float barWidth = bounds.width / (float)historySize;
    const float padding = 1.0f;

    for (int i = 0; i < historySize; i++) {
        // Read from circular buffer in order (oldest to newest)
        const int idx = (currentIndex + i) % historySize;
        float intensity = history[idx];

        // Clamp intensity
        if (intensity < 0.0f) {
            intensity = 0.0f;
        }
        if (intensity > 1.0f) {
            intensity = 1.0f;
        }

        const float barHeight = intensity * (bounds.height - 4.0f);
        const float x = bounds.x + i * barWidth + padding;
        const float y = bounds.y + bounds.height - 2.0f - barHeight;
        float w = barWidth - padding * 2.0f;
        if (w < 1.0f) {
            w = 1.0f;
        }

        // Color gradient: dim gray to bright white based on intensity
        const unsigned char brightness = (unsigned char)(80 + intensity * 175);
        const Color barColor = { brightness, brightness, brightness, 255 };

        if (barHeight > 0.5f) {
            DrawRectangle((int)x, (int)y, (int)w, (int)barHeight, barColor);
        }
    }
}

static const float HUE_HANDLE_W = 8.0f;
static const float HUE_BAR_H = 6.0f;

static void DrawHueRangeBar(Rectangle bounds, float hueStart, float hueEnd)
{
    const float usableW = bounds.width - HUE_HANDLE_W;
    const float barY = bounds.y + (bounds.height - HUE_BAR_H) / 2;
    const float leftX = bounds.x + (hueStart / 360.0f) * usableW;
    const float rightX = bounds.x + (hueEnd / 360.0f) * usableW;

    for (int i = 0; i < (int)bounds.width; i++) {
        const float hue = (float)i / bounds.width * 360.0f;
        const Color c = ColorFromHSV(hue, 1.0f, 0.7f);
        DrawRectangle((int)(bounds.x + i), (int)barY, 1, (int)HUE_BAR_H, c);
    }

    DrawRectangle((int)(leftX + HUE_HANDLE_W/2), (int)barY - 1,
                  (int)(rightX - leftX), (int)HUE_BAR_H + 2, Fade(WHITE, 0.3f));

    const Rectangle leftHandle = { leftX, bounds.y, HUE_HANDLE_W, bounds.height };
    const Rectangle rightHandle = { rightX, bounds.y, HUE_HANDLE_W, bounds.height };
    DrawRectangleRec(leftHandle, RAYWHITE);
    DrawRectangleRec(rightHandle, RAYWHITE);
    DrawRectangleLinesEx(leftHandle, 1, DARKGRAY);
    DrawRectangleLinesEx(rightHandle, 1, DARKGRAY);
}

static bool UpdateHueRangeDrag(Rectangle bounds, float* hueStart, float* hueEnd, int* dragging)
{
    const float usableW = bounds.width - HUE_HANDLE_W;
    const float leftX = bounds.x + (*hueStart / 360.0f) * usableW;
    const float rightX = bounds.x + (*hueEnd / 360.0f) * usableW;
    const Rectangle leftHandle = { leftX, bounds.y, HUE_HANDLE_W, bounds.height };
    const Rectangle rightHandle = { rightX, bounds.y, HUE_HANDLE_W, bounds.height };

    const Vector2 mouse = GetMousePosition();
    const bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mouse, leftHandle)) {
            *dragging = 1;
        } else if (CheckCollisionPointRec(mouse, rightHandle)) {
            *dragging = 2;
        }
    }

    if (!mouseDown) {
        *dragging = 0;
        return false;
    }
    if (*dragging == 0) {
        return false;
    }

    float newHue = ((mouse.x - bounds.x - HUE_HANDLE_W/2) / usableW) * 360.0f;
    newHue = fmaxf(0.0f, fminf(360.0f, newHue));

    if (*dragging == 1 && newHue <= *hueEnd) {
        *hueStart = newHue;
        return true;
    }
    if (*dragging == 2 && newHue >= *hueStart) {
        *hueEnd = newHue;
        return true;
    }
    return false;
}

bool GuiHueRangeSlider(Rectangle bounds, float* hueStart, float* hueEnd, int* dragging)
{
    DrawHueRangeBar(bounds, *hueStart, *hueEnd);
    return UpdateHueRangeDrag(bounds, hueStart, hueEnd, dragging);
}

bool DrawAccordionHeader(UILayout* l, const char* title, bool* expanded)
{
    char buf[ACCORDION_BUF_SIZE];
    (void)snprintf(buf, ACCORDION_BUF_SIZE, "%s %s", *expanded ? "[-]" : "[+]", title);

    UILayoutRow(l, ROW_HEIGHT);
    GuiToggle(UILayoutSlot(l, 1.0f), buf, expanded);
    return *expanded;
}

void GuiBandMeter(Rectangle bounds, const BandEnergies* bands, const BandConfig* config)
{
    DrawRectangleRec(bounds, { 30, 30, 30, 255 });
    DrawRectangleLinesEx(bounds, 1, { 60, 60, 60, 255 });

    if (bands == NULL || config == NULL) {
        return;
    }

    // Layout: 3 rows for bass/mid/treb
    const float rowH = (bounds.height - 4.0f) / 3.0f;
    const float barPadding = 2.0f;
    const float labelWidth = 32.0f;

    // Normalize smoothed values by running average (self-calibrating)
    const float MIN_AVG = 1e-6f;
    float bassNorm = bands->bassSmooth / fmaxf(bands->bassAvg, MIN_AVG);
    float midNorm = bands->midSmooth / fmaxf(bands->midAvg, MIN_AVG);
    float trebNorm = bands->trebSmooth / fmaxf(bands->trebAvg, MIN_AVG);

    // Band data: normalized values and colors
    const struct {
        const char* label;
        float value;
        float sensitivity;
        Color color;
    } bandData[3] = {
        { "Bass", bassNorm, config->bassSensitivity, SKYBLUE },
        { "Mid",  midNorm,  config->midSensitivity,  WHITE },
        { "Treb", trebNorm, config->trebSensitivity, MAGENTA }
    };

    for (int i = 0; i < 3; i++) {
        const float y = bounds.y + 2.0f + i * rowH;

        // Draw label
        DrawText(bandData[i].label, (int)(bounds.x + 4), (int)(y + (rowH - 10) / 2),
                 10, GRAY);

        // Calculate bar dimensions
        const float barX = bounds.x + labelWidth;
        const float barW = bounds.width - labelWidth - 4.0f;
        const float barY = y + barPadding;
        const float barH = rowH - barPadding * 2.0f;

        // Draw bar background
        DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH,
                      { 20, 20, 20, 255 });

        // Calculate fill (value × sensitivity, clamped 0-1)
        float fill = bandData[i].value * bandData[i].sensitivity;
        if (fill < 0.0f) {
            fill = 0.0f;
        }
        if (fill > 1.0f) {
            fill = 1.0f;
        }

        // Draw filled portion
        const float fillW = fill * barW;
        if (fillW > 0.5f) {
            DrawRectangle((int)barX, (int)barY, (int)fillW, (int)barH,
                          bandData[i].color);
        }
    }
}
