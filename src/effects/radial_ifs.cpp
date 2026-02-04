#include "radial_ifs.h"

#include <stddef.h>

bool RadialIfsEffectInit(RadialIfsEffect *e) {
  if (e == NULL) {
    return false;
  }

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
  if (e == NULL || cfg == NULL) {
    return;
  }

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

void RadialIfsEffectUninit(RadialIfsEffect *e) {
  if (e == NULL) {
    return;
  }

  UnloadShader(e->shader);
}

RadialIfsConfig RadialIfsConfigDefault(void) {
  RadialIfsConfig cfg;
  cfg.enabled = false;
  cfg.segments = 6;
  cfg.iterations = 4;
  cfg.scale = 1.8f;
  cfg.offset = 0.5f;
  cfg.rotationSpeed = 0.0f;
  cfg.twistSpeed = 0.0f;
  cfg.smoothing = 0.0f;
  return cfg;
}
