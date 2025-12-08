#include "raygui.h"
#include "ui_layout.h"
#include <stddef.h>

UILayout UILayoutBegin(int x, int y, int width, int padding, int spacing)
{
    return {
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
    const int innerWidth = l->width - 2 * l->padding;
    const int contentRight = l->x + l->padding + innerWidth;
    const int remainingWidth = contentRight - l->slotX;

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
    // Advance past any pending row before starting group
    l->y += l->rowHeight + l->spacing;
    l->rowHeight = 0;

    l->groupStartY = l->y;
    l->groupTitle = title;
    if (title != NULL) {
        l->y += 14;  // Space for title
    }
}

void UILayoutGroupEnd(UILayout* l)
{
    const int groupH = (l->y + l->rowHeight + l->padding) - l->groupStartY;
    GuiGroupBox({(float)l->x, (float)l->groupStartY, (float)l->width, (float)groupH}, l->groupTitle);
    l->y = l->groupStartY + groupH + l->spacing * 2;
    l->rowHeight = 0;
}
