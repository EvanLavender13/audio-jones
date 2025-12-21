#include "imgui.h"
#include "imgui_internal.h"
#include "ui/imgui_panels.h"
#include "analysis/beat.h"
#include "analysis/bands.h"
#include "config/band_config.h"
#include <math.h>

static const float GRAPH_HEIGHT = 80.0f;
static const float METER_BAR_HEIGHT = 22.0f;
static const float METER_SPACING = 4.0f;

// Widget theme colors
static const ImU32 WIDGET_BG_TOP = IM_COL32(25, 28, 32, 255);
static const ImU32 WIDGET_BG_BOTTOM = IM_COL32(18, 20, 24, 255);
static const ImU32 WIDGET_BG_SOLID = IM_COL32(22, 24, 28, 255);
static const ImU32 WIDGET_BORDER = IM_COL32(50, 55, 65, 255);
static const ImU32 WIDGET_BORDER_LIGHT = IM_COL32(45, 50, 60, 255);
static const ImU32 LABEL_COLOR = IM_COL32(140, 145, 160, 255);
static const ImU32 BAR_BG = IM_COL32(15, 16, 20, 255);
static const ImU32 BEAT_FLASH_COLOR = IM_COL32(100, 220, 255, 60);
static const ImU32 BEAT_GLOW_COLOR = IM_COL32(100, 200, 220, 0); // alpha set dynamically

// Convert beat intensity to color (dim gray -> bright cyan on peaks)
static ImU32 IntensityToColor(float intensity)
{
    float t = intensity;
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }

    // Base gray that brightens toward cyan on peaks
    float r = 0.3f + t * 0.2f;
    float g = 0.3f + t * 0.7f;
    float b = 0.35f + t * 0.65f;

    return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
}

// Beat detection graph with gradient bars and peak glow
static void DrawBeatGraph(const BeatDetector* beat)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;

    // Background with subtle gradient
    draw->AddRectFilledMultiColor(
        pos,
        ImVec2(pos.x + width, pos.y + GRAPH_HEIGHT),
        WIDGET_BG_TOP, WIDGET_BG_TOP, WIDGET_BG_BOTTOM, WIDGET_BG_BOTTOM
    );

    // Border
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + GRAPH_HEIGHT),
                  WIDGET_BORDER, 2.0f);

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
        int idx = (beat->graphIndex + i) % historySize;
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

        float x = pos.x + i * barWidth + 1.0f;
        float y = pos.y + GRAPH_HEIGHT - 3.0f - barHeight;
        float w = barWidth - 2.0f;
        if (w < 1.0f) {
            w = 1.0f;
        }

        ImU32 barColor = IntensityToColor(intensity);

        // Draw bar with rounded top
        draw->AddRectFilled(
            ImVec2(x, y),
            ImVec2(x + w, pos.y + GRAPH_HEIGHT - 3.0f),
            barColor, 1.0f
        );

        // Glow effect on peaks (most recent strong beats)
        if (intensity > 0.7f && i > historySize - 10) {
            float glowAlpha = (intensity - 0.7f) / 0.3f * 0.4f;
            ImU32 glowColor = IM_COL32(100, 200, 220, (int)(glowAlpha * 255));
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

// Band meter colors
static const ImU32 BAND_COLORS[3] = {
    IM_COL32(80, 180, 220, 255),   // Bass - cyan/blue
    IM_COL32(220, 220, 240, 255),  // Mid - white/silver
    IM_COL32(200, 100, 220, 255)   // Treble - magenta/purple
};

static const ImU32 BAND_GLOW_COLORS[3] = {
    IM_COL32(80, 180, 220, 100),
    IM_COL32(220, 220, 240, 80),
    IM_COL32(200, 100, 220, 100)
};

static const ImU32 BAND_ACTIVE_COLORS[3] = {
    IM_COL32(100, 200, 240, 255),
    IM_COL32(240, 240, 255, 255),
    IM_COL32(220, 120, 240, 255)
};

static void DrawBandSlider(const char* label, float* value, int bandIndex)
{
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, BAND_COLORS[bandIndex]);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, BAND_ACTIVE_COLORS[bandIndex]);
    ImGui::SliderFloat(label, value, 0.1f, 2.0f, "%.2f");
    ImGui::PopStyleColor(2);
}

// Animated band energy meter with gradient bars
static void DrawBandMeter(const BandEnergies* bands, const BandConfig* config)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float totalHeight = 3 * METER_BAR_HEIGHT + 2 * METER_SPACING;

    // Background
    draw->AddRectFilled(
        pos,
        ImVec2(pos.x + width, pos.y + totalHeight),
        WIDGET_BG_SOLID, 3.0f
    );
    draw->AddRect(
        pos,
        ImVec2(pos.x + width, pos.y + totalHeight),
        WIDGET_BORDER_LIGHT, 3.0f
    );

    if (bands == NULL || config == NULL) {
        ImGui::Dummy(ImVec2(width, totalHeight));
        return;
    }

    const char* labels[3] = { "BASS", "MID", "TREB" };
    const float sensitivities[3] = {
        config->bassSensitivity,
        config->midSensitivity,
        config->trebSensitivity
    };

    // Normalize by running average (self-calibrating)
    const float MIN_AVG = 1e-6f;
    float normalized[3] = {
        bands->bassSmooth / fmaxf(bands->bassAvg, MIN_AVG),
        bands->midSmooth / fmaxf(bands->midAvg, MIN_AVG),
        bands->trebSmooth / fmaxf(bands->trebAvg, MIN_AVG)
    };

    const float labelWidth = 40.0f;
    const float barPadding = 6.0f;

    for (int i = 0; i < 3; i++) {
        float y = pos.y + i * (METER_BAR_HEIGHT + METER_SPACING);
        float barX = pos.x + labelWidth;
        float barW = width - labelWidth - barPadding;
        float barH = METER_BAR_HEIGHT - 4.0f;
        float barY = y + 2.0f;

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

        // Calculate fill
        float fill = normalized[i] * sensitivities[i];
        if (fill < 0.0f) {
            fill = 0.0f;
        }
        if (fill > 2.0f) {
            fill = 2.0f;
        }
        float fillRatio = fill / 2.0f;  // Scale to 0-1 (2.0 = full bar)
        float fillW = fillRatio * barW;

        if (fillW > 1.0f) {
            // Gradient fill - darker at left, brighter at right
            ImU32 col = BAND_COLORS[i];
            float r = ((col >> 0) & 0xFF) / 255.0f;
            float g = ((col >> 8) & 0xFF) / 255.0f;
            float b = ((col >> 16) & 0xFF) / 255.0f;

            ImU32 colDark = IM_COL32(
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
                float glowIntensity = (fillRatio - 0.6f) / 0.4f;
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

        // Tick marks at 50% and 100%
        float tick50 = barX + barW * 0.25f;
        float tick100 = barX + barW * 0.5f;
        draw->AddLine(
            ImVec2(tick50, barY),
            ImVec2(tick50, barY + barH),
            IM_COL32(50, 55, 65, 255)
        );
        draw->AddLine(
            ImVec2(tick100, barY),
            ImVec2(tick100, barY + barH),
            IM_COL32(70, 75, 85, 255)
        );
    }

    ImGui::Dummy(ImVec2(width, totalHeight));
}

void ImGuiDrawAnalysisPanel(BeatDetector* beat, BandEnergies* bands, BandConfig* config)
{
    if (!ImGui::Begin("Analysis")) {
        ImGui::End();
        return;
    }

    // Beat detection section
    ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.75f, 1.0f), "Beat Detection");
    ImGui::Spacing();
    DrawBeatGraph(beat);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Band energy section
    ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.75f, 1.0f), "Band Energy");
    ImGui::Spacing();
    DrawBandMeter(bands, config);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Sensitivity controls
    ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.75f, 1.0f), "Sensitivity");
    ImGui::Spacing();

    DrawBandSlider("Bass##sens", &config->bassSensitivity, 0);
    DrawBandSlider("Mid##sens", &config->midSensitivity, 1);
    DrawBandSlider("Treble##sens", &config->trebSensitivity, 2);

    ImGui::End();
}
