#include "ui_widgets.h"
#include <math.h>

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
