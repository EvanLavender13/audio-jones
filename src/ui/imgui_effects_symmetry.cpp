#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionKaleidoscope = false;
static bool sectionKifs = false;
static bool sectionPoincareDisk = false;
static bool sectionMandelbox = false;
static bool sectionTriangleFold = false;
static bool sectionMoireInterference = false;
static bool sectionRadialIfs = false;

static void DrawSymmetryKaleidoscope(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
  if (DrawSectionBegin("Kaleidoscope", categoryGlow, &sectionKaleidoscope)) {
    const bool wasEnabled = e->kaleidoscope.enabled;
    ImGui::Checkbox("Enabled##kaleido", &e->kaleidoscope.enabled);
    if (!wasEnabled && e->kaleidoscope.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_KALEIDOSCOPE);
    }
    if (e->kaleidoscope.enabled) {
      KaleidoscopeConfig *k = &e->kaleidoscope;

      ImGui::SliderInt("Segments", &k->segments, 1, 12);
      ModulatableSliderSpeedDeg("Spin", &k->rotationSpeed,
                                "kaleidoscope.rotationSpeed", modSources);
      ModulatableSliderAngleDeg("Twist##kaleido", &k->twistAngle,
                                "kaleidoscope.twistAngle", modSources,
                                "%.1f °");
      ModulatableSlider("Smoothing##kaleido", &k->smoothing,
                        "kaleidoscope.smoothing", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawSymmetryKifs(EffectConfig *e, const ModSources *modSources,
                             const ImU32 categoryGlow) {
  if (DrawSectionBegin("KIFS", categoryGlow, &sectionKifs)) {
    const bool wasEnabled = e->kifs.enabled;
    ImGui::Checkbox("Enabled##kifs", &e->kifs.enabled);
    if (!wasEnabled && e->kifs.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_KIFS);
    }
    if (e->kifs.enabled) {
      KifsConfig *k = &e->kifs;

      ImGui::SliderInt("Iterations##kifs", &k->iterations, 1, 6);
      ImGui::SliderFloat("Scale##kifs", &k->scale, 1.5f, 2.5f, "%.2f");
      ImGui::SliderFloat("Offset X##kifs", &k->offsetX, 0.0f, 2.0f, "%.2f");
      ImGui::SliderFloat("Offset Y##kifs", &k->offsetY, 0.0f, 2.0f, "%.2f");
      ModulatableSliderSpeedDeg("Spin##kifs", &k->rotationSpeed,
                                "kifs.rotationSpeed", modSources);
      ModulatableSliderSpeedDeg("Twist##kifs", &k->twistSpeed,
                                "kifs.twistSpeed", modSources);
      ImGui::Checkbox("Octant Fold##kifs", &k->octantFold);
      ImGui::Checkbox("Polar Fold##kifs", &k->polarFold);
      if (k->polarFold) {
        ImGui::SliderInt("Segments##kifsPolar", &k->polarFoldSegments, 2, 12);
        ModulatableSlider("Smoothing##kifsPolar", &k->polarFoldSmoothing,
                          "kifs.polarFoldSmoothing", "%.2f", modSources);
      }
    }
    DrawSectionEnd();
  }
}

static void DrawSymmetryPoincare(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Poincare Disk", categoryGlow, &sectionPoincareDisk)) {
    const bool wasEnabled = e->poincareDisk.enabled;
    ImGui::Checkbox("Enabled##poincare", &e->poincareDisk.enabled);
    if (!wasEnabled && e->poincareDisk.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_POINCARE_DISK);
    }
    if (e->poincareDisk.enabled) {
      PoincareDiskConfig *pd = &e->poincareDisk;

      ImGui::SliderInt("Tile P##poincare", &pd->tileP, 2, 12);
      ImGui::SliderInt("Tile Q##poincare", &pd->tileQ, 2, 12);
      ImGui::SliderInt("Tile R##poincare", &pd->tileR, 2, 12);

      ModulatableSlider("Translation X##poincare", &pd->translationX,
                        "poincareDisk.translationX", "%.2f", modSources);
      ModulatableSlider("Translation Y##poincare", &pd->translationY,
                        "poincareDisk.translationY", "%.2f", modSources);
      ModulatableSlider("Disk Scale##poincare", &pd->diskScale,
                        "poincareDisk.diskScale", "%.2f", modSources);

      ModulatableSlider("Motion Radius##poincare", &pd->translationAmplitude,
                        "poincareDisk.translationAmplitude", "%.2f",
                        modSources);
      ModulatableSliderSpeedDeg("Motion Speed##poincare", &pd->translationSpeed,
                                "poincareDisk.translationSpeed", modSources);
      ModulatableSliderSpeedDeg("Rotation Speed##poincare", &pd->rotationSpeed,
                                "poincareDisk.rotationSpeed", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawSymmetryMandelbox(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
  if (DrawSectionBegin("Mandelbox", categoryGlow, &sectionMandelbox)) {
    const bool wasEnabled = e->mandelbox.enabled;
    ImGui::Checkbox("Enabled##mandelbox", &e->mandelbox.enabled);
    if (!wasEnabled && e->mandelbox.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MANDELBOX);
    }
    if (e->mandelbox.enabled) {
      MandelboxConfig *m = &e->mandelbox;

      ImGui::SliderInt("Iterations##mandelbox", &m->iterations, 1, 6);
      ImGui::SliderFloat("Scale##mandelbox", &m->scale, -3.0f, 3.0f, "%.2f");
      ImGui::SliderFloat("Offset X##mandelbox", &m->offsetX, 0.0f, 2.0f,
                         "%.2f");
      ImGui::SliderFloat("Offset Y##mandelbox", &m->offsetY, 0.0f, 2.0f,
                         "%.2f");
      ModulatableSliderSpeedDeg("Spin##mandelbox", &m->rotationSpeed,
                                "mandelbox.rotationSpeed", modSources);
      ModulatableSliderSpeedDeg("Twist##mandelbox", &m->twistSpeed,
                                "mandelbox.twistSpeed", modSources);

      if (TreeNodeAccented("Box Fold##mandelbox", categoryGlow)) {
        ImGui::SliderFloat("Limit##boxfold", &m->boxLimit, 0.5f, 2.0f, "%.2f");
        ModulatableSlider("Intensity##boxfold", &m->boxIntensity,
                          "mandelbox.boxIntensity", "%.2f", modSources);
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Sphere Fold##mandelbox", categoryGlow)) {
        ImGui::SliderFloat("Min Radius##spherefold", &m->sphereMin, 0.1f, 0.5f,
                           "%.2f");
        ImGui::SliderFloat("Max Radius##spherefold", &m->sphereMax, 0.5f, 2.0f,
                           "%.2f");
        ModulatableSlider("Intensity##spherefold", &m->sphereIntensity,
                          "mandelbox.sphereIntensity", "%.2f", modSources);
        TreeNodeAccentedPop();
      }

      ImGui::Checkbox("Polar Fold##mandelbox", &m->polarFold);
      if (m->polarFold) {
        ImGui::SliderInt("Segments##mandelboxPolar", &m->polarFoldSegments, 2,
                         12);
      }
    }
    DrawSectionEnd();
  }
}

static void DrawSymmetryTriangleFold(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
  if (DrawSectionBegin("Triangle Fold", categoryGlow, &sectionTriangleFold)) {
    const bool wasEnabled = e->triangleFold.enabled;
    ImGui::Checkbox("Enabled##trianglefold", &e->triangleFold.enabled);
    if (!wasEnabled && e->triangleFold.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_TRIANGLE_FOLD);
    }
    if (e->triangleFold.enabled) {
      TriangleFoldConfig *t = &e->triangleFold;

      ImGui::SliderInt("Iterations##trianglefold", &t->iterations, 1, 6);
      ImGui::SliderFloat("Scale##trianglefold", &t->scale, 1.5f, 2.5f, "%.2f");
      ImGui::SliderFloat("Offset X##trianglefold", &t->offsetX, 0.0f, 2.0f,
                         "%.2f");
      ImGui::SliderFloat("Offset Y##trianglefold", &t->offsetY, 0.0f, 2.0f,
                         "%.2f");
      ModulatableSliderSpeedDeg("Spin##trianglefold", &t->rotationSpeed,
                                "triangleFold.rotationSpeed", modSources);
      ModulatableSliderSpeedDeg("Twist##trianglefold", &t->twistSpeed,
                                "triangleFold.twistSpeed", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawSymmetryMoireInterference(EffectConfig *e,
                                          const ModSources *modSources,
                                          const ImU32 categoryGlow) {
  if (DrawSectionBegin("Moire Interference", categoryGlow,
                       &sectionMoireInterference)) {
    const bool wasEnabled = e->moireInterference.enabled;
    ImGui::Checkbox("Enabled##moire", &e->moireInterference.enabled);
    if (!wasEnabled && e->moireInterference.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MOIRE_INTERFERENCE);
    }
    if (e->moireInterference.enabled) {
      MoireInterferenceConfig *mi = &e->moireInterference;

      ModulatableSliderAngleDeg("Rotation##moire", &mi->rotationAngle,
                                "moireInterference.rotationAngle", modSources,
                                "%.1f °");
      ModulatableSlider("Scale Diff##moire", &mi->scaleDiff,
                        "moireInterference.scaleDiff", "%.3f", modSources);
      ImGui::SliderInt("Layers##moire", &mi->layers, 2, 4);
      ImGui::Combo("Blend Mode##moire", &mi->blendMode,
                   "Multiply\0Min\0Average\0Difference\0");
      ModulatableSliderSpeedDeg("Spin##moire", &mi->animationSpeed,
                                "moireInterference.animationSpeed", modSources);
      if (TreeNodeAccented("Center##moire", categoryGlow)) {
        ImGui::SliderFloat("X##moirecenter", &mi->centerX, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Y##moirecenter", &mi->centerY, 0.0f, 1.0f, "%.2f");
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawSymmetryRadialIfs(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
  if (DrawSectionBegin("Radial IFS", categoryGlow, &sectionRadialIfs)) {
    const bool wasEnabled = e->radialIfs.enabled;
    ImGui::Checkbox("Enabled##radialifs", &e->radialIfs.enabled);
    if (!wasEnabled && e->radialIfs.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_RADIAL_IFS);
    }
    if (e->radialIfs.enabled) {
      RadialIfsConfig *r = &e->radialIfs;

      ImGui::SliderInt("Segments##radialifs", &r->segments, 3, 12);
      ImGui::SliderInt("Iterations##radialifs", &r->iterations, 1, 8);
      ImGui::SliderFloat("Scale##radialifs", &r->scale, 1.2f, 2.5f, "%.2f");
      ImGui::SliderFloat("Offset##radialifs", &r->offset, 0.0f, 2.0f, "%.2f");
      ModulatableSliderSpeedDeg("Spin##radialifs", &r->rotationSpeed,
                                "radialIfs.rotationSpeed", modSources);
      ModulatableSliderSpeedDeg("Twist##radialifs", &r->twistSpeed,
                                "radialIfs.twistSpeed", modSources);
      ModulatableSlider("Smoothing##radialifs", &r->smoothing,
                        "radialIfs.smoothing", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

void DrawSymmetryCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(0);
  DrawCategoryHeader("Symmetry", categoryGlow);
  DrawSymmetryKaleidoscope(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawSymmetryKifs(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawSymmetryMoireInterference(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawSymmetryPoincare(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawSymmetryMandelbox(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawSymmetryTriangleFold(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawSymmetryRadialIfs(e, modSources, categoryGlow);
}
