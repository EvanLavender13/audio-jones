#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "config/drawable_config.h"
#include "render/drawable.h"
#include "render/waveform.h"
#include "render/shape.h"
#include <stdio.h>
#include <math.h>

// Preset colors for new waveforms - from ThemeColor constants
static const Color presetColors[] = {
    ThemeColor::NEON_CYAN,
    ThemeColor::NEON_MAGENTA,
    ThemeColor::NEON_ORANGE,
    ThemeColor::NEON_WHITE,
    ThemeColor::NEON_CYAN_BRIGHT,
    ThemeColor::NEON_MAGENTA_BRIGHT,
    ThemeColor::NEON_ORANGE_BRIGHT,
    ThemeColor::NEON_CYAN_DIM
};

// Persistent section open states
static bool sectionGeometry = true;
static bool sectionDynamics = true;
static bool sectionAnimation = true;
static bool sectionColor = true;

// Count waveform-type drawables in array
static int CountWaveforms(const Drawable* drawables, int count)
{
    int waveformCount = 0;
    for (int i = 0; i < count; i++) {
        if (drawables[i].type == DRAWABLE_WAVEFORM) {
            waveformCount++;
        }
    }
    return waveformCount;
}

// Check if spectrum exists in array
static bool HasSpectrum(const Drawable* drawables, int count)
{
    for (int i = 0; i < count; i++) {
        if (drawables[i].type == DRAWABLE_SPECTRUM) {
            return true;
        }
    }
    return false;
}

static int CountShapes(const Drawable* drawables, int count)
{
    int shapeCount = 0;
    for (int i = 0; i < count; i++) {
        if (drawables[i].type == DRAWABLE_SHAPE) {
            shapeCount++;
        }
    }
    return shapeCount;
}

// Shared animation controls for all drawable types
static void DrawBaseAnimationControls(DrawableBase* base)
{
    SliderAngleDeg("Rotation", &base->rotationSpeed, -2.87f, 2.87f, "%.2f °/f");
    SliderAngleDeg("Offset", &base->rotationOffset, 0.0f, 360.0f);
    ImGui::SliderFloat("Feedback", &base->feedbackPhase, 0.0f, 1.0f, "%.2f");
}

// Shared color controls for all drawable types
static void DrawBaseColorControls(DrawableBase* base)
{
    ImGuiDrawColorMode(&base->color);
}

// Draw waveform-specific controls
static void DrawWaveformControls(Drawable* d)
{
    // Geometry section - Cyan accent
    if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
        ImGui::SliderFloat("X", &d->base.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Y", &d->base.y, 0.0f, 1.0f);
        ImGui::SliderFloat("Radius", &d->waveform.radius, 0.05f, 0.45f);
        ImGui::SliderFloat("Height", &d->waveform.amplitudeScale, 0.05f, 0.5f);
        ImGui::SliderInt("Thickness", &d->waveform.thickness, 1, 25, "%d px");
        ImGui::SliderFloat("Smooth", &d->waveform.smoothness, 0.0f, 100.0f, "%.1f px");
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Animation section - Magenta accent
    if (DrawSectionBegin("Animation", Theme::GLOW_MAGENTA, &sectionAnimation)) {
        DrawBaseAnimationControls(&d->base);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Color section - Orange accent
    if (DrawSectionBegin("Color", Theme::GLOW_ORANGE, &sectionColor)) {
        DrawBaseColorControls(&d->base);
        DrawSectionEnd();
    }
}

// Draw spectrum-specific controls
static void DrawSpectrumControls(Drawable* d)
{
    // Geometry section - Cyan accent
    if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
        ImGui::SliderFloat("X", &d->base.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Y", &d->base.y, 0.0f, 1.0f);
        ImGui::SliderFloat("Radius", &d->spectrum.innerRadius, 0.05f, 0.4f);
        ImGui::SliderFloat("Height", &d->spectrum.barHeight, 0.1f, 0.5f);
        ImGui::SliderFloat("Width", &d->spectrum.barWidth, 0.3f, 1.0f);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Dynamics section - Magenta accent
    if (DrawSectionBegin("Dynamics", Theme::GLOW_MAGENTA, &sectionDynamics)) {
        ImGui::SliderFloat("Smooth", &d->spectrum.smoothing, 0.0f, 0.95f);
        ImGui::SliderFloat("Min dB", &d->spectrum.minDb, 0.0f, 40.0f, "%.1f dB");
        ImGui::SliderFloat("Max dB", &d->spectrum.maxDb, 20.0f, 60.0f, "%.1f dB");
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Animation section - Orange accent
    if (DrawSectionBegin("Animation", Theme::GLOW_ORANGE, &sectionAnimation)) {
        DrawBaseAnimationControls(&d->base);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Color section - Cyan accent (cycle)
    if (DrawSectionBegin("Color", Theme::GLOW_CYAN, &sectionColor)) {
        DrawBaseColorControls(&d->base);
        DrawSectionEnd();
    }
}

static bool sectionTexture = true;

static void DrawShapeControls(Drawable* d)
{
    // Geometry section - Cyan accent
    if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
        ImGui::SliderFloat("X", &d->base.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Y", &d->base.y, 0.0f, 1.0f);
        ImGui::SliderInt("Sides", &d->shape.sides, 3, 32);
        ImGui::SliderFloat("Size", &d->shape.size, 0.05f, 0.5f);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Texture section - Magenta accent
    if (DrawSectionBegin("Texture", Theme::GLOW_MAGENTA, &sectionTexture)) {
        ImGui::Checkbox("Textured", &d->shape.textured);
        if (d->shape.textured) {
            ImGui::SliderFloat("Zoom", &d->shape.texZoom, 0.1f, 5.0f);
            SliderAngleDeg("Angle", &d->shape.texAngle, -180.0f, 180.0f);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Animation section - Orange accent
    if (DrawSectionBegin("Animation", Theme::GLOW_ORANGE, &sectionAnimation)) {
        DrawBaseAnimationControls(&d->base);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Color section - Cyan accent (cycle)
    if (DrawSectionBegin("Color", Theme::GLOW_CYAN, &sectionColor)) {
        DrawBaseColorControls(&d->base);
        DrawSectionEnd();
    }
}

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
void ImGuiDrawDrawablesPanel(Drawable* drawables, int* count, int* selected)
{
    if (!ImGui::Begin("Drawables")) {
        ImGui::End();
        return;
    }

    // Header
    ImGui::TextColored(Theme::ACCENT_CYAN, "Drawable List");
    ImGui::Spacing();

    const int waveformCount = CountWaveforms(drawables, *count);
    const bool hasSpectrum = HasSpectrum(drawables, *count);
    const int shapeCount = CountShapes(drawables, *count);

    // Add Waveform button
    ImGui::BeginDisabled(*count >= MAX_DRAWABLES || waveformCount >= MAX_WAVEFORMS);
    if (ImGui::Button("+ Waveform")) {
        Drawable d = {};
        d.type = DRAWABLE_WAVEFORM;
        d.path = PATH_CIRCULAR;
        d.base.color.solid = presetColors[waveformCount % 8];
        drawables[*count] = d;
        *selected = *count;
        (*count)++;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Add Spectrum button (only if none exists)
    ImGui::BeginDisabled(*count >= MAX_DRAWABLES || hasSpectrum);
    if (ImGui::Button("+ Spectrum")) {
        Drawable d = {};
        d.type = DRAWABLE_SPECTRUM;
        d.path = PATH_CIRCULAR;
        d.base.color.solid = ThemeColor::NEON_MAGENTA;
        d.spectrum = SpectrumData{};
        drawables[*count] = d;
        *selected = *count;
        (*count)++;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(*count >= MAX_DRAWABLES || shapeCount >= MAX_SHAPES);
    if (ImGui::Button("+ Shape")) {
        Drawable d = {};
        d.type = DRAWABLE_SHAPE;
        d.path = PATH_CIRCULAR;
        d.base.color.solid = ThemeColor::NEON_ORANGE;
        d.shape = ShapeData{};
        drawables[*count] = d;
        *selected = *count;
        (*count)++;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Delete button - waveforms require at least 1, spectrum and shapes can be deleted
    const bool canDelete = *selected >= 0 && *selected < *count &&
                     (drawables[*selected].type == DRAWABLE_SPECTRUM ||
                      drawables[*selected].type == DRAWABLE_SHAPE ||
                      (drawables[*selected].type == DRAWABLE_WAVEFORM && waveformCount > 1));
    ImGui::BeginDisabled(!canDelete);
    if (ImGui::Button("Delete") && canDelete) {
        for (int i = *selected; i < *count - 1; i++) {
            drawables[i] = drawables[i + 1];
        }
        (*count)--;
        if (*selected >= *count) {
            *selected = *count - 1;
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Move up button
    const bool canMoveUp = *selected > 0 && *selected < *count;
    ImGui::BeginDisabled(!canMoveUp);
    if (ImGui::Button("Up") && canMoveUp) {
        const Drawable temp = drawables[*selected];
        drawables[*selected] = drawables[*selected - 1];
        drawables[*selected - 1] = temp;
        (*selected)--;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Move down button
    const bool canMoveDown = *selected >= 0 && *selected < *count - 1;
    ImGui::BeginDisabled(!canMoveDown);
    if (ImGui::Button("Down") && canMoveDown) {
        const Drawable temp = drawables[*selected];
        drawables[*selected] = drawables[*selected + 1];
        drawables[*selected + 1] = temp;
        (*selected)++;
    }
    ImGui::EndDisabled();

    ImGui::Spacing();

    // Unified drawable list - shows ALL drawables with type indicators
    if (ImGui::BeginListBox("##DrawableList", ImVec2(-FLT_MIN, 100))) {
        int waveformIdx = 0;
        int shapeIdx = 0;
        for (int i = 0; i < *count; i++) {
            char label[48];
            if (drawables[i].type == DRAWABLE_WAVEFORM) {
                waveformIdx++;
                (void)snprintf(label, sizeof(label), "[W] Waveform %d", waveformIdx);
            } else if (drawables[i].type == DRAWABLE_SPECTRUM) {
                (void)snprintf(label, sizeof(label), "[S] Spectrum");
            } else if (drawables[i].type == DRAWABLE_SHAPE) {
                shapeIdx++;
                (void)snprintf(label, sizeof(label), "[P] Shape %d", shapeIdx);
            }

            // Dim disabled drawables in the list
            if (!drawables[i].base.enabled) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            }

            if (ImGui::Selectable(label, *selected == i)) {
                *selected = i;
            }

            if (!drawables[i].base.enabled) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndListBox();
    }

    // Selected drawable settings
    if (*selected >= 0 && *selected < *count) {
        Drawable* sel = &drawables[*selected];

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Type indicator header
        if (sel->type == DRAWABLE_WAVEFORM) {
            ImGui::TextColored(Theme::ACCENT_CYAN, "Waveform Settings");
        } else if (sel->type == DRAWABLE_SPECTRUM) {
            ImGui::TextColored(Theme::ACCENT_MAGENTA, "Spectrum Settings");
        } else if (sel->type == DRAWABLE_SHAPE) {
            ImGui::TextColored(Theme::ACCENT_ORANGE, "Shape Settings");
        }
        ImGui::Spacing();

        // Enabled toggle (for spectrum and shapes; waveforms always enabled via presence in list)
        if (sel->type == DRAWABLE_SPECTRUM || sel->type == DRAWABLE_SHAPE) {
            ImGui::Checkbox("Enabled", &sel->base.enabled);
            ImGui::Spacing();
        }

        // Path selector (applies to waveform and spectrum)
        if (sel->type == DRAWABLE_WAVEFORM || sel->type == DRAWABLE_SPECTRUM) {
            const char* pathItems[] = { "Linear", "Circular" };
            int pathIdx = (int)sel->path;
            if (ImGui::Combo("Path", &pathIdx, pathItems, 2)) {
                sel->path = (DrawablePath)pathIdx;
            }
            ImGui::Spacing();
        }

        // Type-specific controls
        if (sel->type == DRAWABLE_WAVEFORM) {
            DrawWaveformControls(sel);
        } else if (sel->type == DRAWABLE_SPECTRUM) {
            DrawSpectrumControls(sel);
        } else if (sel->type == DRAWABLE_SHAPE) {
            DrawShapeControls(sel);
        }
    }

    ImGui::End();
}
