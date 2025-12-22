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
        if (stops[i].position == 0.0f || stops[i].position == 1.0f) {
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
        GradientStop key = stops[i];
        int j = i - 1;
        while (j >= 0 && stops[j].position > key.position) {
            stops[j + 1] = stops[j];
            j--;
        }
        stops[j + 1] = key;
    }
}

static const float MIN_STOP_SPACING = 0.001f;

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

    // Don't remove endpoints
    if (stops[index].position == 0.0f || stops[index].position == 1.0f) {
        return;
    }

    for (int i = index; i < *count - 1; i++) {
        stops[i] = stops[i + 1];
    }
    (*count)--;
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
    int hoveredIdx = FindHandleAt(mouse, barPos, width, stops, *count);

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

    // Handle mouse down
    if (ImGui::IsItemActivated()) {
        // Store initial click position for drag detection
        storage->SetFloat(clickPosXKey, mouse.x);
        storage->SetFloat(clickPosYKey, mouse.y);

        if (hoveredIdx >= 0) {
            // Clicked on handle - start drag
            dragIdx = hoveredIdx;
            storage->SetInt(dragIdKey, dragIdx);
        } else if (mouse.y >= barPos.y && mouse.y <= barPos.y + BAR_HEIGHT) {
            // Clicked on bar - add new stop
            const float t = ImClamp((mouse.x - barPos.x) / width, 0.0f, 1.0f);
            const int newIdx = AddStop(stops, count, t);
            if (newIdx >= 0) {
                dragIdx = newIdx;
                storage->SetInt(dragIdKey, dragIdx);
                changed = true;
            }
        }
    }

    // Right-click to delete stop
    if (hoveredIdx >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        if (stops[hoveredIdx].position != 0.0f && stops[hoveredIdx].position != 1.0f) {
            RemoveStop(stops, count, hoveredIdx);
            changed = true;
            // Clear any active drag if we deleted the dragged stop
            if (dragIdx == hoveredIdx) {
                storage->SetInt(dragIdKey, -1);
                dragIdx = -1;
            } else if (dragIdx > hoveredIdx) {
                // Adjust drag index if we deleted a stop before it
                dragIdx = dragIdx - 1;
                storage->SetInt(dragIdKey, dragIdx);
            }
        }
    }

    // Handle dragging
    if (ImGui::IsItemActive() && dragIdx >= 0 && dragIdx < *count) {
        // Update position
        float newPos = ImClamp((mouse.x - barPos.x) / width, 0.0f, 1.0f);

        // Prevent crossing neighbors
        if (dragIdx > 0) {
            newPos = ImMax(newPos, stops[dragIdx - 1].position + 0.001f);
        }
        if (dragIdx < *count - 1) {
            newPos = ImMin(newPos, stops[dragIdx + 1].position - 0.001f);
        }

        // Endpoints are locked
        if (stops[dragIdx].position == 0.0f) {
            newPos = 0.0f;
        } else if (stops[dragIdx].position == 1.0f) {
            newPos = 1.0f;
        }

        if (stops[dragIdx].position != newPos) {
            stops[dragIdx].position = newPos;
            changed = true;
        }
    }

    // Handle mouse release
    if (ImGui::IsItemDeactivated()) {
        if (dragIdx >= 0 && dragIdx < *count) {
            // Check if it was a click (not a drag) - open color picker
            const float clickX = storage->GetFloat(clickPosXKey, 0.0f);
            const float clickY = storage->GetFloat(clickPosYKey, 0.0f);
            const float dx = mouse.x - clickX;
            const float dy = mouse.y - clickY;
            const float dragDist = sqrtf(dx * dx + dy * dy);

            if (dragDist < 5.0f) {
                popupIdx = dragIdx;
                storage->SetInt(popupIdKey, popupIdx);
                ImGui::OpenPopup("##gradient_color_popup");
            }
        }
        storage->SetInt(dragIdKey, -1);
    }

    // Color picker popup
    if (popupIdx >= 0 && popupIdx < *count) {
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
    }

    return changed;
}
