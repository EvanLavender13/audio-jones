#include "mobius.h"

#include <stddef.h>

bool MobiusEffectInit(MobiusEffect *e) {
  e->shader = LoadShader(NULL, "shaders/mobius.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->point1Loc = GetShaderLocation(e->shader, "point1");
  e->point2Loc = GetShaderLocation(e->shader, "point2");
  e->spiralTightnessLoc = GetShaderLocation(e->shader, "spiralTightness");
  e->zoomFactorLoc = GetShaderLocation(e->shader, "zoomFactor");

  e->time = 0.0f;
  e->currentPoint1[0] = 0.0f;
  e->currentPoint1[1] = 0.0f;
  e->currentPoint2[0] = 0.0f;
  e->currentPoint2[1] = 0.0f;

  return true;
}

void MobiusEffectSetup(MobiusEffect *e, MobiusConfig *cfg, float deltaTime) {
  e->time += cfg->speed * deltaTime;

  // Compute point1 via Lissajous
  float offset1X, offset1Y;
  DualLissajousUpdate(&cfg->point1Lissajous, deltaTime, 0.0f, &offset1X,
                      &offset1Y);
  e->currentPoint1[0] = cfg->point1X + offset1X;
  e->currentPoint1[1] = cfg->point1Y + offset1Y;

  // Compute point2 via Lissajous
  float offset2X, offset2Y;
  DualLissajousUpdate(&cfg->point2Lissajous, deltaTime, 0.0f, &offset2X,
                      &offset2Y);
  e->currentPoint2[0] = cfg->point2X + offset2X;
  e->currentPoint2[1] = cfg->point2Y + offset2Y;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->point1Loc, e->currentPoint1,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->point2Loc, e->currentPoint2,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->spiralTightnessLoc, &cfg->spiralTightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomFactorLoc, &cfg->zoomFactor,
                 SHADER_UNIFORM_FLOAT);
}

void MobiusEffectUninit(MobiusEffect *e) { UnloadShader(e->shader); }

MobiusConfig MobiusConfigDefault(void) { return MobiusConfig{}; }
