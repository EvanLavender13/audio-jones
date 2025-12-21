#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "raylib.h"
#include "ui_layout.h"
#include <stdbool.h>

typedef struct WindowState {
    Vector2 position;
    Vector2 size;
    Vector2 scroll;
    bool visible;
    int contentHeight;
    int zOrder;  // Higher values drawn on top
} WindowState;

// Begin window with scroll panel. Returns false if closed/hidden.
// On success, outLayout is initialized for content drawing.
bool UIWindowBegin(WindowState* win, const char* title, UILayout* outLayout);

// End window, measure content height, close scissor mode.
void UIWindowEnd(WindowState* win, UILayout* layout);

// Check if mouse is over window (for input blocking).
bool UIWindowIsHovered(WindowState* win);

// Check if any window is currently being dragged.
bool UIWindowIsDragging(void);

// Bring window to front (highest z-order).
void UIWindowBringToFront(WindowState* win);

// Set which window should receive input (topmost under mouse). NULL to allow all.
void UIWindowSetActiveInput(WindowState* win);

// Find topmost window under mouse from an array of window pointers.
WindowState* UIWindowFindTopmost(WindowState** windows, int count);

// Check if mouse is over any window (call UIWindowUpdateHoverState first).
bool UIWindowAnyHovered(void);

// Update global hover state for the frame (call once before drawing UI).
void UIWindowUpdateHoverState(WindowState** windows, int count);

#endif // UI_WINDOW_H
