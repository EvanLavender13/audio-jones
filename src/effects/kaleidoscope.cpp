#include "kaleidoscope.h"

#include <stddef.h>

bool KaleidoscopeEffectInit(KaleidoscopeEffect *e) {
  e->shader = LoadShader(NULL, "shaders/kaleidoscope.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->segmentsLoc = GetShaderLocation(e->shader, "segments");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->twistAngleLoc = GetShaderLocation(e->shader, "twistAngle");
  e->smoothingLoc = GetShaderLocation(e->shader, "smoothing");

  e->rotation = 0.0f;

  return true;
}

void KaleidoscopeEffectSetup(KaleidoscopeEffect *e,
                             const KaleidoscopeConfig *cfg, float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;

  SetShaderValue(e->shader, e->segmentsLoc, &cfg->segments, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAngleLoc, &cfg->twistAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->smoothingLoc, &cfg->smoothing,
                 SHADER_UNIFORM_FLOAT);
}

void KaleidoscopeEffectUninit(KaleidoscopeEffect *e) {
  UnloadShader(e->shader);
}

KaleidoscopeConfig KaleidoscopeConfigDefault(void) {
  return KaleidoscopeConfig{};
}
