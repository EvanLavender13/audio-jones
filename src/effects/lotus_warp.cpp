// Lotus Warp log-polar conformal coordinate warp with cellular sub-effects

#include "lotus_warp.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool LotusWarpEffectInit(LotusWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/lotus_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->zoomPhaseLoc = GetShaderLocation(e->shader, "zoomPhase");
  e->spinPhaseLoc = GetShaderLocation(e->shader, "spinPhase");
  e->edgeFalloffLoc = GetShaderLocation(e->shader, "edgeFalloff");
  e->isoFrequencyLoc = GetShaderLocation(e->shader, "isoFrequency");
  e->modeLoc = GetShaderLocation(e->shader, "mode");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");

  e->zoomPhase = 0.0f;
  e->spinPhase = 0.0f;

  return true;
}

void LotusWarpEffectSetup(LotusWarpEffect *e, const LotusWarpConfig *cfg,
                          float deltaTime) {
  e->zoomPhase += cfg->zoomSpeed * deltaTime;
  e->spinPhase += cfg->spinSpeed * deltaTime;

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomPhaseLoc, &e->zoomPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spinPhaseLoc, &e->spinPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeFalloffLoc, &cfg->edgeFalloff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->isoFrequencyLoc, &cfg->isoFrequency,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
}

void LotusWarpEffectUninit(const LotusWarpEffect *e) {
  UnloadShader(e->shader);
}

void LotusWarpRegisterParams(LotusWarpConfig *cfg) {
  ModEngineRegisterParam("lotusWarp.scale", &cfg->scale, 0.5f, 10.0f);
  ModEngineRegisterParam("lotusWarp.zoomSpeed", &cfg->zoomSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("lotusWarp.spinSpeed", &cfg->spinSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("lotusWarp.edgeFalloff", &cfg->edgeFalloff, 0.01f,
                         1.0f);
  ModEngineRegisterParam("lotusWarp.isoFrequency", &cfg->isoFrequency, 1.0f,
                         30.0f);
  ModEngineRegisterParam("lotusWarp.intensity", &cfg->intensity, 0.0f, 1.0f);
}

// === UI ===

static void DrawLotusWarpParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  LotusWarpConfig *lw = &e->lotusWarp;

  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Scale##lw", &lw->scale, "lotusWarp.scale", "%.1f", ms);
  ImGui::SeparatorText("Cell Mode");
  ImGui::Combo("Mode##lw", &lw->mode,
               "Distort\0Organic Flow\0Edge Iso\0Center Iso\0Flat "
               "Fill\0Edge Glow\0Ratio\0Determinant\0Edge Detect\0");
  ModulatableSlider("Intensity##lw", &lw->intensity, "lotusWarp.intensity",
                    "%.2f", ms);

  if (lw->mode == 2 || lw->mode == 3) {
    ModulatableSlider("Iso Frequency##lw", &lw->isoFrequency,
                      "lotusWarp.isoFrequency", "%.1f", ms);
  }
  if (lw->mode == 1 || lw->mode == 4 || lw->mode == 5 || lw->mode == 8) {
    ModulatableSlider("Edge Falloff##lw", &lw->edgeFalloff,
                      "lotusWarp.edgeFalloff", "%.2f", ms);
  }
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Zoom Speed##lw", &lw->zoomSpeed,
                            "lotusWarp.zoomSpeed", ms);
  ModulatableSliderSpeedDeg("Spin Speed##lw", &lw->spinSpeed,
                            "lotusWarp.spinSpeed", ms);
}

void SetupLotusWarp(PostEffect *pe) {
  LotusWarpEffectSetup(&pe->lotusWarp, &pe->effects.lotusWarp,
                       pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_LOTUS_WARP, LotusWarp, lotusWarp, "Lotus Warp",
                "CELL", 2, EFFECT_FLAG_NONE, SetupLotusWarp, NULL,
                DrawLotusWarpParams)
// clang-format on
