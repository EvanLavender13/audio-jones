#include "fracture_grid.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
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
  e->waveShapeLoc = GetShaderLocation(e->shader, "waveShape");
  e->spatialBiasLoc = GetShaderLocation(e->shader, "spatialBias");
  e->flipChanceLoc = GetShaderLocation(e->shader, "flipChance");
  e->skewScaleLoc = GetShaderLocation(e->shader, "skewScale");
  e->propagationModeLoc = GetShaderLocation(e->shader, "propagationMode");
  e->propagationSpeedLoc = GetShaderLocation(e->shader, "propagationSpeed");
  e->propagationAngleLoc = GetShaderLocation(e->shader, "propagationAngle");
  e->propagationPhaseLoc = GetShaderLocation(e->shader, "propagationPhase");
  e->waveTime = 0.0f;
  e->propagationPhase = 0.0f;

  return true;
}

void FractureGridEffectSetup(FractureGridEffect *e,
                             const FractureGridConfig *cfg, float deltaTime,
                             int screenWidth, int screenHeight) {
  e->waveTime += cfg->waveSpeed * deltaTime;
  e->propagationPhase += cfg->propagationSpeed * deltaTime;

  const float resolution[2] = {(float)screenWidth, (float)screenHeight};
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
  SetShaderValue(e->shader, e->waveShapeLoc, &cfg->waveShape,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spatialBiasLoc, &cfg->spatialBias,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flipChanceLoc, &cfg->flipChance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->skewScaleLoc, &cfg->skewScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->propagationModeLoc, &cfg->propagationMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->propagationSpeedLoc, &cfg->propagationSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->propagationAngleLoc, &cfg->propagationAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->propagationPhaseLoc, &e->propagationPhase,
                 SHADER_UNIFORM_FLOAT);
}

void FractureGridEffectUninit(const FractureGridEffect *e) {
  UnloadShader(e->shader);
}

void FractureGridRegisterParams(FractureGridConfig *cfg) {
  ModEngineRegisterParam("fractureGrid.subdivision", &cfg->subdivision, 0.0f,
                         20.0f);
  ModEngineRegisterParam("fractureGrid.stagger", &cfg->stagger, 0.0f, 1.0f);
  ModEngineRegisterParam("fractureGrid.offsetScale", &cfg->offsetScale, 0.0f,
                         1.0f);
  ModEngineRegisterParam("fractureGrid.rotationScale", &cfg->rotationScale,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("fractureGrid.zoomScale", &cfg->zoomScale, 0.0f, 4.0f);
  ModEngineRegisterParam("fractureGrid.waveSpeed", &cfg->waveSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("fractureGrid.waveShape", &cfg->waveShape, 0.0f, 1.0f);
  ModEngineRegisterParam("fractureGrid.spatialBias", &cfg->spatialBias, 0.0f,
                         1.0f);
  ModEngineRegisterParam("fractureGrid.flipChance", &cfg->flipChance, 0.0f,
                         1.0f);
  ModEngineRegisterParam("fractureGrid.skewScale", &cfg->skewScale, 0.0f, 2.0f);
  ModEngineRegisterParam("fractureGrid.propagationSpeed",
                         &cfg->propagationSpeed, 0.0f, 20.0f);
  ModEngineRegisterParam("fractureGrid.propagationAngle",
                         &cfg->propagationAngle, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
}

// === UI ===

static void DrawFractureGridParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  const char *tessNames[] = {"Rectangular", "Hexagonal", "Triangular"};

  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Subdivision##fracgrid", &e->fractureGrid.subdivision,
                    "fractureGrid.subdivision", "%.1f", ms);
  ModulatableSlider("Stagger##fracgrid", &e->fractureGrid.stagger,
                    "fractureGrid.stagger", "%.2f", ms);
  ModulatableSlider("Offset Scale##fracgrid", &e->fractureGrid.offsetScale,
                    "fractureGrid.offsetScale", "%.2f", ms);
  ModulatableSliderAngleDeg("Rotation##fracgrid",
                            &e->fractureGrid.rotationScale,
                            "fractureGrid.rotationScale", ms);
  ModulatableSlider("Zoom Scale##fracgrid", &e->fractureGrid.zoomScale,
                    "fractureGrid.zoomScale", "%.2f", ms);
  ModulatableSlider("Flip Chance##fracgrid", &e->fractureGrid.flipChance,
                    "fractureGrid.flipChance", "%.2f", ms);
  ModulatableSlider("Skew##fracgrid", &e->fractureGrid.skewScale,
                    "fractureGrid.skewScale", "%.2f", ms);
  ImGui::Combo("Tessellation##fracgrid", &e->fractureGrid.tessellation,
               tessNames, 3);
  ModulatableSlider("Spatial Bias##fracgrid", &e->fractureGrid.spatialBias,
                    "fractureGrid.spatialBias", "%.2f", ms);

  ImGui::SeparatorText("Animation");
  ModulatableSlider("Wave Speed##fracgrid", &e->fractureGrid.waveSpeed,
                    "fractureGrid.waveSpeed", "%.2f", ms);
  ModulatableSlider("Wave Shape##fracgrid", &e->fractureGrid.waveShape,
                    "fractureGrid.waveShape", "%.2f", ms);

  const char *propNames[] = {"Noise", "Directional", "Radial", "Cascade"};
  ImGui::Combo("Propagation##fracgrid", &e->fractureGrid.propagationMode,
               propNames, 4);
  ModulatableSlider("Prop. Speed##fracgrid", &e->fractureGrid.propagationSpeed,
                    "fractureGrid.propagationSpeed", "%.1f", ms);
  if (e->fractureGrid.propagationMode == 1) {
    ModulatableSliderAngleDeg("Prop. Angle##fracgrid",
                              &e->fractureGrid.propagationAngle,
                              "fractureGrid.propagationAngle", ms);
  }
}

FractureGridEffect *GetFractureGridEffect(PostEffect *pe) {
  return (FractureGridEffect *)pe->effectStates[TRANSFORM_FRACTURE_GRID];
}

void SetupFractureGrid(PostEffect *pe) {
  FractureGridEffectSetup(GetFractureGridEffect(pe), &pe->effects.fractureGrid,
                          pe->currentDeltaTime, pe->screenWidth,
                          pe->screenHeight);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_FRACTURE_GRID, FractureGrid, fractureGrid, "Fracture Grid",
                "CELL", 2, EFFECT_FLAG_NONE, SetupFractureGrid, NULL, DrawFractureGridParams)
// clang-format on
