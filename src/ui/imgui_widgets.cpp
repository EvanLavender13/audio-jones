#include "imgui.h"
#include "imgui_internal.h"
#include "ui/imgui_panels.h"
#include "render/color_config.h"

static const float HUE_HANDLE_W = 8.0f;
static const float HUE_BAR_H = 14.0f;

static ImU32 HueToColor(float hue)
{
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(hue / 360.0f, 1.0f, 1.0f, r, g, b);
    return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
}

// Dual-handle hue range slider with rainbow gradient
// Returns true if value changed
static bool HueRangeSlider(const char* label, float* hueStart, float* hueEnd)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float width = ImGui::CalcItemWidth();

    // Calculate label size
    const ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);
    const float totalHeight = HUE_BAR_H + style.FramePadding.y * 2;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect frameBb(pos, ImVec2(pos.x + width, pos.y + totalHeight));
    const ImRect totalBb(pos, ImVec2(pos.x + width + (labelSize.x > 0.0f ? style.ItemInnerSpacing.x + labelSize.x : 0.0f), pos.y + totalHeight));

    ImGui::ItemSize(totalBb, style.FramePadding.y);
    if (!ImGui::ItemAdd(totalBb, id)) {
        return false;
    }

    const float usableW = width - HUE_HANDLE_W;
    const float barY = pos.y + style.FramePadding.y;

    ImDrawList* draw = window->DrawList;

    // Draw rainbow gradient bar
    for (int i = 0; i < (int)width; i++) {
        float hue = (float)i / width * 360.0f;
        draw->AddRectFilled(
            ImVec2(pos.x + i, barY),
            ImVec2(pos.x + i + 1, barY + HUE_BAR_H),
            HueToColor(hue)
        );
    }

    // Calculate handle positions
    float leftX = pos.x + (*hueStart / 360.0f) * usableW;
    float rightX = pos.x + (*hueEnd / 360.0f) * usableW;

    // Draw selection overlay
    draw->AddRectFilled(
        ImVec2(leftX + HUE_HANDLE_W / 2, barY),
        ImVec2(rightX + HUE_HANDLE_W / 2, barY + HUE_BAR_H),
        IM_COL32(255, 255, 255, 50)
    );

    // Draw handles
    ImRect leftHandle(ImVec2(leftX, barY - 2), ImVec2(leftX + HUE_HANDLE_W, barY + HUE_BAR_H + 2));
    ImRect rightHandle(ImVec2(rightX, barY - 2), ImVec2(rightX + HUE_HANDLE_W, barY + HUE_BAR_H + 2));

    draw->AddRectFilled(leftHandle.Min, leftHandle.Max, IM_COL32(255, 255, 255, 255));
    draw->AddRect(leftHandle.Min, leftHandle.Max, IM_COL32(60, 60, 60, 255));
    draw->AddRectFilled(rightHandle.Min, rightHandle.Max, IM_COL32(255, 255, 255, 255));
    draw->AddRect(rightHandle.Min, rightHandle.Max, IM_COL32(60, 60, 60, 255));

    // Handle interaction using ImGui's active ID system
    bool changed = false;
    const ImVec2 mouse = g.IO.MousePos;

    // Check for click on handles (use frameBb, not totalBb which includes label)
    bool hovered = false;
    bool held = false;
    ImGui::ButtonBehavior(frameBb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

    // Track which handle is being dragged (stored in state storage)
    ImGuiStorage* storage = window->DC.StateStorage;
    int dragSide = storage->GetInt(id, 0);

    if (ImGui::IsItemActivated()) {
        if (leftHandle.Contains(mouse)) {
            dragSide = 1;
        } else if (rightHandle.Contains(mouse)) {
            dragSide = 2;
        } else {
            dragSide = 0;
        }
        storage->SetInt(id, dragSide);
    }

    if (ImGui::IsItemActive() && dragSide != 0) {
        float newHue = ((mouse.x - pos.x - HUE_HANDLE_W / 2) / usableW) * 360.0f;
        newHue = ImClamp(newHue, 0.0f, 360.0f);

        if (dragSide == 1 && newHue <= *hueEnd) {
            *hueStart = newHue;
            changed = true;
        } else if (dragSide == 2 && newHue >= *hueStart) {
            *hueEnd = newHue;
            changed = true;
        }
    }

    if (ImGui::IsItemDeactivated()) {
        storage->SetInt(id, 0);
    }

    // Draw label
    if (labelSize.x > 0.0f) {
        ImGui::RenderText(ImVec2(frameBb.Max.x + style.ItemInnerSpacing.x, frameBb.Min.y + style.FramePadding.y), label);
    }

    return changed;
}

void ImGuiDrawColorMode(ColorConfig* color)
{
    const char* modes[] = {"Solid", "Rainbow"};
    int mode = (int)color->mode;
    if (ImGui::Combo("Mode", &mode, modes, 2)) {
        color->mode = (ColorMode)mode;
    }

    if (color->mode == COLOR_MODE_SOLID) {
        float col[4] = {
            color->solid.r / 255.0f,
            color->solid.g / 255.0f,
            color->solid.b / 255.0f,
            color->solid.a / 255.0f
        };

        // Small color button that opens picker popup
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar |
                                    ImGuiColorEditFlags_AlphaPreview |
                                    ImGuiColorEditFlags_PickerHueBar;

        if (ImGui::ColorEdit4("Color", col, flags)) {
            color->solid.r = (unsigned char)(col[0] * 255);
            color->solid.g = (unsigned char)(col[1] * 255);
            color->solid.b = (unsigned char)(col[2] * 255);
            color->solid.a = (unsigned char)(col[3] * 255);
        }
    } else {
        float hueEnd = color->rainbowHue + color->rainbowRange;
        if (hueEnd > 360.0f) {
            hueEnd = 360.0f;
        }

        if (HueRangeSlider("Hue Range", &color->rainbowHue, &hueEnd)) {
            color->rainbowRange = hueEnd - color->rainbowHue;
        }

        ImGui::SliderFloat("Saturation", &color->rainbowSat, 0.0f, 1.0f);
        ImGui::SliderFloat("Brightness", &color->rainbowVal, 0.0f, 1.0f);
    }
}
