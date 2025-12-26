#include "modulatable_slider.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/theme.h"
#include "automation/modulation_engine.h"
#include <string>
#include <math.h>

static const float INDICATOR_SIZE = 10.0f;
static const float INDICATOR_SPACING = 4.0f;
static const float PULSE_PERIOD_MS = 800.0f;

// Draw a diamond shape (rotated square)
static void DrawDiamond(ImDrawList* draw, ImVec2 center, float size, ImU32 color, bool filled)
{
    const float half = size * 0.5f;
    ImVec2 points[4] = {
        ImVec2(center.x, center.y - half),  // top
        ImVec2(center.x + half, center.y),  // right
        ImVec2(center.x, center.y + half),  // bottom
        ImVec2(center.x - half, center.y)   // left
    };

    if (filled) {
        draw->AddConvexPolyFilled(points, 4, color);
    } else {
        draw->AddPolyline(points, 4, color, ImDrawFlags_Closed, 1.5f);
    }
}

// Get slider grab position as 0-1 within the frame
static float ValueToRatio(float value, float min, float max)
{
    return (value - min) / (max - min);
}

// Draw a row of 4 source selection buttons with live value indicators
static void DrawSourceButtonRow(ImDrawList* draw, const ModSource sources[4], int selectedSource,
                                 ModRoute* route, const char* paramId, bool* hasRoute,
                                 const ModSources* modSources, float buttonWidth)
{
    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            ImGui::SameLine();
        }

        const ModSource src = sources[i];
        const bool isSelected = (selectedSource == src);
        const ImU32 srcColor = ModSourceGetColor(src);

        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(srcColor));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(srcColor));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }

        char btnLabel[16];
        (void)snprintf(btnLabel, sizeof(btnLabel), "%s##src%d", ModSourceGetName(src), src);
        if (ImGui::Button(btnLabel, ImVec2(buttonWidth, 0))) {
            route->source = src;
            if (!(*hasRoute)) {
                route->amount = 0.5f;
                route->curve = MOD_CURVE_LINEAR;
                strncpy(route->paramId, paramId, sizeof(route->paramId) - 1);
            }
            ModEngineSetRoute(paramId, route);
            *hasRoute = true;
        }

        // Show live value indicator under button
        if (modSources != NULL) {
            const float val = modSources->values[src];
            const ImVec2 btnMin = ImGui::GetItemRectMin();
            const ImVec2 btnMax = ImGui::GetItemRectMax();
            const float barHeight = 2.0f;
            const float barWidth = (btnMax.x - btnMin.x - 4.0f) * val;
            draw->AddRectFilled(
                ImVec2(btnMin.x + 2.0f, btnMax.y - barHeight - 2.0f),
                ImVec2(btnMin.x + 2.0f + barWidth, btnMax.y - 2.0f),
                srcColor
            );
        }

        if (isSelected) {
            ImGui::PopStyleColor(3);
        }
    }
}

bool ModulatableSlider(const char* label, float* value, const char* paramId,
                       float min, float max, const char* format,
                       const ModSources* sources)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    // Register param (engine handles duplicates gracefully)
    ModEngineRegisterParam(paramId, value, min, max);

    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // Check if we have a route
    ModRoute route;
    bool hasRoute = ModEngineGetRoute(paramId, &route);

    // Draw the slider
    const bool changed = ImGui::SliderFloat(label, value, min, max, format);

    // If user dragged the slider, update base value
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ModEngineSetBase(paramId, *value);
    }

    // Get the slider frame rect (the track area, not including label)
    // ImGui::SliderFloat uses CalcItemWidth() for the frame width
    const float frameWidth = ImGui::CalcItemWidth();
    const ImVec2 frameMin = ImGui::GetItemRectMin();
    const ImVec2 frameMax = ImVec2(frameMin.x + frameWidth, ImGui::GetItemRectMax().y);
    const float frameHeight = frameMax.y - frameMin.y;

    ImDrawList* draw = window->DrawList;

    // Draw modulation visualization if routed
    if (hasRoute) {
        const float offset = ModEngineGetOffset(paramId);
        const float baseValue = *value - offset;  // Current displayed value minus offset = base
        const float modulatedValue = *value;

        // Calculate positions as ratios (0-1)
        const float baseRatio = ImClamp(ValueToRatio(baseValue, min, max), 0.0f, 1.0f);
        const float modRatio = ImClamp(ValueToRatio(modulatedValue, min, max), 0.0f, 1.0f);

        // Calculate grab width (same as ImGui's internal calculation)
        const float grabPadding = 2.0f;
        const float grabMinSize = style.GrabMinSize;
        const float sliderUsable = frameWidth - grabPadding * 2.0f;
        const float grabSz = ImMax(grabMinSize, sliderUsable * 0.05f);
        const float sliderRange = sliderUsable - grabSz;

        // X positions of base and modulated grabs (center of grab)
        const float baseXCenter = frameMin.x + grabPadding + baseRatio * sliderRange + grabSz * 0.5f;
        const float modXCenter = frameMin.x + grabPadding + modRatio * sliderRange + grabSz * 0.5f;

        // Get source color
        const ImU32 sourceColor = ModSourceGetColor((ModSource)route.source);

        // Draw track highlight between base and modulated positions
        const float highlightMinX = ImMin(baseXCenter, modXCenter);
        const float highlightMaxX = ImMax(baseXCenter, modXCenter);
        const float highlightY = frameMin.y + frameHeight * 0.35f;
        const float highlightH = frameHeight * 0.3f;

        // 20% alpha highlight
        const ImU32 highlightColor = (sourceColor & 0x00FFFFFF) | (50 << 24);
        draw->AddRectFilled(
            ImVec2(highlightMinX, highlightY),
            ImVec2(highlightMaxX, highlightY + highlightH),
            highlightColor
        );

        // Draw thin vertical marker at BASE position (shows unmodulated value)
        const float markerWidth = 2.0f;
        const float markerY = frameMin.y + 3.0f;
        const float markerH = frameHeight - 6.0f;
        const ImU32 markerColor = (sourceColor & 0x00FFFFFF) | (180 << 24);  // 70% alpha
        draw->AddRectFilled(
            ImVec2(baseXCenter - markerWidth * 0.5f, markerY),
            ImVec2(baseXCenter + markerWidth * 0.5f, markerY + markerH),
            markerColor
        );
    }

    // Draw modulation indicator (diamond) after slider
    ImGui::SameLine(0, INDICATOR_SPACING);

    const ImVec2 indicatorPos = ImGui::GetCursorScreenPos();
    const ImVec2 indicatorCenter = ImVec2(
        indicatorPos.x + INDICATOR_SIZE * 0.5f,
        indicatorPos.y + frameHeight * 0.5f
    );

    // Reserve space for indicator (unique ID per param)
    char indicatorBtnId[80];
    (void)snprintf(indicatorBtnId, sizeof(indicatorBtnId), "##mod_%s", paramId);
    ImGui::InvisibleButton(indicatorBtnId, ImVec2(INDICATOR_SIZE, frameHeight));
    const bool indicatorHovered = ImGui::IsItemHovered();
    const bool indicatorClicked = ImGui::IsItemClicked();

    // Determine indicator appearance
    ImU32 indicatorColor = Theme::TEXT_SECONDARY_U32;
    bool indicatorFilled = false;

    if (hasRoute) {
        indicatorColor = ModSourceGetColor((ModSource)route.source);
        indicatorFilled = true;

        // Pulse animation: alpha 0.7-1.0 over 800ms sine wave
        const float time = (float)ImGui::GetTime() * 1000.0f;
        const float phase = fmodf(time, PULSE_PERIOD_MS) / PULSE_PERIOD_MS;
        const float alpha = 0.7f + 0.3f * sinf(phase * 2.0f * 3.14159f);

        // Modulate alpha
        const int a = (int)(alpha * 255.0f);
        indicatorColor = (indicatorColor & 0x00FFFFFF) | (a << 24);
    }

    // Hover glow
    if (indicatorHovered) {
        ImU32 glowColor = hasRoute ? ModSourceGetColor((ModSource)route.source) : Theme::GLOW_CYAN;
        glowColor = (glowColor & 0x00FFFFFF) | (60 << 24);
        draw->AddCircleFilled(indicatorCenter, INDICATOR_SIZE * 0.8f, glowColor);
    }

    DrawDiamond(draw, indicatorCenter, INDICATOR_SIZE, indicatorColor, indicatorFilled);

    // Tooltip
    if (indicatorHovered) {
        if (hasRoute) {
            const char* sourceName = ModSourceGetName((ModSource)route.source);
            const int amountPercent = (int)(route.amount * 100.0f);
            ImGui::SetTooltip("%s -> %+d%%", sourceName, amountPercent);
        } else {
            ImGui::SetTooltip("Click to add modulation");
        }
    }

    // Draw source badge if active
    if (hasRoute) {
        ImGui::SameLine(0, 2.0f);
        const char* sourceName = ModSourceGetName((ModSource)route.source);
        const ImU32 badgeColor = ModSourceGetColor((ModSource)route.source);

        ImVec2 textPos = ImGui::GetCursorScreenPos();
        textPos.y += (frameHeight - ImGui::GetTextLineHeight()) * 0.5f;

        char badge[16];
        (void)snprintf(badge, sizeof(badge), "[%s]", sourceName);
        draw->AddText(textPos, badgeColor, badge);
        ImGui::Dummy(ImVec2(ImGui::CalcTextSize(badge).x, frameHeight));
    }

    // Handle indicator click - open popup (unique ID per param)
    char popupId[80];
    (void)snprintf(popupId, sizeof(popupId), "##modpopup_%s", paramId);
    if (indicatorClicked) {
        ImGui::OpenPopup(popupId);
    }

    // Popup for modulation configuration
    if (ImGui::BeginPopup(popupId)) {
            // Title
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), "Modulate: %s", label);
            ImGui::Separator();
            ImGui::Spacing();

            // Source selection grid (2 rows x 4 cols)
            ImGui::Text("Source:");
            ImGui::Spacing();

            static const ModSource audioSources[] = { MOD_SOURCE_BASS, MOD_SOURCE_MID, MOD_SOURCE_TREB, MOD_SOURCE_BEAT };
            static const ModSource lfoSources[] = { MOD_SOURCE_LFO1, MOD_SOURCE_LFO2, MOD_SOURCE_LFO3, MOD_SOURCE_LFO4 };

            const int selectedSource = hasRoute ? route.source : -1;
            const float buttonWidth = 50.0f;

            // Audio sources row
            DrawSourceButtonRow(draw, audioSources, selectedSource, &route, paramId, &hasRoute, sources, buttonWidth);

            // LFO sources row
            DrawSourceButtonRow(draw, lfoSources, selectedSource, &route, paramId, &hasRoute, sources, buttonWidth);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Amount slider (-100% to +100%)
            if (hasRoute) {
                float amountPercent = route.amount * 100.0f;
                if (ImGui::SliderFloat("Amount", &amountPercent, -100.0f, 100.0f, "%.0f%%")) {
                    route.amount = amountPercent / 100.0f;
                    ModEngineSetRoute(paramId, &route);
                }

                ImGui::Spacing();

                // Curve selection
                ImGui::Text("Curve:");
                ImGui::SameLine();

                const char* curveNames[] = { "Linear", "Exp", "Squared" };
                for (int i = 0; i < 3; i++) {
                    if (i > 0) {
                        ImGui::SameLine();
                    }
                    if (ImGui::RadioButton(curveNames[i], route.curve == i)) {
                        route.curve = i;
                        ModEngineSetRoute(paramId, &route);
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Remove button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Remove Modulation", ImVec2(-1, 0))) {
                    ModEngineRemoveRoute(paramId);
                    hasRoute = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor(2);
            }

        ImGui::EndPopup();
    }

    return changed;
}
