// Dot matrix effect module implementation

#include "dot_matrix.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool DotMatrixEffectInit(DotMatrixEffect *e) {
  e->shader = LoadShader(NULL, "shaders/dot_matrix.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->dotScaleLoc = GetShaderLocation(e->shader, "dotScale");
  e->softnessLoc = GetShaderLocation(e->shader, "softness");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");

  e->rotation = 0.0f;

  return true;
}

void DotMatrixEffectSetup(DotMatrixEffect *e, const DotMatrixConfig *cfg,
                          float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;

  float finalRotation = e->rotation + cfg->rotationAngle;

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->dotScaleLoc, &cfg->dotScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->softnessLoc, &cfg->softness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &finalRotation,
                 SHADER_UNIFORM_FLOAT);
}

void DotMatrixEffectUninit(const DotMatrixEffect *e) {
  UnloadShader(e->shader);
}

void DotMatrixRegisterParams(DotMatrixConfig *cfg) {
  ModEngineRegisterParam("dotMatrix.dotScale", &cfg->dotScale, 4.0f, 80.0f);
  ModEngineRegisterParam("dotMatrix.softness", &cfg->softness, 0.2f, 4.0f);
  ModEngineRegisterParam("dotMatrix.brightness", &cfg->brightness, 0.5f, 8.0f);
  ModEngineRegisterParam("dotMatrix.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("dotMatrix.rotationAngle", &cfg->rotationAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

// === UI ===

static void DrawDotMatrixParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  DotMatrixConfig *d = &e->dotMatrix;

  ModulatableSlider("Scale##dotmtx", &d->dotScale, "dotMatrix.dotScale", "%.1f",
                    ms);
  ModulatableSlider("Softness##dotmtx", &d->softness, "dotMatrix.softness",
                    "%.2f", ms);
  ModulatableSlider("Brightness##dotmtx", &d->brightness,
                    "dotMatrix.brightness", "%.1f", ms);
  ModulatableSliderSpeedDeg("Spin##dotmtx", &d->rotationSpeed,
                            "dotMatrix.rotationSpeed", ms);
  ModulatableSliderAngleDeg("Angle##dotmtx", &d->rotationAngle,
                            "dotMatrix.rotationAngle", ms);
}

void SetupDotMatrix(PostEffect *pe) {
  DotMatrixEffectSetup(&pe->dotMatrix, &pe->effects.dotMatrix,
                       pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_DOT_MATRIX, DotMatrix, dotMatrix, "Dot Matrix",
                "CELL", 2, EFFECT_FLAG_NONE, SetupDotMatrix, NULL, DrawDotMatrixParams)
// clang-format on
