#include "raygui.h"

#include "ui_widgets.h"
#include <math.h>
#include <stdio.h>

static const int ROW_HEIGHT = 20;
static const float LABEL_RATIO = 0.38f;

// Accordion title buffer size (includes prefix and null terminator)
static const int ACCORDION_BUF_SIZE = 64;

void DrawLabeledSlider(UILayout* l, const char* label, float* value, float min, float max)
{
    UILayoutRow(l, ROW_HEIGHT);
    DrawText(label, l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, LABEL_RATIO);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, value, min, max);
}

void DrawIntSlider(UILayout* l, const char* label, int* value, int min, int max)
{
    UILayoutRow(l, ROW_HEIGHT);
    DrawText(label, l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, LABEL_RATIO);
    float floatVal = (float)*value;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &floatVal, (float)min, (float)max);
    *value = lroundf(floatVal);
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

bool GuiHueRangeSlider(Rectangle bounds, float* hueStart, float* hueEnd, int* dragging)
{
    const float handleW = 8.0f;
    const float barH = 6.0f;
    bool changed = false;

    // Calculate positions
    const float barY = bounds.y + (bounds.height - barH) / 2;
    const float usableW = bounds.width - handleW;
    const float leftX = bounds.x + (*hueStart / 360.0f) * usableW;
    const float rightX = bounds.x + (*hueEnd / 360.0f) * usableW;

    // Draw rainbow gradient background
    for (int i = 0; i < (int)bounds.width; i++) {
        const float hue = (float)i / bounds.width * 360.0f;
        const Color c = ColorFromHSV(hue, 1.0f, 0.7f);
        DrawRectangle((int)(bounds.x + i), (int)barY, 1, (int)barH, c);
    }

    // Draw selected range highlight
    DrawRectangle((int)(leftX + handleW/2), (int)barY - 1,
                  (int)(rightX - leftX), (int)barH + 2, Fade(WHITE, 0.3f));

    // Draw handles
    const Rectangle leftHandle = { leftX, bounds.y, handleW, bounds.height };
    const Rectangle rightHandle = { rightX, bounds.y, handleW, bounds.height };
    DrawRectangleRec(leftHandle, RAYWHITE);
    DrawRectangleRec(rightHandle, RAYWHITE);
    DrawRectangleLinesEx(leftHandle, 1, DARKGRAY);
    DrawRectangleLinesEx(rightHandle, 1, DARKGRAY);

    // Handle input
    const Vector2 mouse = GetMousePosition();
    const bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    const bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

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

bool DrawAccordionHeader(UILayout* l, const char* title, bool* expanded)
{
    char buf[ACCORDION_BUF_SIZE];
    (void)snprintf(buf, ACCORDION_BUF_SIZE, "%s %s", *expanded ? "[-]" : "[+]", title);

    UILayoutRow(l, ROW_HEIGHT);
    GuiToggle(UILayoutSlot(l, 1.0f), buf, expanded);
    return *expanded;
}
