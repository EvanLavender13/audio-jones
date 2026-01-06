#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/drawable_type_controls.h"
#include "ui/theme.h"
#include "config/drawable_config.h"
#include "render/drawable.h"
#include "automation/drawable_params.h"
#include <stdio.h>

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

// Stable ID counter for drawables - survives reorder/delete operations
static uint32_t sNextDrawableId = 1;

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
void ImGuiDrawDrawablesPanel(Drawable* drawables, int* count, int* selected, const ModSources* sources)
{
    if (!ImGui::Begin("Drawables")) {
        ImGui::End();
        return;
    }

    // Header
    ImGui::TextColored(Theme::ACCENT_CYAN, "Drawable List");
    ImGui::Spacing();

    const int waveformCount = DrawableCountByType(drawables, *count, DRAWABLE_WAVEFORM);

    // Add Waveform button
    ImGui::BeginDisabled(*count >= MAX_DRAWABLES);
    if (ImGui::Button("+ Waveform")) {
        Drawable d = {};
        d.id = sNextDrawableId++;
        d.type = DRAWABLE_WAVEFORM;
        d.path = PATH_CIRCULAR;
        d.base.color.solid = presetColors[waveformCount % 8];
        drawables[*count] = d;
        DrawableParamsRegister(&drawables[*count]);
        *selected = *count;
        (*count)++;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Add Spectrum button
    ImGui::BeginDisabled(*count >= MAX_DRAWABLES);
    if (ImGui::Button("+ Spectrum")) {
        Drawable d = {};
        d.id = sNextDrawableId++;
        d.type = DRAWABLE_SPECTRUM;
        d.path = PATH_CIRCULAR;
        d.base.color.solid = ThemeColor::NEON_MAGENTA;
        d.spectrum = SpectrumData{};
        drawables[*count] = d;
        DrawableParamsRegister(&drawables[*count]);
        *selected = *count;
        (*count)++;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(*count >= MAX_DRAWABLES);
    if (ImGui::Button("+ Shape")) {
        Drawable d = {};
        d.id = sNextDrawableId++;
        d.type = DRAWABLE_SHAPE;
        d.path = PATH_CIRCULAR;
        d.base.color.solid = ThemeColor::NEON_ORANGE;
        d.shape = ShapeData{};
        drawables[*count] = d;
        DrawableParamsRegister(&drawables[*count]);
        *selected = *count;
        (*count)++;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Delete button
    const bool canDelete = *selected >= 0 && *selected < *count;
    ImGui::BeginDisabled(!canDelete);
    if (ImGui::Button("Delete") && canDelete) {
        const uint32_t deletedId = drawables[*selected].id;
        DrawableParamsUnregister(deletedId);
        for (int i = *selected; i < *count - 1; i++) {
            drawables[i] = drawables[i + 1];
        }
        (*count)--;
        if (*selected >= *count) {
            *selected = *count - 1;
        }
        DrawableParamsSyncAll(drawables, *count);
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
        DrawableParamsSyncAll(drawables, *count);
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
        DrawableParamsSyncAll(drawables, *count);
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

        // Enabled toggle
        ImGui::Checkbox("Enabled", &sel->base.enabled);
        ImGui::Spacing();

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
            DrawWaveformControls(sel, sources);
        } else if (sel->type == DRAWABLE_SPECTRUM) {
            DrawSpectrumControls(sel, sources);
        } else if (sel->type == DRAWABLE_SHAPE) {
            DrawShapeControls(sel, sources);
        }
    }

    ImGui::End();
}

void ImGuiDrawDrawablesSyncIdCounter(const Drawable* drawables, int count)
{
    uint32_t maxId = 0;
    for (int i = 0; i < count; i++) {
        if (drawables[i].id > maxId) {
            maxId = drawables[i].id;
        }
    }
    sNextDrawableId = maxId + 1;
}
