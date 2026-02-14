#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionVoronoi = false;
static bool sectionLatticeFold = false;
static bool sectionPhyllotaxis = false;
static bool sectionMultiScaleGrid = false;
static bool sectionDotMatrix = false;

static void DrawCellularVoronoi(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Voronoi", categoryGlow, &sectionVoronoi,
                       e->voronoi.enabled)) {
    const bool wasEnabled = e->voronoi.enabled;
    ImGui::Checkbox("Enabled##vor", &e->voronoi.enabled);
    if (!wasEnabled && e->voronoi.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_VORONOI);
    }
    if (e->voronoi.enabled) {
      VoronoiConfig *v = &e->voronoi;

      ModulatableSlider("Scale##vor", &v->scale, "voronoi.scale", "%.1f",
                        modSources);
      ModulatableSlider("Speed##vor", &v->speed, "voronoi.speed", "%.2f",
                        modSources);
      ImGui::Checkbox("Smooth##vor", &v->smoothMode);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Effects");
      ImGui::Spacing();

      const bool uvDistortActive = IntensityToggleButton(
          "Distort", &v->uvDistortIntensity, "voronoi.uvDistortIntensity",
          Theme::ACCENT_CYAN_U32);
      ImGui::SameLine();
      const bool edgeIsoActive = IntensityToggleButton(
          "Edge Iso", &v->edgeIsoIntensity, "voronoi.edgeIsoIntensity",
          Theme::ACCENT_MAGENTA_U32);
      ImGui::SameLine();
      const bool centerIsoActive = IntensityToggleButton(
          "Ctr Iso", &v->centerIsoIntensity, "voronoi.centerIsoIntensity",
          Theme::ACCENT_ORANGE_U32);

      const bool flatFillActive = IntensityToggleButton(
          "Fill", &v->flatFillIntensity, "voronoi.flatFillIntensity",
          Theme::ACCENT_CYAN_U32);
      ImGui::SameLine();
      const bool organicFlowActive = IntensityToggleButton(
          "Organic", &v->organicFlowIntensity, "voronoi.organicFlowIntensity",
          Theme::ACCENT_MAGENTA_U32);
      ImGui::SameLine();
      const bool edgeGlowActive = IntensityToggleButton(
          "Glow", &v->edgeGlowIntensity, "voronoi.edgeGlowIntensity",
          Theme::ACCENT_ORANGE_U32);

      const bool determinantActive = IntensityToggleButton(
          "Determ", &v->determinantIntensity, "voronoi.determinantIntensity",
          Theme::ACCENT_CYAN_U32);
      ImGui::SameLine();
      const bool ratioActive = IntensityToggleButton(
          "Ratio", &v->ratioIntensity, "voronoi.ratioIntensity",
          Theme::ACCENT_MAGENTA_U32);
      ImGui::SameLine();
      const bool edgeDetectActive = IntensityToggleButton(
          "Detect", &v->edgeDetectIntensity, "voronoi.edgeDetectIntensity",
          Theme::ACCENT_ORANGE_U32);

      const int activeCount =
          (uvDistortActive ? 1 : 0) + (edgeIsoActive ? 1 : 0) +
          (centerIsoActive ? 1 : 0) + (flatFillActive ? 1 : 0) +
          (organicFlowActive ? 1 : 0) + (edgeGlowActive ? 1 : 0) +
          (determinantActive ? 1 : 0) + (ratioActive ? 1 : 0) +
          (edgeDetectActive ? 1 : 0);

      if (activeCount > 1) {
        ImGui::Spacing();
        ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                           "Blend Mix");
        if (uvDistortActive) {
          ImGui::SliderFloat("Distort##mix", &v->uvDistortIntensity, 0.01f,
                             1.0f, "%.2f");
        }
        if (edgeIsoActive) {
          ImGui::SliderFloat("Edge Iso##mix", &v->edgeIsoIntensity, 0.01f, 1.0f,
                             "%.2f");
        }
        if (centerIsoActive) {
          ImGui::SliderFloat("Ctr Iso##mix", &v->centerIsoIntensity, 0.01f,
                             1.0f, "%.2f");
        }
        if (flatFillActive) {
          ImGui::SliderFloat("Fill##mix", &v->flatFillIntensity, 0.01f, 1.0f,
                             "%.2f");
        }
        if (organicFlowActive) {
          ImGui::SliderFloat("Organic##mix", &v->organicFlowIntensity, 0.01f,
                             1.0f, "%.2f");
        }
        if (edgeGlowActive) {
          ImGui::SliderFloat("Glow##mix", &v->edgeGlowIntensity, 0.01f, 1.0f,
                             "%.2f");
        }
        if (determinantActive) {
          ImGui::SliderFloat("Determ##mix", &v->determinantIntensity, 0.01f,
                             1.0f, "%.2f");
        }
        if (ratioActive) {
          ImGui::SliderFloat("Ratio##mix", &v->ratioIntensity, 0.01f, 1.0f,
                             "%.2f");
        }
        if (edgeDetectActive) {
          ImGui::SliderFloat("Detect##mix", &v->edgeDetectIntensity, 0.01f,
                             1.0f, "%.2f");
        }
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (TreeNodeAccented("Iso Settings##vor", categoryGlow)) {
        ModulatableSlider("Frequency", &v->isoFrequency, "voronoi.isoFrequency",
                          "%.1f", modSources);
        ModulatableSlider("Edge Falloff", &v->edgeFalloff,
                          "voronoi.edgeFalloff", "%.2f", modSources);
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawCellularLatticeFold(EffectConfig *e,
                                    const ModSources *modSources,
                                    const ImU32 categoryGlow) {
  if (DrawSectionBegin("Lattice Fold", categoryGlow, &sectionLatticeFold,
                       e->latticeFold.enabled)) {
    const bool wasEnabled = e->latticeFold.enabled;
    ImGui::Checkbox("Enabled##lattice", &e->latticeFold.enabled);
    if (!wasEnabled && e->latticeFold.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_LATTICE_FOLD);
    }
    if (e->latticeFold.enabled) {
      LatticeFoldConfig *l = &e->latticeFold;

      const char *cellTypeNames[] = {"Square", "Hexagon"};
      int cellTypeIndex = (l->cellType == 4) ? 0 : 1;
      if (ImGui::Combo("Cell Type##lattice", &cellTypeIndex, cellTypeNames,
                       2)) {
        l->cellType = (cellTypeIndex == 0) ? 4 : 6;
      }
      ModulatableSlider("Cell Scale##lattice", &l->cellScale,
                        "latticeFold.cellScale", "%.1f", modSources);
      ModulatableSliderSpeedDeg("Spin##lattice", &l->rotationSpeed,
                                "latticeFold.rotationSpeed", modSources);
      ModulatableSlider("Smoothing##lattice", &l->smoothing,
                        "latticeFold.smoothing", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawCellularPhyllotaxis(EffectConfig *e,
                                    const ModSources *modSources,
                                    const ImU32 categoryGlow) {
  if (DrawSectionBegin("Phyllotaxis", categoryGlow, &sectionPhyllotaxis,
                       e->phyllotaxis.enabled)) {
    const bool wasEnabled = e->phyllotaxis.enabled;
    ImGui::Checkbox("Enabled##phyllo", &e->phyllotaxis.enabled);
    if (!wasEnabled && e->phyllotaxis.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_PHYLLOTAXIS);
    }
    if (e->phyllotaxis.enabled) {
      PhyllotaxisConfig *p = &e->phyllotaxis;

      ModulatableSlider("Scale##phyllo", &p->scale, "phyllotaxis.scale", "%.3f",
                        modSources);
      ImGui::Checkbox("Smooth##phyllo", &p->smoothMode);
      ModulatableSliderAngleDeg("Angle##phyllo", &p->divergenceAngle,
                                "phyllotaxis.divergenceAngle", modSources,
                                "%.1f deg");
      ModulatableSliderSpeedDeg("Angle Drift##phyllo", &p->angleSpeed,
                                "phyllotaxis.angleSpeed", modSources,
                                "%.2f °/s");
      ModulatableSliderSpeedDeg("Phase Pulse##phyllo", &p->phaseSpeed,
                                "phyllotaxis.phaseSpeed", modSources);
      ModulatableSliderSpeedDeg("Spin Speed##phyllo", &p->spinSpeed,
                                "phyllotaxis.spinSpeed", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Effects");
      ImGui::Spacing();

      const bool uvDistortActive = IntensityToggleButton(
          "Distort##phyllo", &p->uvDistortIntensity,
          "phyllotaxis.uvDistortIntensity", Theme::ACCENT_CYAN_U32);
      ImGui::SameLine();
      const bool organicFlowActive = IntensityToggleButton(
          "Organic##phyllo", &p->organicFlowIntensity,
          "phyllotaxis.organicFlowIntensity", Theme::ACCENT_MAGENTA_U32);
      ImGui::SameLine();
      const bool edgeIsoActive = IntensityToggleButton(
          "Edge Iso##phyllo", &p->edgeIsoIntensity,
          "phyllotaxis.edgeIsoIntensity", Theme::ACCENT_ORANGE_U32);

      const bool centerIsoActive = IntensityToggleButton(
          "Ctr Iso##phyllo", &p->centerIsoIntensity,
          "phyllotaxis.centerIsoIntensity", Theme::ACCENT_CYAN_U32);
      ImGui::SameLine();
      const bool flatFillActive = IntensityToggleButton(
          "Fill##phyllo", &p->flatFillIntensity,
          "phyllotaxis.flatFillIntensity", Theme::ACCENT_MAGENTA_U32);
      ImGui::SameLine();
      const bool edgeGlowActive = IntensityToggleButton(
          "Glow##phyllo", &p->edgeGlowIntensity,
          "phyllotaxis.edgeGlowIntensity", Theme::ACCENT_ORANGE_U32);

      const bool ratioActive = IntensityToggleButton(
          "Ratio##phyllo", &p->ratioIntensity, "phyllotaxis.ratioIntensity",
          Theme::ACCENT_CYAN_U32);
      ImGui::SameLine();
      const bool determinantActive = IntensityToggleButton(
          "Determ##phyllo", &p->determinantIntensity,
          "phyllotaxis.determinantIntensity", Theme::ACCENT_MAGENTA_U32);
      ImGui::SameLine();
      const bool edgeDetectActive = IntensityToggleButton(
          "Detect##phyllo", &p->edgeDetectIntensity,
          "phyllotaxis.edgeDetectIntensity", Theme::ACCENT_ORANGE_U32);

      const int activeCount =
          (uvDistortActive ? 1 : 0) + (organicFlowActive ? 1 : 0) +
          (edgeIsoActive ? 1 : 0) + (centerIsoActive ? 1 : 0) +
          (flatFillActive ? 1 : 0) + (edgeGlowActive ? 1 : 0) +
          (ratioActive ? 1 : 0) + (determinantActive ? 1 : 0) +
          (edgeDetectActive ? 1 : 0);

      if (activeCount > 1) {
        ImGui::Spacing();
        ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                           "Blend Mix");
        if (uvDistortActive) {
          ImGui::SliderFloat("Distort##phyllomix", &p->uvDistortIntensity,
                             0.01f, 1.0f, "%.2f");
        }
        if (organicFlowActive) {
          ImGui::SliderFloat("Organic##phyllomix", &p->organicFlowIntensity,
                             0.01f, 1.0f, "%.2f");
        }
        if (edgeIsoActive) {
          ImGui::SliderFloat("Edge Iso##phyllomix", &p->edgeIsoIntensity, 0.01f,
                             1.0f, "%.2f");
        }
        if (centerIsoActive) {
          ImGui::SliderFloat("Ctr Iso##phyllomix", &p->centerIsoIntensity,
                             0.01f, 1.0f, "%.2f");
        }
        if (flatFillActive) {
          ImGui::SliderFloat("Fill##phyllomix", &p->flatFillIntensity, 0.01f,
                             1.0f, "%.2f");
        }
        if (edgeGlowActive) {
          ImGui::SliderFloat("Glow##phyllomix", &p->edgeGlowIntensity, 0.01f,
                             1.0f, "%.2f");
        }
        if (ratioActive) {
          ImGui::SliderFloat("Ratio##phyllomix", &p->ratioIntensity, 0.01f,
                             1.0f, "%.2f");
        }
        if (determinantActive) {
          ImGui::SliderFloat("Determ##phyllomix", &p->determinantIntensity,
                             0.01f, 1.0f, "%.2f");
        }
        if (edgeDetectActive) {
          ImGui::SliderFloat("Detect##phyllomix", &p->edgeDetectIntensity,
                             0.01f, 1.0f, "%.2f");
        }
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (TreeNodeAccented("Iso Settings##phyllo", categoryGlow)) {
        ModulatableSlider("Frequency##phyllo", &p->isoFrequency,
                          "phyllotaxis.isoFrequency", "%.1f", modSources);
        ModulatableSlider("Cell Radius##phyllo", &p->cellRadius,
                          "phyllotaxis.cellRadius", "%.2f", modSources);
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawCellularMultiScaleGrid(EffectConfig *e,
                                       const ModSources *modSources,
                                       const ImU32 categoryGlow) {
  if (DrawSectionBegin("Multi-Scale Grid", categoryGlow, &sectionMultiScaleGrid,
                       e->multiScaleGrid.enabled)) {
    const bool wasEnabled = e->multiScaleGrid.enabled;
    ImGui::Checkbox("Enabled##msg", &e->multiScaleGrid.enabled);
    if (!wasEnabled && e->multiScaleGrid.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MULTI_SCALE_GRID);
    }
    if (e->multiScaleGrid.enabled) {
      MultiScaleGridConfig *g = &e->multiScaleGrid;

      ModulatableSlider("Coarse Scale##msg", &g->scale1,
                        "multiScaleGrid.scale1", "%.1f", modSources);
      ModulatableSlider("Medium Scale##msg", &g->scale2,
                        "multiScaleGrid.scale2", "%.1f", modSources);
      ModulatableSlider("Fine Scale##msg", &g->scale3, "multiScaleGrid.scale3",
                        "%.1f", modSources);
      ModulatableSlider("Warp##msg", &g->warpAmount,
                        "multiScaleGrid.warpAmount", "%.2f", modSources);
      ModulatableSlider("Edge Contrast##msg", &g->edgeContrast,
                        "multiScaleGrid.edgeContrast", "%.2f", modSources);
      ModulatableSlider("Edge Power##msg", &g->edgePower,
                        "multiScaleGrid.edgePower", "%.1f", modSources);
      ModulatableSlider("Glow Threshold##msg", &g->glowThreshold,
                        "multiScaleGrid.glowThreshold", "%.2f", modSources);
      ModulatableSlider("Glow Amount##msg", &g->glowAmount,
                        "multiScaleGrid.glowAmount", "%.1f", modSources);
      ModulatableSlider("Cell Variation##msg", &g->cellVariation,
                        "multiScaleGrid.cellVariation", "%.2f", modSources);
      ImGui::Combo("Glow Mode##msg", &g->glowMode, "Hard\0Soft\0");
    }
    DrawSectionEnd();
  }
}

static void DrawCellularDotMatrix(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
  if (DrawSectionBegin("Dot Matrix", categoryGlow, &sectionDotMatrix,
                       e->dotMatrix.enabled)) {
    const bool wasEnabled = e->dotMatrix.enabled;
    ImGui::Checkbox("Enabled##dotmtx", &e->dotMatrix.enabled);
    if (!wasEnabled && e->dotMatrix.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_DOT_MATRIX);
    }
    if (e->dotMatrix.enabled) {
      DotMatrixConfig *d = &e->dotMatrix;

      ModulatableSlider("Scale##dotmtx", &d->dotScale, "dotMatrix.dotScale",
                        "%.1f", modSources);
      ModulatableSlider("Softness##dotmtx", &d->softness, "dotMatrix.softness",
                        "%.2f", modSources);
      ModulatableSlider("Brightness##dotmtx", &d->brightness,
                        "dotMatrix.brightness", "%.1f", modSources);
      ModulatableSliderSpeedDeg("Spin##dotmtx", &d->rotationSpeed,
                                "dotMatrix.rotationSpeed", modSources);
      ModulatableSliderAngleDeg("Angle##dotmtx", &d->rotationAngle,
                                "dotMatrix.rotationAngle", modSources);
    }
    DrawSectionEnd();
  }
}

void DrawCellularCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(2);
  DrawCategoryHeader("Cellular", categoryGlow);
  DrawCellularVoronoi(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawCellularLatticeFold(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawCellularPhyllotaxis(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawCellularMultiScaleGrid(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawCellularDotMatrix(e, modSources, categoryGlow);
}
