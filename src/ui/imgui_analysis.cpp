#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "analysis/beat.h"
#include "analysis/bands.h"
#include "config/band_config.h"
#include "render/render_pipeline.h"
#include <math.h>

static const float GRAPH_HEIGHT = 80.0f;
static const float METER_BAR_HEIGHT = 22.0f;
static const float METER_SPACING = 4.0f;

// Widget colors from theme
static const ImU32 LABEL_COLOR = IM_COL32(153, 148, 173, 255);  // Theme::TEXT_SECONDARY as ImU32
static const ImU32 BAR_BG = IM_COL32(10, 8, 16, 255);           // Darker than BG_VOID
static const ImU32 BEAT_FLASH_COLOR = IM_COL32(0, 230, 242, 50); // Theme::CYAN with low alpha

// Convert beat intensity to color (muted purple -> bright cyan on peaks)
static ImU32 IntensityToColor(float intensity)
{
    float t = intensity;
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }

    // Interpolate from muted purple-blue to bright cyan
    // Low: Theme::BG_SURFACE-ish (0.12, 0.10, 0.16)
    // High: Theme::CYAN (0.00, 0.90, 0.95)
    const float r = 0.12f + t * (0.00f - 0.12f);
    const float g = 0.15f + t * (0.90f - 0.15f);
    const float b = 0.22f + t * (0.95f - 0.22f);

    return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
}

// Beat detection graph with gradient bars and peak glow
static void DrawBeatGraph(const BeatDetector* beat)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;

    // Background with subtle gradient using Theme colors
    DrawGradientBox(pos, ImVec2(width, GRAPH_HEIGHT),
                    Theme::WIDGET_BG_TOP, Theme::WIDGET_BG_BOTTOM);
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + GRAPH_HEIGHT),
                  Theme::WIDGET_BORDER, 2.0f);

    if (beat == NULL) {
        ImGui::Dummy(ImVec2(width, GRAPH_HEIGHT));
        return;
    }

    const int historySize = BEAT_GRAPH_SIZE;
    const float barWidth = width / (float)historySize;
    const float innerHeight = GRAPH_HEIGHT - 6.0f;

    // Find max for peak detection
    float maxIntensity = 0.0f;
    for (int i = 0; i < historySize; i++) {
        if (beat->graphHistory[i] > maxIntensity) {
            maxIntensity = beat->graphHistory[i];
        }
    }

    for (int i = 0; i < historySize; i++) {
        // Read from circular buffer (oldest to newest)
        const int idx = (beat->graphIndex + i) % historySize;
        float intensity = beat->graphHistory[idx];

        if (intensity < 0.0f) {
            intensity = 0.0f;
        }
        if (intensity > 1.0f) {
            intensity = 1.0f;
        }

        float barHeight = intensity * innerHeight;
        if (barHeight < 1.0f) {
            barHeight = 1.0f;
        }

        const float x = pos.x + i * barWidth + 1.0f;
        const float y = pos.y + GRAPH_HEIGHT - 3.0f - barHeight;
        float w = barWidth - 2.0f;
        if (w < 1.0f) {
            w = 1.0f;
        }

        const ImU32 barColor = IntensityToColor(intensity);

        // Draw bar with rounded top
        draw->AddRectFilled(
            ImVec2(x, y),
            ImVec2(x + w, pos.y + GRAPH_HEIGHT - 3.0f),
            barColor, 1.0f
        );

        // Glow effect on peaks (most recent strong beats) using Theme::GLOW_CYAN
        if (intensity > 0.7f && i > historySize - 10) {
            const float glowAlpha = (intensity - 0.7f) / 0.3f * 0.5f;
            const ImU32 glowColor = IM_COL32(0, 230, 242, (int)(glowAlpha * 255));
            draw->AddRectFilled(
                ImVec2(x - 1, y - 2),
                ImVec2(x + w + 1, y + 4),
                glowColor, 2.0f
            );
        }
    }

    // Beat indicator flash
    if (beat->beatDetected) {
        draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + GRAPH_HEIGHT), BEAT_FLASH_COLOR);
    }

    ImGui::Dummy(ImVec2(width, GRAPH_HEIGHT));
}

// Band meter colors - from Theme constants
static const ImU32 BAND_COLORS[3] = {
    Theme::BAND_CYAN_U32,
    Theme::BAND_WHITE_U32,
    Theme::BAND_MAGENTA_U32
};

static const ImU32 BAND_GLOW_COLORS[3] = {
    Theme::BAND_CYAN_GLOW_U32,
    Theme::BAND_WHITE_GLOW_U32,
    Theme::BAND_MAGENTA_GLOW_U32
};

// Zone colors for flame graph bars - cycling through theme accents
static const ImU32 ZONE_COLORS[ZONE_COUNT] = {
    Theme::ACCENT_CYAN_U32,     // Pre-Feedback
    Theme::BAND_WHITE_U32,      // Feedback
    Theme::ACCENT_MAGENTA_U32,  // Physarum
    Theme::ACCENT_ORANGE_U32,   // Curl
    Theme::ACCENT_CYAN_U32,     // Attractor
    Theme::BAND_WHITE_U32,      // Post-Feedback
    Theme::ACCENT_MAGENTA_U32   // Output
};

// Flame graph showing stacked horizontal bars for per-zone time breakdown
static void DrawProfilerFlame(const Profiler* profiler)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float barHeight = 24.0f;

    // Background
    DrawGradientBox(pos, ImVec2(width, barHeight),
                    Theme::WIDGET_BG_TOP, Theme::WIDGET_BG_BOTTOM);
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + barHeight),
                  Theme::WIDGET_BORDER, 2.0f);

    if (profiler == NULL || !profiler->enabled) {
        ImGui::Dummy(ImVec2(width, barHeight));
        return;
    }

    // Calculate total frame time from all zones
    float totalMs = 0.0f;
    for (int i = 0; i < ZONE_COUNT; i++) {
        totalMs += profiler->zones[i].lastMs;
    }

    // Draw stacked bars proportional to zone time
    const float innerPadding = 2.0f;
    const float barY = pos.y + innerPadding;
    const float innerHeight = barHeight - innerPadding * 2.0f;
    float xOffset = pos.x + innerPadding;
    const float innerWidth = width - innerPadding * 2.0f;

    if (totalMs > 0.001f) {
        for (int i = 0; i < ZONE_COUNT; i++) {
            const float zoneMs = profiler->zones[i].lastMs;
            if (zoneMs < 0.001f) {
                continue;
            }

            const float ratio = zoneMs / totalMs;
            const float barW = ratio * innerWidth;

            if (barW >= 1.0f) {
                // Draw zone bar
                const ImU32 col = ZONE_COLORS[i];
                const float r = ((col >> 0) & 0xFF) / 255.0f;
                const float g = ((col >> 8) & 0xFF) / 255.0f;
                const float b = ((col >> 16) & 0xFF) / 255.0f;

                // Darker left edge for depth
                const ImU32 colDark = IM_COL32(
                    (int)(r * 0.5f * 255),
                    (int)(g * 0.5f * 255),
                    (int)(b * 0.5f * 255),
                    255
                );

                draw->AddRectFilledMultiColor(
                    ImVec2(xOffset, barY),
                    ImVec2(xOffset + barW, barY + innerHeight),
                    colDark, col, col, colDark
                );

                // Draw zone name if bar is wide enough
                if (barW > 30.0f) {
                    const char* name = profiler->zones[i].name;
                    draw->AddText(ImVec2(xOffset + 3, barY + 2), Theme::TEXT_PRIMARY_U32, name);
                }

                xOffset += barW;
            }
        }
    }

    // Show total frame time as text header
    char label[32];
    snprintf(label, sizeof(label), "%.2f ms", totalMs);
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    draw->AddText(ImVec2(pos.x + width - textSize.x - 4, pos.y + 4),
                  Theme::TEXT_SECONDARY_U32, label);

    ImGui::Dummy(ImVec2(width, barHeight));
}

// Sparkline row height and graph dimensions
static const float SPARKLINE_ROW_HEIGHT = 28.0f;
static const float SPARKLINE_LABEL_WIDTH = 95.0f;
static const float SPARKLINE_VALUE_WIDTH = 36.0f;

// Frame budget bar showing percentage of 16.67ms target based on profiled zone times
static void DrawFrameBudgetBar(const Profiler* profiler)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float barHeight = METER_BAR_HEIGHT;

    // Background
    DrawGradientBox(pos, ImVec2(width, barHeight),
                    Theme::WIDGET_BG_TOP, Theme::WIDGET_BG_BOTTOM);
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + barHeight),
                  Theme::WIDGET_BORDER, 2.0f);

    // Sum profiled zone times (actual CPU work)
    float cpuMs = 0.0f;
    if (profiler != NULL && profiler->enabled) {
        for (int i = 0; i < ZONE_COUNT; i++) {
            cpuMs += profiler->zones[i].lastMs;
        }
    }

    const float budgetMs = 16.67f;
    const float budgetRatio = fminf(cpuMs / budgetMs, 1.0f);

    const float labelWidth = 50.0f;
    const float barPadding = 6.0f;
    const float barX = pos.x + labelWidth;
    const float barW = width - labelWidth - barPadding;
    const float barH = barHeight - 4.0f;
    const float barY = pos.y + 2.0f;

    // Label
    char label[32];
    snprintf(label, sizeof(label), "%.0f%%", budgetRatio * 100.0f);
    draw->AddText(ImVec2(pos.x + 6, pos.y + (barHeight - 12) / 2), LABEL_COLOR, label);

    // Bar background
    draw->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), BAR_BG, 2.0f);

    // Gradient fill based on budget usage
    const float fillW = budgetRatio * barW;
    if (fillW > 1.0f) {
        // Color shifts from cyan (low) -> orange (mid) -> magenta (high)
        ImU32 col;
        if (budgetRatio < 0.5f) {
            col = Theme::ACCENT_CYAN_U32;
        } else if (budgetRatio < 0.8f) {
            col = Theme::ACCENT_ORANGE_U32;
        } else {
            col = Theme::ACCENT_MAGENTA_U32;
        }

        const float r = ((col >> 0) & 0xFF) / 255.0f;
        const float g = ((col >> 8) & 0xFF) / 255.0f;
        const float b = ((col >> 16) & 0xFF) / 255.0f;
        const ImU32 colDark = IM_COL32((int)(r * 0.4f * 255), (int)(g * 0.4f * 255),
                                        (int)(b * 0.4f * 255), 255);

        draw->AddRectFilledMultiColor(
            ImVec2(barX, barY), ImVec2(barX + fillW, barY + barH),
            colDark, col, col, colDark
        );

        // Highlight line
        draw->AddLine(ImVec2(barX, barY), ImVec2(barX + fillW, barY), IM_COL32(255, 255, 255, 60));
    }

    // FPS and CPU ms text on right side
    char statsStr[32];
    snprintf(statsStr, sizeof(statsStr), "%d fps  %.2f ms", GetFPS(), cpuMs);
    const ImVec2 textSize = ImGui::CalcTextSize(statsStr);
    draw->AddText(ImVec2(pos.x + width - textSize.x - 4, pos.y + (barHeight - 12) / 2),
                  Theme::TEXT_SECONDARY_U32, statsStr);

    ImGui::Dummy(ImVec2(width, barHeight));
}

// Draw per-zone timing sparklines with history graphs
static void DrawProfilerSparklines(const Profiler* profiler)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }

    static bool sparklinesOpen = true;
    if (!DrawSectionBegin("Zone Timing", Theme::GLOW_ORANGE, &sparklinesOpen)) {
        DrawSectionEnd();
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float availWidth = ImGui::GetContentRegionAvail().x;

    for (int z = 0; z < ZONE_COUNT; z++) {
        const ProfileZone* zone = &profiler->zones[z];
        const ImVec2 rowPos = ImGui::GetCursorScreenPos();

        // Zone color
        const ImU32 zoneColor = ZONE_COLORS[z];

        // Zone name label (left side)
        draw->AddText(ImVec2(rowPos.x, rowPos.y + 6), Theme::TEXT_SECONDARY_U32, zone->name);

        // Sparkline graph (middle)
        const float graphX = rowPos.x + SPARKLINE_LABEL_WIDTH;
        const float graphW = availWidth - SPARKLINE_LABEL_WIDTH - SPARKLINE_VALUE_WIDTH - 8.0f;
        const float graphH = SPARKLINE_ROW_HEIGHT - 6.0f;
        const float graphY = rowPos.y + 3.0f;

        draw->AddRectFilled(
            ImVec2(graphX, graphY),
            ImVec2(graphX + graphW, graphY + graphH),
            BAR_BG, 2.0f
        );

        // Find max value for scaling
        float maxMs = 0.1f;
        for (int i = 0; i < PROFILER_HISTORY_SIZE; i++) {
            if (zone->history[i] > maxMs) {
                maxMs = zone->history[i];
            }
        }

        // Draw history bars (oldest to newest)
        const float barWidth = graphW / (float)PROFILER_HISTORY_SIZE;
        for (int i = 0; i < PROFILER_HISTORY_SIZE; i++) {
            const int idx = (zone->historyIndex + i) % PROFILER_HISTORY_SIZE;
            const float ms = zone->history[idx];
            const float ratio = ms / maxMs;
            const float barH = ratio * (graphH - 2.0f);

            if (barH >= 1.0f) {
                const float x = graphX + i * barWidth;
                const float y = graphY + graphH - 1.0f - barH;

                draw->AddRectFilled(
                    ImVec2(x, y),
                    ImVec2(x + barWidth - 1.0f, graphY + graphH - 1.0f),
                    zoneColor
                );
            }
        }

        // Current ms value (right side)
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), "%.2f", zone->lastMs);
        const float valueX = rowPos.x + availWidth - SPARKLINE_VALUE_WIDTH;
        draw->AddText(ImVec2(valueX, rowPos.y + 6), Theme::TEXT_PRIMARY_U32, valueStr);

        ImGui::Dummy(ImVec2(availWidth, SPARKLINE_ROW_HEIGHT));
    }

    DrawSectionEnd();
}

// Animated band energy meter with gradient bars
// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
static void DrawBandMeter(const BandEnergies* bands)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float totalHeight = 3 * METER_BAR_HEIGHT + 2 * METER_SPACING;

    // Background using Theme colors
    DrawGradientBox(pos, ImVec2(width, totalHeight),
                    Theme::WIDGET_BG_TOP, Theme::WIDGET_BG_BOTTOM);
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + totalHeight),
                  Theme::WIDGET_BORDER, 3.0f);

    if (bands == NULL) {
        ImGui::Dummy(ImVec2(width, totalHeight));
        return;
    }

    const char* labels[3] = { "BASS", "MID", "TREB" };

    // Normalize by running average (self-calibrating)
    const float MIN_AVG = 1e-6f;
    const float normalized[3] = {
        bands->bassSmooth / fmaxf(bands->bassAvg, MIN_AVG),
        bands->midSmooth / fmaxf(bands->midAvg, MIN_AVG),
        bands->trebSmooth / fmaxf(bands->trebAvg, MIN_AVG)
    };

    const float labelWidth = 40.0f;
    const float barPadding = 6.0f;

    for (int i = 0; i < 3; i++) {
        const float y = pos.y + i * (METER_BAR_HEIGHT + METER_SPACING);
        const float barX = pos.x + labelWidth;
        const float barW = width - labelWidth - barPadding;
        const float barH = METER_BAR_HEIGHT - 4.0f;
        const float barY = y + 2.0f;

        // Label
        draw->AddText(
            ImVec2(pos.x + 6, y + (METER_BAR_HEIGHT - 12) / 2),
            LABEL_COLOR,
            labels[i]
        );

        // Bar background with subtle inner shadow
        draw->AddRectFilled(
            ImVec2(barX, barY),
            ImVec2(barX + barW, barY + barH),
            BAR_BG, 2.0f
        );

        // Calculate fill: normalized/2 maps to 0-1 (2x average = full bar)
        const float fillRatio = fminf(normalized[i] / 2.0f, 1.0f);
        const float fillW = fillRatio * barW;

        if (fillW > 1.0f) {
            // Gradient fill - darker at left, brighter at right
            const ImU32 col = BAND_COLORS[i];
            const float r = ((col >> 0) & 0xFF) / 255.0f;
            const float g = ((col >> 8) & 0xFF) / 255.0f;
            const float b = ((col >> 16) & 0xFF) / 255.0f;

            const ImU32 colDark = IM_COL32(
                (int)(r * 0.4f * 255),
                (int)(g * 0.4f * 255),
                (int)(b * 0.4f * 255),
                255
            );

            draw->AddRectFilledMultiColor(
                ImVec2(barX, barY),
                ImVec2(barX + fillW, barY + barH),
                colDark, col, col, colDark
            );

            // Glow on high values
            if (fillRatio > 0.6f) {
                const float glowIntensity = (fillRatio - 0.6f) / 0.4f;
                ImU32 glow = BAND_GLOW_COLORS[i];
                glow = (glow & 0x00FFFFFF) | ((int)(glowIntensity * 120) << 24);
                draw->AddRectFilled(
                    ImVec2(barX, barY - 1),
                    ImVec2(barX + fillW, barY + barH + 1),
                    glow, 2.0f
                );
            }

            // Highlight line at top of bar
            draw->AddLine(
                ImVec2(barX, barY),
                ImVec2(barX + fillW, barY),
                IM_COL32(255, 255, 255, 60)
            );
        }

        // Tick marks at 50% and 100% using muted theme colors
        const float tick50 = barX + barW * 0.25f;
        const float tick100 = barX + barW * 0.5f;
        draw->AddLine(
            ImVec2(tick50, barY),
            ImVec2(tick50, barY + barH),
            IM_COL32(56, 46, 77, 200)  // Theme::BORDER muted
        );
        draw->AddLine(
            ImVec2(tick100, barY),
            ImVec2(tick100, barY + barH),
            IM_COL32(56, 46, 77, 255)  // Theme::BORDER
        );
    }

    ImGui::Dummy(ImVec2(width, totalHeight));
}

void ImGuiDrawAnalysisPanel(BeatDetector* beat, BandEnergies* bands, const Profiler* profiler)
{
    if (!ImGui::Begin("Analysis")) {
        ImGui::End();
        return;
    }

    // Beat detection section - Cyan accent
    ImGui::TextColored(Theme::ACCENT_CYAN, "Beat Detection");
    ImGui::Spacing();
    DrawBeatGraph(beat);

    ImGui::Spacing();

    // Band energy section - Magenta accent
    ImGui::TextColored(Theme::ACCENT_MAGENTA, "Band Energy");
    ImGui::Spacing();
    DrawBandMeter(bands);

    ImGui::Spacing();

    // Profiler section - Orange accent (includes performance info)
    ImGui::TextColored(Theme::ACCENT_ORANGE, "Profiler");
    ImGui::Spacing();
    DrawFrameBudgetBar(profiler);
    ImGui::Spacing();
    DrawProfilerFlame(profiler);

    ImGui::Spacing();
    DrawProfilerSparklines(profiler);

    ImGui::End();
}
