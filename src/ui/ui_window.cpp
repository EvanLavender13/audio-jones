#include "ui_window.h"
#include "raygui.h"
#include <stddef.h>

// Drag state for window title bar dragging
static WindowState* draggedWindow = NULL;
static Vector2 dragOffset = {0};

// Z-order counter (incremented each time a window is brought to front)
static int zOrderCounter = 0;

// Window that should receive input (NULL means check hovered state)
static WindowState* activeInputWindow = NULL;

// Track if current window is input-disabled (for UIWindowEnd to restore)
static bool currentWindowDisabled = false;

// Global hover state for the frame
static bool anyWindowHovered = false;

bool UIWindowBegin(WindowState* win, const char* title, UILayout* outLayout)
{
    if (!win->visible) {
        return false;
    }

    Rectangle bounds = {win->position.x, win->position.y, win->size.x, win->size.y};

    // Check if this window should receive input
    bool shouldReceiveInput = (activeInputWindow == NULL || activeInputWindow == win || draggedWindow == win);
    currentWindowDisabled = !shouldReceiveInput;

    // Disable raygui input if this window is occluded by another
    if (currentWindowDisabled) {
        GuiSetState(STATE_DISABLED);
    }

    // Window frame with close button
    if (GuiWindowBox(bounds, title)) {
        if (currentWindowDisabled) {
            GuiSetState(STATE_NORMAL);
        }
        win->visible = false;
        return false;
    }

    // Restore normal state for input handling logic
    if (currentWindowDisabled) {
        GuiSetState(STATE_NORMAL);
    }

    // Handle title bar drag (top 24px is raygui's default title bar height)
    const float titleBarHeight = 24.0f;
    Rectangle titleBar = {bounds.x, bounds.y, bounds.width, titleBarHeight};

    // Bring to front and start drag on title bar click (only if receiving input)
    if (shouldReceiveInput && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(GetMousePosition(), titleBar)) {
        UIWindowBringToFront(win);
        draggedWindow = win;
        dragOffset.x = GetMousePosition().x - bounds.x;
        dragOffset.y = GetMousePosition().y - bounds.y;
    }

    // Bring to front on click anywhere in window (only if receiving input)
    if (shouldReceiveInput && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(GetMousePosition(), bounds)) {
        UIWindowBringToFront(win);
    }

    if (draggedWindow == win) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            win->position.x = GetMousePosition().x - dragOffset.x;
            win->position.y = GetMousePosition().y - dragOffset.y;
        } else {
            draggedWindow = NULL;
        }
    }

    // Re-disable for content if this window shouldn't receive input
    if (currentWindowDisabled) {
        GuiSetState(STATE_DISABLED);
    }

    // Scroll panel for content area (below title bar)
    Rectangle panelArea = {bounds.x, bounds.y + titleBarHeight, bounds.width, bounds.height - titleBarHeight};
    Rectangle contentSize = {0, 0, bounds.width - 14, (float)win->contentHeight}; // -14 for scrollbar width
    Rectangle view;
    GuiScrollPanel(panelArea, NULL, contentSize, &win->scroll, &view);

    // Begin scissor mode for content clipping
    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

    // Create layout with scroll offset applied
    // scroll.y is negative when scrolled down, so adding it moves content up
    *outLayout = UILayoutBegin(
        (int)view.x,
        (int)view.y + (int)win->scroll.y,
        (int)view.width,
        8,
        4
    );

    return true;
}

void UIWindowEnd(WindowState* win, UILayout* layout)
{
    const float titleBarHeight = 24.0f;

    // Measure content height for next frame's scroll bounds
    int layoutEnd = UILayoutEnd(layout);
    int contentStartY = (int)(win->position.y + titleBarHeight + win->scroll.y);
    win->contentHeight = layoutEnd - contentStartY;

    EndScissorMode();

    // Restore normal state if this window was disabled
    if (currentWindowDisabled) {
        GuiSetState(STATE_NORMAL);
        currentWindowDisabled = false;
    }
}

bool UIWindowIsHovered(WindowState* win)
{
    if (!win->visible) {
        return false;
    }
    Rectangle bounds = {win->position.x, win->position.y, win->size.x, win->size.y};
    return CheckCollisionPointRec(GetMousePosition(), bounds);
}

bool UIWindowIsDragging(void)
{
    return draggedWindow != NULL;
}

void UIWindowBringToFront(WindowState* win)
{
    zOrderCounter++;
    win->zOrder = zOrderCounter;
}

void UIWindowSetActiveInput(WindowState* win)
{
    activeInputWindow = win;
}

WindowState* UIWindowFindTopmost(WindowState** windows, int count)
{
    Vector2 mouse = GetMousePosition();
    WindowState* topmost = NULL;
    int highestZ = -1;

    for (int i = 0; i < count; i++) {
        WindowState* win = windows[i];
        if (!win->visible) {
            continue;
        }
        Rectangle bounds = {win->position.x, win->position.y, win->size.x, win->size.y};
        if (CheckCollisionPointRec(mouse, bounds) && win->zOrder > highestZ) {
            highestZ = win->zOrder;
            topmost = win;
        }
    }

    return topmost;
}

bool UIWindowAnyHovered(void)
{
    return anyWindowHovered || draggedWindow != NULL;
}

void UIWindowUpdateHoverState(WindowState** windows, int count)
{
    anyWindowHovered = false;
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < count; i++) {
        WindowState* win = windows[i];
        if (!win->visible) {
            continue;
        }
        Rectangle bounds = {win->position.x, win->position.y, win->size.x, win->size.y};
        if (CheckCollisionPointRec(mouse, bounds)) {
            anyWindowHovered = true;
            break;
        }
    }
}
