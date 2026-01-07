#include "ui/drawable_type_controls.h"
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_drawable_slider.h"
#include "config/drawable_config.h"
#include "automation/mod_sources.h"

static bool sectionGeometry = true;
static bool sectionDynamics = true;
static bool sectionAnimation = true;
static bool sectionColor = true;
static bool sectionTexture = true;

static void DrawBaseAnimationControls(DrawableBase* base, uint32_t drawableId, const ModSources* sources)
{
    ModulatableDrawableSliderAngleDeg("Spin", &base->rotationSpeed, drawableId, "rotationSpeed", sources);
    ModulatableDrawableSliderAngleDeg("Angle", &base->rotationAngle, drawableId, "rotationAngle", sources);
    ImGui::SliderFloat("Opacity", &base->opacity, 0.0f, 1.0f, "%.2f");
    SliderDrawInterval("Draw Freq", &base->drawInterval);
}

static void DrawBaseColorControls(DrawableBase* base)
{
    ImGuiDrawColorMode(&base->color);
}

void DrawWaveformControls(Drawable* d, const ModSources* sources)
{
    if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
        ModulatableDrawableSlider("X", &d->base.x, d->id, "x", "%.2f", sources);
        ModulatableDrawableSlider("Y", &d->base.y, d->id, "y", "%.2f", sources);
        ImGui::SliderFloat("Radius", &d->waveform.radius, 0.05f, 0.45f);
        ImGui::SliderFloat("Height", &d->waveform.amplitudeScale, 0.05f, 0.5f);
        ImGui::SliderInt("Thickness", &d->waveform.thickness, 1, 25, "%d px");
        ImGui::SliderFloat("Smooth", &d->waveform.smoothness, 0.0f, 100.0f, "%.1f px");
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Animation", Theme::GLOW_MAGENTA, &sectionAnimation)) {
        DrawBaseAnimationControls(&d->base, d->id, sources);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Color", Theme::GLOW_ORANGE, &sectionColor)) {
        DrawBaseColorControls(&d->base);
        DrawSectionEnd();
    }
}

void DrawSpectrumControls(Drawable* d, const ModSources* sources)
{
    if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
        ModulatableDrawableSlider("X", &d->base.x, d->id, "x", "%.2f", sources);
        ModulatableDrawableSlider("Y", &d->base.y, d->id, "y", "%.2f", sources);
        ImGui::SliderFloat("Radius", &d->spectrum.innerRadius, 0.05f, 0.4f);
        ImGui::SliderFloat("Height", &d->spectrum.barHeight, 0.1f, 0.5f);
        ImGui::SliderFloat("Width", &d->spectrum.barWidth, 0.3f, 1.0f);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Dynamics", Theme::GLOW_MAGENTA, &sectionDynamics)) {
        ImGui::SliderFloat("Smooth", &d->spectrum.smoothing, 0.0f, 0.95f);
        ImGui::SliderFloat("Min dB", &d->spectrum.minDb, 0.0f, 40.0f, "%.1f dB");
        ImGui::SliderFloat("Max dB", &d->spectrum.maxDb, 20.0f, 60.0f, "%.1f dB");
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Animation", Theme::GLOW_ORANGE, &sectionAnimation)) {
        DrawBaseAnimationControls(&d->base, d->id, sources);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Color", Theme::GLOW_CYAN, &sectionColor)) {
        DrawBaseColorControls(&d->base);
        DrawSectionEnd();
    }
}

void DrawShapeControls(Drawable* d, const ModSources* sources)
{
    if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
        ModulatableDrawableSlider("X", &d->base.x, d->id, "x", "%.2f", sources);
        ModulatableDrawableSlider("Y", &d->base.y, d->id, "y", "%.2f", sources);
        ImGui::SliderInt("Sides", &d->shape.sides, 3, 32);
        ImGui::SliderFloat("Size", &d->shape.size, 0.05f, 0.5f);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Texture", Theme::GLOW_MAGENTA, &sectionTexture)) {
        ImGui::Checkbox("Textured", &d->shape.textured);
        if (d->shape.textured) {
            ImGui::SliderFloat("Zoom", &d->shape.texZoom, 0.1f, 5.0f);
            ModulatableDrawableSliderAngleDeg("Tex Angle", &d->shape.texAngle, d->id, "texAngle", sources);
            ImGui::SliderFloat("Brightness", &d->shape.texBrightness, 0.0f, 1.0f);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Animation", Theme::GLOW_ORANGE, &sectionAnimation)) {
        DrawBaseAnimationControls(&d->base, d->id, sources);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Color", Theme::GLOW_CYAN, &sectionColor)) {
        DrawBaseColorControls(&d->base);
        DrawSectionEnd();
    }
}
