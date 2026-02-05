// Plasma effect module implementation
// Generates animated lightning bolts via FBM noise with glow and drift

#include "plasma.h"
#include "render/color_lut.h"
#include <stddef.h>

bool PlasmaEffectInit(PlasmaEffect *e, const PlasmaConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/plasma.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->boltCountLoc = GetShaderLocation(e->shader, "boltCount");
  e->layerCountLoc = GetShaderLocation(e->shader, "layerCount");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->falloffTypeLoc = GetShaderLocation(e->shader, "falloffType");
  e->driftAmountLoc = GetShaderLocation(e->shader, "driftAmount");
  e->displacementLoc = GetShaderLocation(e->shader, "displacement");
  e->glowRadiusLoc = GetShaderLocation(e->shader, "glowRadius");
  e->coreBrightnessLoc = GetShaderLocation(e->shader, "coreBrightness");
  e->flickerAmountLoc = GetShaderLocation(e->shader, "flickerAmount");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->driftPhaseLoc = GetShaderLocation(e->shader, "driftPhase");
  e->flickerTimeLoc = GetShaderLocation(e->shader, "flickerTime");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->animPhase = 0.0f;
  e->driftPhase = 0.0f;
  e->flickerTime = 0.0f;

  return true;
}

void PlasmaEffectSetup(PlasmaEffect *e, const PlasmaConfig *cfg,
                       float deltaTime) {
  e->animPhase += cfg->animSpeed * deltaTime;
  e->driftPhase += cfg->driftSpeed * deltaTime;
  e->flickerTime += deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->boltCountLoc, &cfg->boltCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->layerCountLoc, &cfg->layerCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->falloffTypeLoc, &cfg->falloffType,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->driftAmountLoc, &cfg->driftAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->displacementLoc, &cfg->displacement,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowRadiusLoc, &cfg->glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->coreBrightnessLoc, &cfg->coreBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flickerAmountLoc, &cfg->flickerAmount,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->animPhaseLoc, &e->animPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftPhaseLoc, &e->driftPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flickerTimeLoc, &e->flickerTime,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void PlasmaEffectUninit(PlasmaEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

PlasmaConfig PlasmaConfigDefault(void) { return PlasmaConfig{}; }
