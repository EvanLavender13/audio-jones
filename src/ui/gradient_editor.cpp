#include "gradient_editor.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/theme.h"
#include "render/color_config.h"
#include "render/gradient.h"
#include <math.h>

static const float BAR_HEIGHT = 24.0f;
static const int GRADIENT_SAMPLES = 128;

static ImU32 ColorToImU32(Color c)
{
    return IM_COL32(c.r, c.g, c.b, c.a);
}

static void DrawGradientBar(ImDrawList* draw, ImVec2 pos, float width, GradientStop* stops, int count)
{
    const float stepW = width / GRADIENT_SAMPLES;

    for (int i = 0; i < GRADIENT_SAMPLES; i++) {
        const float t0 = (float)i / GRADIENT_SAMPLES;
        const float t1 = (float)(i + 1) / GRADIENT_SAMPLES;
        const Color c0 = GradientEvaluate(stops, count, t0);
        const Color c1 = GradientEvaluate(stops, count, t1);

        const ImVec2 pMin = ImVec2(pos.x + i * stepW, pos.y);
        const ImVec2 pMax = ImVec2(pos.x + (i + 1) * stepW, pos.y + BAR_HEIGHT);

        draw->AddRectFilledMultiColor(pMin, pMax,
            ColorToImU32(c0), ColorToImU32(c1),
            ColorToImU32(c1), ColorToImU32(c0));
    }

    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + BAR_HEIGHT), Theme::WIDGET_BORDER);
}

static ImRect GetHandleRect(ImVec2 barPos, float width, float position)
{
    const float handleX = barPos.x + position * width - Theme::HANDLE_WIDTH / 2;
    const float handleY = barPos.y + BAR_HEIGHT - Theme::HANDLE_OVERLAP;
    return ImRect(
        ImVec2(handleX, handleY),
        ImVec2(handleX + Theme::HANDLE_WIDTH, handleY + Theme::HANDLE_HEIGHT)
    );
}

static int FindHandleAt(ImVec2 mouse, ImVec2 barPos, float width, GradientStop* stops, int count)
{
    for (int i = 0; i < count; i++) {
        const ImRect handle = GetHandleRect(barPos, width, stops[i].position);
        if (handle.Contains(mouse)) {
            return i;
        }
    }
    return -1;
}

static bool IsEndpoint(float position)
{
    return position == 0.0f || position == 1.0f;
}

static void DrawStopHandles(ImDrawList* draw, ImVec2 barPos, float width,
                            GradientStop* stops, int count,
                            int hoveredIdx, int activeIdx)
{
    for (int i = 0; i < count; i++) {
        const ImRect handle = GetHandleRect(barPos, width, stops[i].position);
        const ImU32 fillColor = ColorToImU32(stops[i].color);

        DrawInteractiveHandle(draw, handle.Min, handle.Max, fillColor,
                              i == activeIdx, i == hoveredIdx, Theme::HANDLE_RADIUS);

        // Lock indicator for endpoints: horizontal line at top of handle
        if (IsEndpoint(stops[i].position)) {
            const float lineY = handle.Min.y + 3;
            draw->AddLine(
                ImVec2(handle.Min.x + 2, lineY),
                ImVec2(handle.Max.x - 2, lineY),
                IM_COL32(255, 255, 255, 150), 1.5f
            );
        }
    }
}

static void SortStops(GradientStop* stops, int count)
{
    for (int i = 1; i < count; i++) {
        const GradientStop key = stops[i];
        int j = i - 1;
        while (j >= 0 && stops[j].position > key.position) {
            stops[j + 1] = stops[j];
            j--;
        }
        stops[j + 1] = key;
    }
}

static const float MIN_STOP_SPACING = 0.001f;
static const float CLICK_THRESHOLD = 5.0f;

static int AddStop(GradientStop* stops, int* count, float position)
{
    if (*count >= MAX_GRADIENT_STOPS) {
        return -1;
    }

    // Prevent stops at identical or nearly-identical positions
    for (int i = 0; i < *count; i++) {
        if (fabsf(stops[i].position - position) < MIN_STOP_SPACING) {
            return -1;
        }
    }

    const Color interpolated = GradientEvaluate(stops, *count, position);
    stops[*count].position = position;
    stops[*count].color = interpolated;
    (*count)++;

    SortStops(stops, *count);

    // Find index of newly added stop
    for (int i = 0; i < *count; i++) {
        if (stops[i].position == position) {
            return i;
        }
    }
    return -1;
}

static void RemoveStop(GradientStop* stops, int* count, int index)
{
    if (index < 0 || index >= *count || *count <= 2) {
        return;
    }

    if (IsEndpoint(stops[index].position)) {
        return;
    }

    for (int i = index; i < *count - 1; i++) {
        stops[i] = stops[i + 1];
    }
    (*count)--;
}

// Handles mouse activation: clicking on handle starts drag, clicking on bar adds stop
static int HandleMouseActivation(GradientStop* stops, int* count, int hoveredIdx,
                                  ImVec2 mouse, ImVec2 barPos, float width,
                                  ImGuiStorage* storage, ImGuiID clickPosXKey,
                                  ImGuiID clickPosYKey, bool* changed)
{
    storage->SetFloat(clickPosXKey, mouse.x);
    storage->SetFloat(clickPosYKey, mouse.y);

    if (hoveredIdx >= 0) {
        return hoveredIdx;
    }

    if (mouse.y >= barPos.y && mouse.y <= barPos.y + BAR_HEIGHT) {
        const float t = ImClamp((mouse.x - barPos.x) / width, 0.0f, 1.0f);
        const int newIdx = AddStop(stops, count, t);
        if (newIdx >= 0) {
            *changed = true;
            return newIdx;
        }
    }

    return -1;
}

// Handles right-click deletion, returns adjusted dragIdx
static int HandleRightClickDelete(GradientStop* stops, int* count, int hoveredIdx,
                                   int dragIdx, bool* changed)
{
    if (!IsEndpoint(stops[hoveredIdx].position)) {
        RemoveStop(stops, count, hoveredIdx);
        *changed = true;

        if (dragIdx == hoveredIdx) {
            return -1;
        } else if (dragIdx > hoveredIdx) {
            return dragIdx - 1;
        }
    }
    return dragIdx;
}

// Updates drag position with neighbor constraints and endpoint locks
static bool UpdateDragPosition(GradientStop* stops, int count, int dragIdx,
                                float mouseX, float barX, float width)
{
    float newPos = ImClamp((mouseX - barX) / width, 0.0f, 1.0f);

    if (dragIdx > 0) {
        newPos = ImMax(newPos, stops[dragIdx - 1].position + MIN_STOP_SPACING);
    }
    if (dragIdx < count - 1) {
        newPos = ImMin(newPos, stops[dragIdx + 1].position - MIN_STOP_SPACING);
    }

    if (IsEndpoint(stops[dragIdx].position)) {
        newPos = stops[dragIdx].position;
    }

    if (stops[dragIdx].position != newPos) {
        stops[dragIdx].position = newPos;
        return true;
    }
    return false;
}

// Detects click vs drag and opens popup if click
static int DetectClickAndOpenPopup(ImGuiStorage* storage, ImGuiID clickPosXKey,
                                    ImGuiID clickPosYKey, ImVec2 mouse, int dragIdx)
{
    const float clickX = storage->GetFloat(clickPosXKey, 0.0f);
    const float clickY = storage->GetFloat(clickPosYKey, 0.0f);
    const float dx = mouse.x - clickX;
    const float dy = mouse.y - clickY;
    const float dragDist = sqrtf(dx * dx + dy * dy);

    if (dragDist < CLICK_THRESHOLD) {
        ImGui::OpenPopup("##gradient_color_popup");
        return dragIdx;
    }
    return -1;
}

// Draws color picker popup, returns true if color changed
static bool DrawColorPickerPopup(GradientStop* stops, int count, int popupIdx,
                                  ImGuiStorage* storage, ImGuiID popupIdKey)
{
    if (popupIdx < 0 || popupIdx >= count) {
        return false;
    }

    bool changed = false;
    if (ImGui::BeginPopup("##gradient_color_popup")) {
        float col[4] = {
            stops[popupIdx].color.r / 255.0f,
            stops[popupIdx].color.g / 255.0f,
            stops[popupIdx].color.b / 255.0f,
            stops[popupIdx].color.a / 255.0f
        };

        const ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar |
                                          ImGuiColorEditFlags_AlphaPreview |
                                          ImGuiColorEditFlags_PickerHueBar;

        if (ImGui::ColorPicker4("##picker", col, flags)) {
            stops[popupIdx].color.r = (unsigned char)(col[0] * 255);
            stops[popupIdx].color.g = (unsigned char)(col[1] * 255);
            stops[popupIdx].color.b = (unsigned char)(col[2] * 255);
            stops[popupIdx].color.a = (unsigned char)(col[3] * 255);
            changed = true;
        }
        ImGui::EndPopup();
    } else {
        storage->SetInt(popupIdKey, -1);
    }

    return changed;
}

bool GradientEditor(const char* label, GradientStop* stops, int* count)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float width = ImGui::CalcItemWidth();

    // Layout: gradient bar + handle extension below
    const float handleExtension = Theme::HANDLE_HEIGHT - Theme::HANDLE_OVERLAP;
    const float totalHeight = BAR_HEIGHT + handleExtension + style.FramePadding.y * 2;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect frameBb(pos, ImVec2(pos.x + width, pos.y + totalHeight));

    ImGui::ItemSize(frameBb, style.FramePadding.y);
    if (!ImGui::ItemAdd(frameBb, id)) {
        return false;
    }

    ImDrawList* draw = window->DrawList;
    const ImVec2 barPos = ImVec2(pos.x, pos.y + style.FramePadding.y);

    // State storage keys
    const ImGuiID dragIdKey = id + 1;
    const ImGuiID popupIdKey = id + 2;
    const ImGuiID clickPosXKey = id + 3;
    const ImGuiID clickPosYKey = id + 4;

    ImGuiStorage* storage = window->DC.StateStorage;
    int dragIdx = storage->GetInt(dragIdKey, -1);
    int popupIdx = storage->GetInt(popupIdKey, -1);

    // Find hovered handle
    const ImVec2 mouse = g.IO.MousePos;
    const int hoveredIdx = FindHandleAt(mouse, barPos, width, stops, *count);

    // Draw gradient bar
    DrawGradientBar(draw, barPos, width, stops, *count);

    // Bar glow when dragging
    if (dragIdx >= 0) {
        draw->AddRect(barPos, ImVec2(barPos.x + width, barPos.y + BAR_HEIGHT),
                      Theme::GLOW_CYAN, 0.0f, 0, 1.5f);
    }

    // Draw handles
    DrawStopHandles(draw, barPos, width, stops, *count, hoveredIdx, dragIdx);

    // Set cursor on handle hover
    if (hoveredIdx >= 0) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    // Interaction
    bool hovered = false;
    bool held = false;
    ImGui::ButtonBehavior(frameBb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

    bool changed = false;

    if (ImGui::IsItemActivated()) {
        dragIdx = HandleMouseActivation(stops, count, hoveredIdx, mouse, barPos, width,
                                         storage, clickPosXKey, clickPosYKey, &changed);
        storage->SetInt(dragIdKey, dragIdx);
    }

    if (hoveredIdx >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        dragIdx = HandleRightClickDelete(stops, count, hoveredIdx, dragIdx, &changed);
        storage->SetInt(dragIdKey, dragIdx);
    }

    if (ImGui::IsItemActive() && dragIdx >= 0 && dragIdx < *count) {
        changed |= UpdateDragPosition(stops, *count, dragIdx, mouse.x, barPos.x, width);
    }

    if (ImGui::IsItemDeactivated()) {
        if (dragIdx >= 0 && dragIdx < *count) {
            popupIdx = DetectClickAndOpenPopup(storage, clickPosXKey, clickPosYKey, mouse, dragIdx);
            storage->SetInt(popupIdKey, popupIdx);
        }
        storage->SetInt(dragIdKey, -1);
    }

    changed |= DrawColorPickerPopup(stops, *count, popupIdx, storage, popupIdKey);

    return changed;
}
