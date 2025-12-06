#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include "raylib.h"

// Declarative UI layout helper - eliminates manual coordinate math
typedef struct {
    int x, y;           // Container origin
    int width;          // Container width
    int padding;        // Inner padding
    int spacing;        // Between rows
    int rowHeight;      // Current row height
    int slotX;          // Current X within row (for multi-column)
    int groupStartY;    // For deferred group box drawing
    const char* groupTitle;
} UILayout;

// Begin a layout container at position (x,y) with given dimensions
UILayout UILayoutBegin(int x, int y, int width, int padding, int spacing);

// Start a new row with specified height
void UILayoutRow(UILayout* l, int height);

// Get a slot rectangle consuming widthRatio of row (1.0 = remaining width)
Rectangle UILayoutSlot(UILayout* l, float widthRatio);

// End layout, returns final Y position
int UILayoutEnd(UILayout* l);

// Begin a labeled group box (draws box at GroupEnd)
void UILayoutGroupBegin(UILayout* l, const char* title);

// End group box, draws the box around contained rows
void UILayoutGroupEnd(UILayout* l);

#endif // UI_LAYOUT_H
