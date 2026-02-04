#include "poincare_disk.h"

#include <math.h>
#include <stddef.h>

bool PoincareDiskEffectInit(PoincareDiskEffect *e) {
  e->shader = LoadShader(NULL, "shaders/poincare_disk.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->tilePLoc = GetShaderLocation(e->shader, "tileP");
  e->tileQLoc = GetShaderLocation(e->shader, "tileQ");
  e->tileRLoc = GetShaderLocation(e->shader, "tileR");
  e->translationLoc = GetShaderLocation(e->shader, "translation");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->diskScaleLoc = GetShaderLocation(e->shader, "diskScale");

  e->time = 0.0f;
  e->rotation = 0.0f;
  e->currentTranslation[0] = 0.0f;
  e->currentTranslation[1] = 0.0f;

  return true;
}

void PoincareDiskEffectSetup(PoincareDiskEffect *e,
                             const PoincareDiskConfig *cfg, float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->time += cfg->translationSpeed * deltaTime;

  // Compute circular translation motion
  e->currentTranslation[0] =
      cfg->translationX + cfg->translationAmplitude * sinf(e->time);
  e->currentTranslation[1] =
      cfg->translationY + cfg->translationAmplitude * cosf(e->time);

  SetShaderValue(e->shader, e->tilePLoc, &cfg->tileP, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->tileQLoc, &cfg->tileQ, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->tileRLoc, &cfg->tileR, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->translationLoc, e->currentTranslation,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->diskScaleLoc, &cfg->diskScale,
                 SHADER_UNIFORM_FLOAT);
}

void PoincareDiskEffectUninit(PoincareDiskEffect *e) {
  UnloadShader(e->shader);
}

PoincareDiskConfig PoincareDiskConfigDefault(void) {
  PoincareDiskConfig cfg;
  cfg.enabled = false;
  cfg.tileP = 4;
  cfg.tileQ = 4;
  cfg.tileR = 4;
  cfg.translationX = 0.0f;
  cfg.translationY = 0.0f;
  cfg.translationSpeed = 0.0f;
  cfg.translationAmplitude = 0.0f;
  cfg.diskScale = 1.0f;
  cfg.rotationSpeed = 0.0f;
  return cfg;
}
