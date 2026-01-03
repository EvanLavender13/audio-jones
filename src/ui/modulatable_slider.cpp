#include "modulatable_slider.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/theme.h"
#include "automation/modulation_engine.h"
#include "automation/param_registry.h"
#include "automation/easing.h"
#include <string>
#include <math.h>

static const float INDICATOR_SIZE = 10.0f;
static const float INDICATOR_SPACING = 4.0f;
static const float PULSE_PERIOD_MS = 800.0f;
static const int CURVE_SAMPLE_COUNT = 24;

// Y-range expansion for overshoot curves (spring/elastic)
// Spring peaks at ~1.08, elastic at ~1.05; 1.3 provides 20% visual headroom
static const float OVERSHOOT_Y_MIN = -0.1f;
static const float OVERSHOOT_Y_MAX = 1.3f;

static void DrawCurvePreview(ImVec2 size, ModCurve curve, ImU32 curveColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::Dummy(size);

    const float padX = 4.0f;
    const float padY = 3.0f;
    const ImVec2 graphMin = ImVec2(pos.x + padX, pos.y + padY);
    const ImVec2 graphMax = ImVec2(pos.x + size.x - padX, pos.y + size.y - padY);
    const float graphW = graphMax.x - graphMin.x;
    const float graphH = graphMax.y - graphMin.y;

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                        SetColorAlpha(Theme::WIDGET_BG_BOTTOM, 200), 3.0f);
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                  Theme::WIDGET_BORDER, 3.0f);

    // Determine Y range - most curves are 0-1, but spring/elastic overshoot
    float yMin = 0.0f;
    float yMax = 1.0f;
    if (curve == MOD_CURVE_SPRING || curve == MOD_CURVE_ELASTIC) {
        yMin = OVERSHOOT_Y_MIN;
        yMax = OVERSHOOT_Y_MAX;
    }
    const float yRange = yMax - yMin;

    // Baseline at y=0 for overshoot curves
    if (yMin < 0.0f) {
        const float baselineY = graphMax.y - ((-yMin) / yRange) * graphH;
        draw->AddLine(ImVec2(graphMin.x, baselineY), ImVec2(graphMax.x, baselineY),
                      Theme::GUIDE_LINE, 1.0f);
    }

    // Target line at y=1
    const float targetY = graphMax.y - ((1.0f - yMin) / yRange) * graphH;
    draw->AddLine(ImVec2(graphMin.x, targetY), ImVec2(graphMax.x, targetY),
                  SetColorAlpha(Theme::GUIDE_LINE, 50), 1.0f);

    // Sample the curve and build polyline
    ImVec2 points[CURVE_SAMPLE_COUNT];
    for (int i = 0; i < CURVE_SAMPLE_COUNT; i++) {
        const float t = (float)i / (float)(CURVE_SAMPLE_COUNT - 1);
        const float value = EasingEvaluate(t, curve);
        const float normY = (value - yMin) / yRange;
        points[i] = ImVec2(
            graphMin.x + t * graphW,
            graphMax.y - normY * graphH
        );
    }

    // Draw curve with glow
    const ImU32 glowColor = SetColorAlpha(curveColor, 40);
    draw->AddPolyline(points, CURVE_SAMPLE_COUNT, glowColor, ImDrawFlags_None, 3.0f);
    draw->AddPolyline(points, CURVE_SAMPLE_COUNT, curveColor, ImDrawFlags_None, 1.5f);
}

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
static void DrawSourceButtonRow(const ModSource sources[4], int selectedSource,
                                 ModRoute* route, const char* paramId, bool* hasRoute,
                                 const ModSources* modSources, float buttonWidth)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
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
            const float maxBarWidth = (btnMax.x - btnMin.x - 4.0f) * 0.5f;
            const float centerX = (btnMin.x + btnMax.x) * 0.5f;
            const float barWidth = maxBarWidth * fabsf(val);
            const float barX = (val >= 0.0f) ? centerX : centerX - barWidth;
            draw->AddRectFilled(
                ImVec2(barX, btnMax.y - barHeight - 2.0f),
                ImVec2(barX + barWidth, btnMax.y - 2.0f),
                srcColor
            );
        }

        if (isSelected) {
            ImGui::PopStyleColor(3);
        }
    }
}

// NOLINTNEXTLINE(readability-function-size) - UI widget with detailed visual rendering
static void DrawModulationTrack(ImDrawList* draw, float baseValue, float limitValue,
                                 float modulatedValue, float min, float max, ImVec2 frameMin,
                                 float frameWidth, float frameHeight, const ImGuiStyle& style,
                                 ImU32 sourceColor)
{
    const float baseRatio = ImClamp(ValueToRatio(baseValue, min, max), 0.0f, 1.0f);
    const float limitRatio = ImClamp(ValueToRatio(limitValue, min, max), 0.0f, 1.0f);
    const float modRatio = ImClamp(ValueToRatio(modulatedValue, min, max), 0.0f, 1.0f);

    const float grabPadding = 2.0f;
    const float sliderUsable = frameWidth - grabPadding * 2.0f;
    const float grabSz = ImMin(style.GrabMinSize, sliderUsable);
    const float sliderRange = sliderUsable - grabSz;

    const float baseXCenter = frameMin.x + grabPadding + baseRatio * sliderRange + grabSz * 0.5f;
    const float limitXCenter = frameMin.x + grabPadding + limitRatio * sliderRange + grabSz * 0.5f;
    const float modXCenter = frameMin.x + grabPadding + modRatio * sliderRange + grabSz * 0.5f;

    const float highlightMinX = ImMin(baseXCenter, modXCenter);
    const float highlightMaxX = ImMax(baseXCenter, modXCenter);
    const float highlightY = frameMin.y + frameHeight * 0.35f;
    const float highlightH = frameHeight * 0.3f;

    draw->AddRectFilled(
        ImVec2(highlightMinX, highlightY),
        ImVec2(highlightMaxX, highlightY + highlightH),
        SetColorAlpha(sourceColor, 50)
    );

    const float markerWidth = 2.0f;
    const float markerY = frameMin.y + 3.0f;
    const float markerH = frameHeight - 6.0f;

    auto drawTick = [&](float xCenter, int alpha) {
        draw->AddRectFilled(
            ImVec2(xCenter - markerWidth * 0.5f, markerY),
            ImVec2(xCenter + markerWidth * 0.5f, markerY + markerH),
            SetColorAlpha(sourceColor, alpha)
        );
    };
    drawTick(baseXCenter, 180);
    drawTick(limitXCenter, 90);
}

// Draws modulation indicator diamond with pulse animation, hover glow, and tooltip
static bool DrawModulationIndicator(ImDrawList* draw, const char* paramId, bool hasRoute,
                                     ModSource source, float frameHeight, ModRoute* route)
{
    ImGui::SameLine(0, INDICATOR_SPACING);

    const ImVec2 indicatorPos = ImGui::GetCursorScreenPos();
    const ImVec2 indicatorCenter = ImVec2(
        indicatorPos.x + INDICATOR_SIZE * 0.5f,
        indicatorPos.y + frameHeight * 0.5f
    );

    char indicatorBtnId[80];
    (void)snprintf(indicatorBtnId, sizeof(indicatorBtnId), "##mod_%s", paramId);
    ImGui::InvisibleButton(indicatorBtnId, ImVec2(INDICATOR_SIZE, frameHeight));
    const bool indicatorHovered = ImGui::IsItemHovered();
    const bool indicatorClicked = ImGui::IsItemClicked();

    ImU32 indicatorColor = Theme::TEXT_SECONDARY_U32;
    bool indicatorFilled = false;

    if (hasRoute) {
        indicatorColor = ModSourceGetColor(source);
        indicatorFilled = true;

        const float time = (float)ImGui::GetTime() * 1000.0f;
        const float phase = fmodf(time, PULSE_PERIOD_MS) / PULSE_PERIOD_MS;
        const float alpha = 0.7f + 0.3f * sinf(phase * 2.0f * 3.14159f);
        indicatorColor = SetColorAlpha(indicatorColor, (int)(alpha * 255.0f));
    }

    if (indicatorHovered) {
        const ImU32 glowColor = hasRoute ? ModSourceGetColor(source) : Theme::GLOW_CYAN;
        draw->AddCircleFilled(indicatorCenter, INDICATOR_SIZE * 0.8f, SetColorAlpha(glowColor, 60));
    }

    DrawDiamond(draw, indicatorCenter, INDICATOR_SIZE, indicatorColor, indicatorFilled);

    if (indicatorHovered) {
        if (hasRoute) {
            const char* sourceName = ModSourceGetName(source);
            const int amountPercent = (int)(route->amount * 100.0f);
            ImGui::SetTooltip("%s -> %+d%%", sourceName, amountPercent);
        } else {
            ImGui::SetTooltip("Click to add modulation");
        }
    }

    return indicatorClicked;
}

// Draws source badge after slider
static void DrawSourceBadge(ImDrawList* draw, ModSource source, float frameHeight)
{
    ImGui::SameLine(0, 2.0f);
    const char* sourceName = ModSourceGetName(source);
    const ImU32 badgeColor = ModSourceGetColor(source);

    ImVec2 textPos = ImGui::GetCursorScreenPos();
    textPos.y += (frameHeight - ImGui::GetTextLineHeight()) * 0.5f;

    char badge[16];
    (void)snprintf(badge, sizeof(badge), "[%s]", sourceName);
    draw->AddText(textPos, badgeColor, badge);
    ImGui::Dummy(ImVec2(ImGui::CalcTextSize(badge).x, frameHeight));
}

// Draws modulation configuration popup content
static void DrawModulationPopup(const char* label, const char* paramId, const char* popupId,
                                 ModRoute* route, bool* hasRoute, const ModSources* sources)
{
    if (!ImGui::BeginPopup(popupId)) {
        return;
    }

    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), "Modulate: %s", label);
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Source:");
    ImGui::Spacing();

    static const ModSource audioSources[] = { MOD_SOURCE_BASS, MOD_SOURCE_MID, MOD_SOURCE_TREB, MOD_SOURCE_BEAT };
    static const ModSource lfoSources[] = { MOD_SOURCE_LFO1, MOD_SOURCE_LFO2, MOD_SOURCE_LFO3, MOD_SOURCE_LFO4 };

    const int selectedSource = *hasRoute ? route->source : -1;
    const float buttonWidth = 50.0f;

    DrawSourceButtonRow(audioSources, selectedSource, route, paramId, hasRoute, sources, buttonWidth);
    DrawSourceButtonRow(lfoSources, selectedSource, route, paramId, hasRoute, sources, buttonWidth);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (*hasRoute) {
        float amountPercent = route->amount * 100.0f;
        if (ImGui::SliderFloat("Amount", &amountPercent, -100.0f, 100.0f, "%.0f%%")) {
            route->amount = amountPercent / 100.0f;
            ModEngineSetRoute(paramId, route);
        }

        ImGui::Spacing();

        static const char* curveNames[] = {
            "Linear", "Ease In", "Ease Out", "Ease In-Out",
            "Spring", "Elastic", "Bounce"
        };

        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::Combo("Curve", &route->curve, curveNames, MOD_CURVE_COUNT)) {
            ModEngineSetRoute(paramId, route);
        }
        ImGui::SameLine();
        DrawCurvePreview(ImVec2(60.0f, 28.0f), (ModCurve)route->curve,
                         ModSourceGetColor((ModSource)route->source));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Remove Modulation", ImVec2(-1, 0))) {
            ModEngineRemoveRoute(paramId);
            *hasRoute = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::EndPopup();
}

bool ModulatableSlider(const char* label, float* value, const char* paramId,
                       const char* format, const ModSources* sources,
                       float displayScale)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    ParamDef def;
    if (!ParamRegistryGetDynamic(paramId, 0.0f, 1.0f, &def)) {
        TraceLog(LOG_WARNING, "ModulatableSlider: paramId '%s' not found in registry", paramId);
        return false;
    }
    const float min = def.min;
    const float max = def.max;

    // Scale bounds and value for display
    const float displayMin = min * displayScale;
    const float displayMax = max * displayScale;
    float displayValue = *value * displayScale;

    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // Check if we have a route
    ModRoute route;
    bool hasRoute = ModEngineGetRoute(paramId, &route);

    // Draw the slider with display-scaled values
    const bool changed = ImGui::SliderFloat(label, &displayValue, displayMin, displayMax, format);

    // Convert back to internal units
    if (changed) {
        *value = displayValue / displayScale;
    }

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

    if (hasRoute) {
        const float baseValue = ModEngineGetBase(paramId) * displayScale;
        const float range = displayMax - displayMin;
        const float limitValue = ImClamp(baseValue + route.amount * range, displayMin, displayMax);
        DrawModulationTrack(draw, baseValue, limitValue, displayValue, displayMin, displayMax,
                            frameMin, frameWidth, frameHeight, style,
                            ModSourceGetColor((ModSource)route.source));
    }

    const bool indicatorClicked = DrawModulationIndicator(draw, paramId, hasRoute,
                                                           (ModSource)route.source, frameHeight, &route);

    if (hasRoute) {
        DrawSourceBadge(draw, (ModSource)route.source, frameHeight);
    }

    char popupId[80];
    (void)snprintf(popupId, sizeof(popupId), "##modpopup_%s", paramId);
    if (indicatorClicked) {
        ImGui::OpenPopup(popupId);
    }

    DrawModulationPopup(label, paramId, popupId, &route, &hasRoute, sources);

    return changed;
}
