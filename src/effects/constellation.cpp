// Constellation effect module implementation
// Procedural star field with wandering points and distance-based connection
// lines

#include "constellation.h"
#include "automation/modulation_engine.h"
#include "render/color_lut.h"
#include <stddef.h>

bool ConstellationEffectInit(ConstellationEffect *e,
                             const ConstellationConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/constellation.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->gridScaleLoc = GetShaderLocation(e->shader, "gridScale");
  e->wanderAmpLoc = GetShaderLocation(e->shader, "wanderAmp");
  e->radialFreqLoc = GetShaderLocation(e->shader, "radialFreq");
  e->radialAmpLoc = GetShaderLocation(e->shader, "radialAmp");
  e->pointSizeLoc = GetShaderLocation(e->shader, "pointSize");
  e->pointBrightnessLoc = GetShaderLocation(e->shader, "pointBrightness");
  e->lineThicknessLoc = GetShaderLocation(e->shader, "lineThickness");
  e->maxLineLenLoc = GetShaderLocation(e->shader, "maxLineLen");
  e->lineOpacityLoc = GetShaderLocation(e->shader, "lineOpacity");
  e->interpolateLineColorLoc =
      GetShaderLocation(e->shader, "interpolateLineColor");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->radialPhaseLoc = GetShaderLocation(e->shader, "radialPhase");
  e->pointLUTLoc = GetShaderLocation(e->shader, "pointLUT");
  e->lineLUTLoc = GetShaderLocation(e->shader, "lineLUT");

  e->pointLUT = ColorLUTInit(&cfg->pointGradient);
  if (e->pointLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->lineLUT = ColorLUTInit(&cfg->lineGradient);
  if (e->lineLUT == NULL) {
    ColorLUTUninit(e->pointLUT);
    UnloadShader(e->shader);
    return false;
  }

  e->animPhase = 0.0f;
  e->radialPhase = 0.0f;

  return true;
}

void ConstellationEffectSetup(ConstellationEffect *e,
                              const ConstellationConfig *cfg, float deltaTime) {
  e->animPhase += cfg->animSpeed * deltaTime;
  e->radialPhase += cfg->radialSpeed * deltaTime;

  ColorLUTUpdate(e->pointLUT, &cfg->pointGradient);
  ColorLUTUpdate(e->lineLUT, &cfg->lineGradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->animPhaseLoc, &e->animPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radialPhaseLoc, &e->radialPhase,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->gridScaleLoc, &cfg->gridScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wanderAmpLoc, &cfg->wanderAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radialFreqLoc, &cfg->radialFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radialAmpLoc, &cfg->radialAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pointSizeLoc, &cfg->pointSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pointBrightnessLoc, &cfg->pointBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineThicknessLoc, &cfg->lineThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxLineLenLoc, &cfg->maxLineLen,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineOpacityLoc, &cfg->lineOpacity,
                 SHADER_UNIFORM_FLOAT);

  int interp = cfg->interpolateLineColor ? 1 : 0;
  SetShaderValue(e->shader, e->interpolateLineColorLoc, &interp,
                 SHADER_UNIFORM_INT);

  SetShaderValueTexture(e->shader, e->pointLUTLoc,
                        ColorLUTGetTexture(e->pointLUT));
  SetShaderValueTexture(e->shader, e->lineLUTLoc,
                        ColorLUTGetTexture(e->lineLUT));
}

void ConstellationEffectUninit(ConstellationEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->pointLUT);
  ColorLUTUninit(e->lineLUT);
}

ConstellationConfig ConstellationConfigDefault(void) {
  return ConstellationConfig{};
}

void ConstellationRegisterParams(ConstellationConfig *cfg) {
  ModEngineRegisterParam("constellation.animSpeed", &cfg->animSpeed, 0.0f,
                         5.0f);
  ModEngineRegisterParam("constellation.gridScale", &cfg->gridScale, 5.0f,
                         50.0f);
  ModEngineRegisterParam("constellation.lineOpacity", &cfg->lineOpacity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("constellation.maxLineLen", &cfg->maxLineLen, 0.5f,
                         2.0f);
  ModEngineRegisterParam("constellation.pointBrightness", &cfg->pointBrightness,
                         0.0f, 2.0f);
  ModEngineRegisterParam("constellation.pointSize", &cfg->pointSize, 0.3f,
                         3.0f);
  ModEngineRegisterParam("constellation.radialAmp", &cfg->radialAmp, 0.0f,
                         4.0f);
  ModEngineRegisterParam("constellation.radialSpeed", &cfg->radialSpeed, 0.0f,
                         5.0f);
  ModEngineRegisterParam("constellation.wanderAmp", &cfg->wanderAmp, 0.0f,
                         0.5f);
  ModEngineRegisterParam("constellation.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}
