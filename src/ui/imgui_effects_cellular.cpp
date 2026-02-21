#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "effects/fracture_grid.h"
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
static bool sectionFractureGrid = false;

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

      ImGui::Combo("Mode##vor", &v->mode,
                   "Distort\0Organic Flow\0Edge Iso\0Center Iso\0Flat "
                   "Fill\0Edge Glow\0Ratio\0Determinant\0Edge Detect\0");
      ModulatableSlider("Intensity##vor", &v->intensity, "voronoi.intensity",
                        "%.2f", modSources);

      if (v->mode == 2 || v->mode == 3) {
        ModulatableSlider("Iso Frequency##vor", &v->isoFrequency,
                          "voronoi.isoFrequency", "%.1f", modSources);
      }
      if (v->mode == 1 || v->mode == 4 || v->mode == 5 || v->mode == 8) {
        ModulatableSlider("Edge Falloff##vor", &v->edgeFalloff,
                          "voronoi.edgeFalloff", "%.2f", modSources);
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

      ImGui::Combo("Mode##phyllo", &p->mode,
                   "Distort\0Organic Flow\0Edge Iso\0Center Iso\0Flat "
                   "Fill\0Edge Glow\0Ratio\0Determinant\0Edge Detect\0");
      ModulatableSlider("Intensity##phyllo", &p->intensity,
                        "phyllotaxis.intensity", "%.2f", modSources);

      if (p->mode == 2 || p->mode == 3) {
        ModulatableSlider("Iso Frequency##phyllo", &p->isoFrequency,
                          "phyllotaxis.isoFrequency", "%.1f", modSources);
      }
      if (p->mode == 1 || p->mode == 5 || p->mode == 8) {
        ModulatableSlider("Cell Radius##phyllo", &p->cellRadius,
                          "phyllotaxis.cellRadius", "%.2f", modSources);
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

static void DrawCellularFractureGrid(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
  if (DrawSectionBegin("Fracture Grid", categoryGlow, &sectionFractureGrid,
                       e->fractureGrid.enabled)) {
    const bool wasEnabled = e->fractureGrid.enabled;
    ImGui::Checkbox("Enabled##fracgrid", &e->fractureGrid.enabled);
    if (!wasEnabled && e->fractureGrid.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_FRACTURE_GRID);
    }
    if (e->fractureGrid.enabled) {
      ModulatableSlider("Subdivision##fracgrid", &e->fractureGrid.subdivision,
                        "fractureGrid.subdivision", "%.1f", modSources);
      ModulatableSlider("Stagger##fracgrid", &e->fractureGrid.stagger,
                        "fractureGrid.stagger", "%.2f", modSources);
      ModulatableSlider("Offset Scale##fracgrid", &e->fractureGrid.offsetScale,
                        "fractureGrid.offsetScale", "%.2f", modSources);
      ModulatableSlider("Rotation Scale##fracgrid",
                        &e->fractureGrid.rotationScale,
                        "fractureGrid.rotationScale", "%.2f", modSources);
      ModulatableSlider("Zoom Scale##fracgrid", &e->fractureGrid.zoomScale,
                        "fractureGrid.zoomScale", "%.2f", modSources);
      const char *tessNames[] = {"Rectangular", "Hexagonal", "Triangular"};
      ImGui::Combo("Tessellation##fracgrid", &e->fractureGrid.tessellation,
                   tessNames, 3);
      ModulatableSlider("Wave Speed##fracgrid", &e->fractureGrid.waveSpeed,
                        "fractureGrid.waveSpeed", "%.2f", modSources);
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
  ImGui::Spacing();
  DrawCellularFractureGrid(e, modSources, categoryGlow);
}
