#include "radial_ifs.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool RadialIfsEffectInit(RadialIfsEffect *e) {
  e->shader = LoadShader(NULL, "shaders/radial_ifs.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->segmentsLoc = GetShaderLocation(e->shader, "segments");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->offsetLoc = GetShaderLocation(e->shader, "offset");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->twistAngleLoc = GetShaderLocation(e->shader, "twistAngle");
  e->smoothingLoc = GetShaderLocation(e->shader, "smoothing");

  e->rotation = 0.0f;
  e->twist = 0.0f;

  return true;
}

void RadialIfsEffectSetup(RadialIfsEffect *e, const RadialIfsConfig *cfg,
                          float deltaTime) {
  // Accumulate animation state
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->twist += cfg->twistSpeed * deltaTime;

  // Set uniforms
  SetShaderValue(e->shader, e->segmentsLoc, &cfg->segments, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetLoc, &cfg->offset, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAngleLoc, &e->twist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->smoothingLoc, &cfg->smoothing,
                 SHADER_UNIFORM_FLOAT);
}

void RadialIfsEffectUninit(RadialIfsEffect *e) { UnloadShader(e->shader); }

RadialIfsConfig RadialIfsConfigDefault(void) { return RadialIfsConfig{}; }

void RadialIfsRegisterParams(RadialIfsConfig *cfg) {
  ModEngineRegisterParam("radialIfs.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("radialIfs.twistSpeed", &cfg->twistSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("radialIfs.smoothing", &cfg->smoothing, 0.0f, 0.5f);
}

// clang-format off
REGISTER_EFFECT(
    TRANSFORM_RADIAL_IFS, RadialIfs, radialIfs,
    "Radial IFS", "SYM", 0, EFFECT_FLAG_NONE,
    SetupRadialIfs, NULL)
// clang-format on
