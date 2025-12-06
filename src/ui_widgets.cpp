#include "ui_widgets.h"
#include <math.h>

void GuiBeatGraph(Rectangle bounds, const float* history, int historySize, int currentIndex)
{
    // Background
    DrawRectangleRec(bounds, { 30, 30, 30, 255 });
    DrawRectangleLinesEx(bounds, 1, { 60, 60, 60, 255 });

    if (historySize <= 0) return;

    float barWidth = bounds.width / (float)historySize;
    float padding = 1.0f;

    for (int i = 0; i < historySize; i++) {
        // Read from circular buffer in order (oldest to newest)
        int idx = (currentIndex + i) % historySize;
        float intensity = history[idx];

        // Clamp intensity
        if (intensity < 0.0f) intensity = 0.0f;
        if (intensity > 1.0f) intensity = 1.0f;

        float barHeight = intensity * (bounds.height - 4.0f);
        float x = bounds.x + i * barWidth + padding;
        float y = bounds.y + bounds.height - 2.0f - barHeight;
        float w = barWidth - padding * 2.0f;
        if (w < 1.0f) w = 1.0f;

        // Color gradient: dim gray to bright white based on intensity
        unsigned char brightness = (unsigned char)(80 + intensity * 175);
        Color barColor = { brightness, brightness, brightness, 255 };

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
