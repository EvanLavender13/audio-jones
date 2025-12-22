#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "analysis/beat.h"
#include "analysis/bands.h"
#include "config/band_config.h"
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

static const ImU32 BAND_ACTIVE_COLORS[3] = {
    Theme::BAND_CYAN_ACTIVE_U32,
    Theme::BAND_WHITE_ACTIVE_U32,
    Theme::BAND_MAGENTA_ACTIVE_U32
};

static void DrawBandSlider(const char* label, float* value, int bandIndex)
{
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, BAND_COLORS[bandIndex]);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, BAND_ACTIVE_COLORS[bandIndex]);
    ImGui::SliderFloat(label, value, 0.1f, 2.0f, "%.2f");
    ImGui::PopStyleColor(2);
}

// Animated band energy meter with gradient bars
// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
static void DrawBandMeter(const BandEnergies* bands, const BandConfig* config)
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

        // Calculate fill
        float fill = normalized[i] * sensitivities[i];
        if (fill < 0.0f) {
            fill = 0.0f;
        }
        if (fill > 2.0f) {
            fill = 2.0f;
        }
        const float fillRatio = fill / 2.0f;  // Scale to 0-1 (2.0 = full bar)
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

void ImGuiDrawAnalysisPanel(BeatDetector* beat, BandEnergies* bands, BandConfig* config)
{
    if (!ImGui::Begin("Analysis")) {
        ImGui::End();
        return;
    }

    // Performance section
    ImGui::TextColored(Theme::ACCENT_CYAN, "Performance");
    ImGui::Spacing();
    ImGui::Text("%d fps  %.2f ms", GetFPS(), GetFrameTime() * 1000.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Beat detection section - Cyan accent
    ImGui::TextColored(Theme::ACCENT_CYAN, "Beat Detection");
    ImGui::Spacing();
    DrawBeatGraph(beat);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Band energy section - Magenta accent
    ImGui::TextColored(Theme::ACCENT_MAGENTA, "Band Energy");
    ImGui::Spacing();
    DrawBandMeter(bands, config);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Sensitivity controls - Orange accent
    ImGui::TextColored(Theme::ACCENT_ORANGE, "Sensitivity");
    ImGui::Spacing();

    DrawBandSlider("Bass##sens", &config->bassSensitivity, 0);
    DrawBandSlider("Mid##sens", &config->midSensitivity, 1);
    DrawBandSlider("Treble##sens", &config->trebSensitivity, 2);

    ImGui::End();
}
