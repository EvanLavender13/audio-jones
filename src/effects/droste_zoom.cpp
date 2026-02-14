#include "droste_zoom.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include "render/shader_setup_motion.h"
#include <stddef.h>

bool DrosteZoomEffectInit(DrosteZoomEffect *e) {
  e->shader = LoadShader(NULL, "shaders/droste_zoom.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->spiralAngleLoc = GetShaderLocation(e->shader, "spiralAngle");
  e->shearCoeffLoc = GetShaderLocation(e->shader, "shearCoeff");
  e->innerRadiusLoc = GetShaderLocation(e->shader, "innerRadius");
  e->branchesLoc = GetShaderLocation(e->shader, "branches");

  e->time = 0.0f;

  return true;
}

void DrosteZoomEffectSetup(DrosteZoomEffect *e, const DrosteZoomConfig *cfg,
                           float deltaTime) {
  e->time += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralAngleLoc, &cfg->spiralAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shearCoeffLoc, &cfg->shearCoeff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->innerRadiusLoc, &cfg->innerRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->branchesLoc, &cfg->branches, SHADER_UNIFORM_INT);
}

void DrosteZoomEffectUninit(DrosteZoomEffect *e) { UnloadShader(e->shader); }

DrosteZoomConfig DrosteZoomConfigDefault(void) { return DrosteZoomConfig{}; }

void DrosteZoomRegisterParams(DrosteZoomConfig *cfg) {
  ModEngineRegisterParam("drosteZoom.scale", &cfg->scale, 1.5f, 10.0f);
  ModEngineRegisterParam("drosteZoom.spiralAngle", &cfg->spiralAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("drosteZoom.shearCoeff", &cfg->shearCoeff, -1.0f,
                         1.0f);
  ModEngineRegisterParam("drosteZoom.innerRadius", &cfg->innerRadius, 0.0f,
                         0.5f);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_DROSTE_ZOOM, DrosteZoom, drosteZoom, "Droste Zoom",
                "MOT", 3, EFFECT_FLAG_NONE, SetupDrosteZoom, NULL)
// clang-format on
