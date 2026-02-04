#include "circuit_board.h"

#include <stddef.h>

bool CircuitBoardEffectInit(CircuitBoardEffect *e) {
  e->shader = LoadShader(NULL, "shaders/circuit_board.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->patternConstLoc = GetShaderLocation(e->shader, "patternConst");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->offsetLoc = GetShaderLocation(e->shader, "offset");
  e->scaleDecayLoc = GetShaderLocation(e->shader, "scaleDecay");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->scrollOffsetLoc = GetShaderLocation(e->shader, "scrollOffset");
  e->rotationAngleLoc = GetShaderLocation(e->shader, "rotationAngle");
  e->chromaticLoc = GetShaderLocation(e->shader, "chromatic");

  e->scrollOffset = 0.0f;

  return true;
}

void CircuitBoardEffectSetup(CircuitBoardEffect *e,
                             const CircuitBoardConfig *cfg, float deltaTime) {
  e->scrollOffset += cfg->scrollSpeed * deltaTime;

  float patternConst[2] = {cfg->patternX, cfg->patternY};
  SetShaderValue(e->shader, e->patternConstLoc, patternConst,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetLoc, &cfg->offset, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleDecayLoc, &cfg->scaleDecay,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollOffsetLoc, &e->scrollOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAngleLoc, &cfg->scrollAngle,
                 SHADER_UNIFORM_FLOAT);

  int chromatic = cfg->chromatic ? 1 : 0;
  SetShaderValue(e->shader, e->chromaticLoc, &chromatic, SHADER_UNIFORM_INT);
}

void CircuitBoardEffectUninit(CircuitBoardEffect *e) {
  UnloadShader(e->shader);
}

CircuitBoardConfig CircuitBoardConfigDefault(void) {
  CircuitBoardConfig cfg;
  cfg.enabled = false;
  cfg.patternX = 7.0f;
  cfg.patternY = 5.0f;
  cfg.iterations = 6;
  cfg.scale = 1.4f;
  cfg.offset = 0.16f;
  cfg.scaleDecay = 1.05f;
  cfg.strength = 0.5f;
  cfg.scrollSpeed = 0.0f;
  cfg.scrollAngle = 0.0f;
  cfg.chromatic = false;
  return cfg;
}
