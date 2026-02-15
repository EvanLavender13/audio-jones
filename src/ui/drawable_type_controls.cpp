#include "ui/drawable_type_controls.h"
#include "automation/mod_sources.h"
#include "config/drawable_config.h"
#include "config/random_walk_config.h"
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
static bool sectionTrailShape = true;
static bool sectionTrailGate = true;
static bool sectionRandomWalk = true;

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
    ModulatableDrawableSliderSpeedDeg("Color Spin",
                                      &d->waveform.colorShiftSpeed, d->id,
                                      "colorShiftSpeed", sources);
    ModulatableDrawableSliderAngleDeg("Color Angle", &d->waveform.colorShift,
                                      d->id, "colorShift", sources);
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
    ModulatableDrawableSlider("Radius", &d->spectrum.innerRadius, d->id,
                              "innerRadius", "%.2f", sources);
    ModulatableDrawableSlider("Height", &d->spectrum.barHeight, d->id,
                              "barHeight", "%.2f", sources);
    ModulatableDrawableSlider("Width", &d->spectrum.barWidth, d->id, "barWidth",
                              "%.2f", sources);
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Dynamics", Theme::GLOW_MAGENTA, &sectionDynamics)) {
    ModulatableDrawableSlider("Smooth", &d->spectrum.smoothing, d->id,
                              "smoothing", "%.2f", sources);
    ImGui::SliderFloat("Min dB", &d->spectrum.minDb, 0.0f, 40.0f, "%.1f dB");
    ImGui::SliderFloat("Max dB", &d->spectrum.maxDb, 20.0f, 60.0f, "%.1f dB");
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Animation", Theme::GLOW_ORANGE, &sectionAnimation)) {
    DrawBaseAnimationControls(&d->base, d->id, sources);
    ModulatableDrawableSliderSpeedDeg("Color Spin",
                                      &d->spectrum.colorShiftSpeed, d->id,
                                      "colorShiftSpeed", sources);
    ModulatableDrawableSliderAngleDeg("Color Angle", &d->spectrum.colorShift,
                                      d->id, "colorShift", sources);
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
  const char *motionLabels[] = {"Lissajous", "Random Walk"};
  int motionIdx = static_cast<int>(d->parametricTrail.motionType);
  if (ImGui::Combo("Motion", &motionIdx, motionLabels, 2)) {
    d->parametricTrail.motionType = static_cast<TrailMotionType>(motionIdx);
    RandomWalkReset(&d->parametricTrail.walkState);
    d->parametricTrail.lissajous.phase = 0.0f;
  }

  ModulatableDrawableSlider("X", &d->base.x, d->id, "x", "%.2f", sources);
  ModulatableDrawableSlider("Y", &d->base.y, d->id, "y", "%.2f", sources);

  if (d->parametricTrail.motionType == TRAIL_MOTION_LISSAJOUS) {
    if (DrawSectionBegin("Path", Theme::GLOW_CYAN, &sectionTrailPath)) {
      ModulatableDrawableSlider(
          "Speed", &d->parametricTrail.lissajous.motionSpeed, d->id,
          "lissajous.motionSpeed", "%.2f", sources);
      ModulatableDrawableSlider("Amplitude",
                                &d->parametricTrail.lissajous.amplitude, d->id,
                                "lissajous.amplitude", "%.2f", sources);
      ImGui::SliderFloat("Freq X1", &d->parametricTrail.lissajous.freqX1, 0.0f,
                         10.0f, "%.2f Hz");
      ImGui::SliderFloat("Freq Y1", &d->parametricTrail.lissajous.freqY1, 0.0f,
                         10.0f, "%.2f Hz");
      ImGui::SliderFloat("Freq X2", &d->parametricTrail.lissajous.freqX2, 0.0f,
                         10.0f, "%.2f Hz");
      ImGui::SliderFloat("Freq Y2", &d->parametricTrail.lissajous.freqY2, 0.0f,
                         10.0f, "%.2f Hz");
      SliderAngleDeg("Offset X2", &d->parametricTrail.lissajous.offsetX2, 0.0f,
                     360.0f);
      SliderAngleDeg("Offset Y2", &d->parametricTrail.lissajous.offsetY2, 0.0f,
                     360.0f);
      DrawSectionEnd();
    }
  }

  if (d->parametricTrail.motionType == TRAIL_MOTION_RANDOM_WALK) {
    if (DrawSectionBegin("Random Walk", Theme::GLOW_CYAN, &sectionRandomWalk)) {
      ModulatableDrawableSlider("Step Size",
                                &d->parametricTrail.randomWalk.stepSize, d->id,
                                "randomWalk.stepSize", "%.3f", sources);
      ModulatableDrawableSlider(
          "Smoothness", &d->parametricTrail.randomWalk.smoothness, d->id,
          "randomWalk.smoothness", "%.2f", sources);
      ImGui::SliderFloat("Tick Rate", &d->parametricTrail.randomWalk.tickRate,
                         1.0f, 60.0f, "%.0f /s");
      const char *boundaryLabels[] = {"Clamp", "Wrap", "Drift"};
      int boundaryIdx =
          static_cast<int>(d->parametricTrail.randomWalk.boundaryMode);
      if (ImGui::Combo("Boundary", &boundaryIdx, boundaryLabels, 3)) {
        d->parametricTrail.randomWalk.boundaryMode =
            static_cast<WalkBoundaryMode>(boundaryIdx);
      }
      if (d->parametricTrail.randomWalk.boundaryMode == WALK_BOUNDARY_DRIFT) {
        ImGui::SliderFloat("Drift",
                           &d->parametricTrail.randomWalk.driftStrength, 0.0f,
                           2.0f, "%.2f");
      }
      ImGui::SliderInt("Seed", &d->parametricTrail.randomWalk.seed, 0, 9999);
      DrawSectionEnd();
    }
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Shape", Theme::GLOW_MAGENTA, &sectionTrailShape)) {
    const char *shapeLabels[] = {"Circle", "Triangle", "Square", "Pentagon",
                                 "Hexagon"};
    int shapeIdx = static_cast<int>(d->parametricTrail.shapeType);
    if (ImGui::Combo("Shape##trail", &shapeIdx, shapeLabels, 5)) {
      d->parametricTrail.shapeType = static_cast<TrailShapeType>(shapeIdx);
    }
    ModulatableDrawableSlider("Size", &d->parametricTrail.size, d->id, "size",
                              "%.0f px", sources);
    ImGui::Checkbox("Filled", &d->parametricTrail.filled);
    if (!d->parametricTrail.filled) {
      ModulatableDrawableSlider("Stroke", &d->parametricTrail.strokeThickness,
                                d->id, "strokeThickness", "%.1f px", sources);
    }
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
