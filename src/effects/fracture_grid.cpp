#include "fracture_grid.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool FractureGridEffectInit(FractureGridEffect *e) {
  e->shader = LoadShader(NULL, "shaders/fracture_grid.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->subdivisionLoc = GetShaderLocation(e->shader, "subdivision");
  e->staggerLoc = GetShaderLocation(e->shader, "stagger");
  e->offsetScaleLoc = GetShaderLocation(e->shader, "offsetScale");
  e->rotationScaleLoc = GetShaderLocation(e->shader, "rotationScale");
  e->zoomScaleLoc = GetShaderLocation(e->shader, "zoomScale");
  e->tessellationLoc = GetShaderLocation(e->shader, "tessellation");
  e->waveTimeLoc = GetShaderLocation(e->shader, "waveTime");
  e->waveTime = 0.0f;

  return true;
}

void FractureGridEffectSetup(FractureGridEffect *e,
                             const FractureGridConfig *cfg, float deltaTime,
                             int screenWidth, int screenHeight) {
  e->waveTime += cfg->waveSpeed * deltaTime;

  float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->subdivisionLoc, &cfg->subdivision,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->staggerLoc, &cfg->stagger, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetScaleLoc, &cfg->offsetScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationScaleLoc, &cfg->rotationScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomScaleLoc, &cfg->zoomScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tessellationLoc, &cfg->tessellation,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->waveTimeLoc, &e->waveTime, SHADER_UNIFORM_FLOAT);
}

void FractureGridEffectUninit(FractureGridEffect *e) {
  UnloadShader(e->shader);
}

FractureGridConfig FractureGridConfigDefault(void) {
  return FractureGridConfig{};
}

void FractureGridRegisterParams(FractureGridConfig *cfg) {
  ModEngineRegisterParam("fractureGrid.subdivision", &cfg->subdivision, 0.0f,
                         20.0f);
  ModEngineRegisterParam("fractureGrid.stagger", &cfg->stagger, 0.0f, 1.0f);
  ModEngineRegisterParam("fractureGrid.offsetScale", &cfg->offsetScale, 0.0f,
                         1.0f);
  ModEngineRegisterParam("fractureGrid.rotationScale", &cfg->rotationScale,
                         0.0f, PI_F);
  ModEngineRegisterParam("fractureGrid.zoomScale", &cfg->zoomScale, 0.0f, 4.0f);
  ModEngineRegisterParam("fractureGrid.waveSpeed", &cfg->waveSpeed, 0.0f, 5.0f);
}

// === UI ===

static void DrawFractureGridParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  const char *tessNames[] = {"Rectangular", "Hexagonal", "Triangular"};

  ModulatableSlider("Subdivision##fracgrid", &e->fractureGrid.subdivision,
                    "fractureGrid.subdivision", "%.1f", ms);
  ModulatableSlider("Stagger##fracgrid", &e->fractureGrid.stagger,
                    "fractureGrid.stagger", "%.2f", ms);
  ModulatableSlider("Offset Scale##fracgrid", &e->fractureGrid.offsetScale,
                    "fractureGrid.offsetScale", "%.2f", ms);
  ModulatableSlider("Rotation Scale##fracgrid", &e->fractureGrid.rotationScale,
                    "fractureGrid.rotationScale", "%.2f", ms);
  ModulatableSlider("Zoom Scale##fracgrid", &e->fractureGrid.zoomScale,
                    "fractureGrid.zoomScale", "%.2f", ms);
  ImGui::Combo("Tessellation##fracgrid", &e->fractureGrid.tessellation,
               tessNames, 3);
  ModulatableSlider("Wave Speed##fracgrid", &e->fractureGrid.waveSpeed,
                    "fractureGrid.waveSpeed", "%.2f", ms);
}

void SetupFractureGrid(PostEffect *pe) {
  FractureGridEffectSetup(&pe->fractureGrid, &pe->effects.fractureGrid,
                          pe->currentDeltaTime, pe->screenWidth,
                          pe->screenHeight);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_FRACTURE_GRID, FractureGrid, fractureGrid, "Fracture Grid",
                "CELL", 2, EFFECT_FLAG_NONE, SetupFractureGrid, NULL, DrawFractureGridParams)
// clang-format on
