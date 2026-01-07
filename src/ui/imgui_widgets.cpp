#include "imgui.h"
#include "imgui_internal.h"
#include "ui/imgui_panels.h"
#include "ui/gradient_editor.h"
#include "ui/theme.h"
#include "render/color_config.h"
#include "render/gradient.h"

static const float HUE_BAR_H = 14.0f;

// ---------------------------------------------------------------------------
// Reusable Drawing Helpers
// ---------------------------------------------------------------------------

void DrawGradientBox(ImVec2 pos, ImVec2 size, ImU32 topColor, ImU32 bottomColor, float rounding)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilledMultiColor(
        pos,
        ImVec2(pos.x + size.x, pos.y + size.y),
        topColor, topColor, bottomColor, bottomColor
    );
    if (rounding > 0.0f) {
        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      Theme::WIDGET_BORDER, rounding);
    }
}

void DrawGlow(ImVec2 pos, ImVec2 size, ImU32 glowColor, float expand)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 glowMin = ImVec2(pos.x - expand, pos.y - expand);
    const ImVec2 glowMax = ImVec2(pos.x + size.x + expand, pos.y + size.y + expand);
    draw->AddRectFilled(glowMin, glowMax, glowColor, expand);
}

void DrawGroupHeader(const char* label, ImU32 accentColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float height = 22.0f;
    const float lineY = pos.y + height - 4.0f;
    const float lineThickness = 1.5f;

    // Glow layer - soft diffuse underline
    draw->AddRectFilled(
        ImVec2(pos.x, lineY - 2.0f),
        ImVec2(pos.x + width, lineY + 4.0f),
        SetColorAlpha(accentColor, 50)
    );

    // Core accent line - crisp horizon
    draw->AddLine(
        ImVec2(pos.x, lineY),
        ImVec2(pos.x + width, lineY),
        accentColor,
        lineThickness
    );

    // Text: positioned above the line, uses accent color for punch
    const float textY = pos.y + 2.0f;
    draw->AddText(ImVec2(pos.x, textY), accentColor, label);

    // Advance cursor with built-in bottom margin
    ImGui::Dummy(ImVec2(width, height));
    ImGui::Spacing();
}

bool DrawSectionHeader(const char* label, ImU32 accentColor, bool* isOpen)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    const float lineHeight = ImGui::GetTextLineHeight();
    const float headerHeight = lineHeight + style.FramePadding.y * 2;
    const float accentBarWidth = 3.0f;

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;

    // Background with subtle gradient
    DrawGradientBox(pos, ImVec2(width, headerHeight),
                    Theme::WIDGET_BG_TOP, Theme::WIDGET_BG_BOTTOM);

    // Accent bar on left edge
    draw->AddRectFilled(
        pos,
        ImVec2(pos.x + accentBarWidth, pos.y + headerHeight),
        accentColor
    );

    // Collapse arrow
    const char* arrow = (isOpen != NULL && *isOpen) ? "-" : "+";
    const float arrowX = pos.x + accentBarWidth + style.FramePadding.x;
    draw->AddText(ImVec2(arrowX, pos.y + style.FramePadding.y),
                  Theme::TEXT_SECONDARY_U32, arrow);

    // Label text
    const float textX = arrowX + lineHeight;
    draw->AddText(ImVec2(textX, pos.y + style.FramePadding.y),
                  Theme::TEXT_PRIMARY_U32, label);

    // Border
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + headerHeight),
                  Theme::WIDGET_BORDER, 0.0f);

    // Invisible button for interaction
    ImGui::InvisibleButton(label, ImVec2(width, headerHeight));
    const bool clicked = ImGui::IsItemClicked();

    if (clicked && isOpen != NULL) {
        *isOpen = !(*isOpen);
    }

    return (isOpen != NULL) ? *isOpen : true;
}

bool DrawSectionBegin(const char* label, ImU32 accentColor, bool* isOpen)
{
    const bool open = DrawSectionHeader(label, accentColor, isOpen);
    if (open) {
        ImGui::Indent(8.0f);
        ImGui::Spacing();
    }
    return open;
}

void DrawSectionEnd(void)
{
    ImGui::Spacing();
    ImGui::Unindent(8.0f);
}

bool SliderFloatWithTooltip(const char* label, float* value, float min, float max,
                            const char* format, const char* tooltip)
{
    const bool changed = ImGui::SliderFloat(label, value, min, max, format);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return changed;
}

static ImU32 HueToColor(float hue)
{
    // NOLINTNEXTLINE(readability-isolate-declaration) - output parameters for ImGui API
    float r = 0.0f, g = 0.0f, b = 0.0f;
    ImGui::ColorConvertHSVtoRGB(hue / 360.0f, 1.0f, 1.0f, r, g, b);
    return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
}

static void DrawRainbowBar(ImDrawList* draw, ImVec2 pos, float width, float barY)
{
    for (int i = 0; i < (int)width; i++) {
        const float hue = (float)i / width * 360.0f;
        draw->AddRectFilled(
            ImVec2(pos.x + i, barY),
            ImVec2(pos.x + i + 1, barY + HUE_BAR_H),
            HueToColor(hue)
        );
    }
}

static int DetermineClickedHandle(ImVec2 mouse, ImRect leftHandle, ImRect rightHandle)
{
    if (leftHandle.Contains(mouse)) {
        return 1;
    }
    if (rightHandle.Contains(mouse)) {
        return 2;
    }
    return 0;
}

static bool UpdateDraggedHue(int dragSide, float newHue, float* hueStart, float* hueEnd)
{
    if (dragSide == 1 && newHue <= *hueEnd) {
        *hueStart = newHue;
        return true;
    }
    if (dragSide == 2 && newHue >= *hueStart) {
        *hueEnd = newHue;
        return true;
    }
    return false;
}

// Dual-handle hue range slider with rainbow gradient
// Returns true if value changed
// NOLINTNEXTLINE(readability-function-size) - UI widget with complex rendering and input handling
static bool HueRangeSlider(const char* label, float* hueStart, float* hueEnd)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float width = ImGui::CalcItemWidth();

    const ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);
    const float handleExtension = Theme::HANDLE_HEIGHT - Theme::HANDLE_OVERLAP;
    const float totalHeight = HUE_BAR_H + handleExtension + style.FramePadding.y * 2;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect frameBb(pos, ImVec2(pos.x + width, pos.y + totalHeight));
    const ImRect totalBb(pos, ImVec2(pos.x + width + (labelSize.x > 0.0f ? style.ItemInnerSpacing.x + labelSize.x : 0.0f), pos.y + totalHeight));

    ImGui::ItemSize(totalBb, style.FramePadding.y);
    if (!ImGui::ItemAdd(totalBb, id)) {
        return false;
    }

    const float usableW = width - Theme::HANDLE_WIDTH;
    const float barY = pos.y + style.FramePadding.y;
    ImDrawList* draw = window->DrawList;

    ImGuiStorage* storage = window->DC.StateStorage;
    int dragSide = storage->GetInt(id, 0);

    DrawRainbowBar(draw, pos, width, barY);

    const float leftX = pos.x + (*hueStart / 360.0f) * usableW;
    const float rightX = pos.x + (*hueEnd / 360.0f) * usableW;

    // Selection overlay
    draw->AddRectFilled(
        ImVec2(leftX + Theme::HANDLE_WIDTH / 2, barY),
        ImVec2(rightX + Theme::HANDLE_WIDTH / 2, barY + HUE_BAR_H),
        IM_COL32(255, 255, 255, 50)
    );

    // Bar glow when dragging
    if (dragSide != 0) {
        draw->AddRect(ImVec2(pos.x, barY), ImVec2(pos.x + width, barY + HUE_BAR_H),
                      Theme::GLOW_CYAN, 0.0f, 0, 1.5f);
    }

    // Handles - positioned to overlap bar and extend below
    const float handleY = barY + HUE_BAR_H - Theme::HANDLE_OVERLAP;
    const ImRect leftHandle(
        ImVec2(leftX, handleY),
        ImVec2(leftX + Theme::HANDLE_WIDTH, handleY + Theme::HANDLE_HEIGHT)
    );
    const ImRect rightHandle(
        ImVec2(rightX, handleY),
        ImVec2(rightX + Theme::HANDLE_WIDTH, handleY + Theme::HANDLE_HEIGHT)
    );

    // Determine which handle is hovered
    int hoveredSide = 0;
    if (leftHandle.Contains(g.IO.MousePos)) {
        hoveredSide = 1;
    } else if (rightHandle.Contains(g.IO.MousePos)) {
        hoveredSide = 2;
    }

    // Set cursor on handle hover
    if (hoveredSide != 0) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    // Get handle colors from hue positions
    const ImU32 leftColor = HueToColor(*hueStart);
    const ImU32 rightColor = HueToColor(*hueEnd);

    // Draw handles
    DrawInteractiveHandle(draw, leftHandle.Min, leftHandle.Max, leftColor,
                          dragSide == 1, hoveredSide == 1, Theme::HANDLE_RADIUS);
    DrawInteractiveHandle(draw, rightHandle.Min, rightHandle.Max, rightColor,
                          dragSide == 2, hoveredSide == 2, Theme::HANDLE_RADIUS);

    // Interaction
    bool hovered = false;
    bool held = false;
    ImGui::ButtonBehavior(frameBb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

    if (ImGui::IsItemActivated()) {
        dragSide = DetermineClickedHandle(g.IO.MousePos, leftHandle, rightHandle);
        storage->SetInt(id, dragSide);
    }

    bool changed = false;
    if (ImGui::IsItemActive() && dragSide != 0) {
        float newHue = ((g.IO.MousePos.x - pos.x - Theme::HANDLE_WIDTH / 2) / usableW) * 360.0f;
        newHue = ImClamp(newHue, 0.0f, 360.0f);
        changed = UpdateDraggedHue(dragSide, newHue, hueStart, hueEnd);
    }

    if (ImGui::IsItemDeactivated()) {
        storage->SetInt(id, 0);
    }

    if (labelSize.x > 0.0f) {
        ImGui::RenderText(ImVec2(frameBb.Max.x + style.ItemInnerSpacing.x, frameBb.Min.y + style.FramePadding.y), label);
    }

    return changed;
}

void ImGuiDrawColorMode(ColorConfig* color)
{
    ImGui::PushID(color);
    const char* modes[] = {"Solid", "Rainbow", "Gradient"};
    int mode = (int)color->mode;
    if (ImGui::Combo("Mode", &mode, modes, 3)) {
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
        const ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar |
                                    ImGuiColorEditFlags_AlphaPreview |
                                    ImGuiColorEditFlags_PickerHueBar;

        if (ImGui::ColorEdit4("Color", col, flags)) {
            color->solid.r = (unsigned char)(col[0] * 255);
            color->solid.g = (unsigned char)(col[1] * 255);
            color->solid.b = (unsigned char)(col[2] * 255);
            color->solid.a = (unsigned char)(col[3] * 255);
        }
    } else if (color->mode == COLOR_MODE_GRADIENT) {
        if (color->gradientStopCount == 0) {
            GradientInitDefault(color->gradientStops, &color->gradientStopCount);
        }
        GradientEditor("##gradient", color->gradientStops, &color->gradientStopCount);
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
    ImGui::PopID();
}
