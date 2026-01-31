#include "ui/drawable_type_controls.h"
#include "automation/mod_sources.h"
#include "config/drawable_config.h"
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_drawable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionGeometry = true;
static bool sectionDynamics = true;
static bool sectionAnimation = true;
static bool sectionColor = true;
static bool sectionTexture = true;
static bool sectionTrailPath = true;
static bool sectionTrailStroke = true;
static bool sectionTrailGate = true;

static void DrawBaseAnimationControls(DrawableBase *base, uint32_t drawableId,
                                      const ModSources *sources) {
  ModulatableDrawableSliderSpeedDeg("Spin", &base->rotationSpeed, drawableId,
                                    "rotationSpeed", sources);
  ModulatableDrawableSliderAngleDeg("Angle", &base->rotationAngle, drawableId,
                                    "rotationAngle", sources);
  ImGui::SliderFloat("Opacity", &base->opacity, 0.0f, 1.0f, "%.2f");
  SliderDrawInterval("Draw Freq", &base->drawInterval);
}

static void DrawBaseColorControls(DrawableBase *base) {
  ImGuiDrawColorMode(&base->color);
}

void DrawWaveformControls(Drawable *d, const ModSources *sources) {
  if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
    ModulatableDrawableSlider("X", &d->base.x, d->id, "x", "%.2f", sources);
    ModulatableDrawableSlider("Y", &d->base.y, d->id, "y", "%.2f", sources);
    ModulatableDrawableSlider("Radius", &d->waveform.radius, d->id, "radius",
                              "%.2f", sources);
    ModulatableDrawableSlider("Height", &d->waveform.amplitudeScale, d->id,
                              "amplitudeScale", "%.2f", sources);
    ModulatableDrawableSlider("Thickness", &d->waveform.thickness, d->id,
                              "thickness", "%.0f px", sources);
    ModulatableDrawableSlider("Smooth", &d->waveform.smoothness, d->id,
                              "smoothness", "%.1f px", sources);
    ModulatableDrawableSliderLog("Motion", &d->waveform.waveformMotionScale,
                                 d->id, "waveformMotionScale", "%.3f", sources);
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

void DrawSpectrumControls(Drawable *d, const ModSources *sources) {
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

void DrawShapeControls(Drawable *d, const ModSources *sources) {
  if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
    ModulatableDrawableSlider("X", &d->base.x, d->id, "x", "%.2f", sources);
    ModulatableDrawableSlider("Y", &d->base.y, d->id, "y", "%.2f", sources);
    ImGui::SliderInt("Sides", &d->shape.sides, 3, 32);

    const float prevWidth = d->shape.width;
    const float prevHeight = d->shape.height;

    ImGui::Checkbox("Lock Aspect", &d->shape.aspectLocked);
    const bool widthChanged = ModulatableDrawableSlider(
        "Width", &d->shape.width, d->id, "width", "%.2f", sources);
    const bool heightChanged = ModulatableDrawableSlider(
        "Height", &d->shape.height, d->id, "height", "%.2f", sources);

    // Only apply aspect lock for user slider interaction, not modulation
    // changes
    if (d->shape.aspectLocked) {
      if (widthChanged && prevWidth > 0.0f) {
        d->shape.height *= (d->shape.width / prevWidth);
      } else if (heightChanged && prevHeight > 0.0f) {
        d->shape.width *= (d->shape.height / prevHeight);
      }
    }
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Texture", Theme::GLOW_MAGENTA, &sectionTexture)) {
    ImGui::Checkbox("Textured", &d->shape.textured);
    if (d->shape.textured) {
      ImGui::SliderFloat("Zoom", &d->shape.texZoom, 0.1f, 5.0f);
      ModulatableDrawableSliderAngleDeg("Tex Angle", &d->shape.texAngle, d->id,
                                        "texAngle", sources);
      ImGui::SliderFloat("Brightness", &d->shape.texBrightness, 0.0f, 1.0f);
      ModulatableDrawableSliderLog("Motion", &d->shape.texMotionScale, d->id,
                                   "texMotionScale", "%.3f", sources);
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

void DrawParametricTrailControls(Drawable *d, const ModSources *sources) {
  if (DrawSectionBegin("Path", Theme::GLOW_CYAN, &sectionTrailPath)) {
    ModulatableDrawableSlider("X", &d->base.x, d->id, "x", "%.2f", sources);
    ModulatableDrawableSlider("Y", &d->base.y, d->id, "y", "%.2f", sources);
    ModulatableDrawableSlider("Speed", &d->parametricTrail.speed, d->id,
                              "speed", "%.2f", sources);
    ModulatableDrawableSlider("Size", &d->parametricTrail.amplitude, d->id,
                              "amplitude", "%.2f", sources);
    ModulatableDrawableSlider("Freq X1", &d->parametricTrail.freqX1, d->id,
                              "freqX1", "%.2f", sources);
    ModulatableDrawableSlider("Freq Y1", &d->parametricTrail.freqY1, d->id,
                              "freqY1", "%.2f", sources);
    ModulatableDrawableSlider("Freq X2", &d->parametricTrail.freqX2, d->id,
                              "freqX2", "%.2f", sources);
    ModulatableDrawableSlider("Freq Y2", &d->parametricTrail.freqY2, d->id,
                              "freqY2", "%.2f", sources);
    SliderAngleDeg("Offset X", &d->parametricTrail.offsetX, 0.0f, 360.0f);
    SliderAngleDeg("Offset Y", &d->parametricTrail.offsetY, 0.0f, 360.0f);
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Stroke", Theme::GLOW_MAGENTA, &sectionTrailStroke)) {
    ModulatableDrawableSlider("Thickness", &d->parametricTrail.thickness, d->id,
                              "thickness", "%.0f px", sources);
    ImGui::Checkbox("Rounded##trailrounded", &d->parametricTrail.roundedCaps);
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Gate", Theme::GLOW_ORANGE, &sectionTrailGate)) {
    ModulatableDrawableSlider("Frequency", &d->parametricTrail.gateFreq, d->id,
                              "gateFreq", "%.1f Hz", sources);
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
