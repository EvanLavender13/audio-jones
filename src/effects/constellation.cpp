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
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");
  e->waveAmpLoc = GetShaderLocation(e->shader, "waveAmp");
  e->pointSizeLoc = GetShaderLocation(e->shader, "pointSize");
  e->pointBrightnessLoc = GetShaderLocation(e->shader, "pointBrightness");
  e->lineThicknessLoc = GetShaderLocation(e->shader, "lineThickness");
  e->maxLineLenLoc = GetShaderLocation(e->shader, "maxLineLen");
  e->lineOpacityLoc = GetShaderLocation(e->shader, "lineOpacity");
  e->interpolateLineColorLoc =
      GetShaderLocation(e->shader, "interpolateLineColor");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->wavePhaseLoc = GetShaderLocation(e->shader, "wavePhase");
  e->pointLUTLoc = GetShaderLocation(e->shader, "pointLUT");
  e->lineLUTLoc = GetShaderLocation(e->shader, "lineLUT");
  e->fillEnabledLoc = GetShaderLocation(e->shader, "fillEnabled");
  e->fillOpacityLoc = GetShaderLocation(e->shader, "fillOpacity");
  e->fillThresholdLoc = GetShaderLocation(e->shader, "fillThreshold");
  e->waveCenterLoc = GetShaderLocation(e->shader, "waveCenter");
  e->waveInfluenceLoc = GetShaderLocation(e->shader, "waveInfluence");
  e->pointOpacityLoc = GetShaderLocation(e->shader, "pointOpacity");
  e->depthLayersLoc = GetShaderLocation(e->shader, "depthLayers");

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
  e->wavePhase = 0.0f;

  return true;
}

void ConstellationEffectSetup(ConstellationEffect *e,
                              const ConstellationConfig *cfg, float deltaTime) {
  e->animPhase += cfg->animSpeed * deltaTime;
  e->animPhase = fmodf(e->animPhase, 6.2831853f);
  e->wavePhase += cfg->waveSpeed * deltaTime;
  e->wavePhase = fmodf(e->wavePhase, 6.2831853f);

  ColorLUTUpdate(e->pointLUT, &cfg->pointGradient);
  ColorLUTUpdate(e->lineLUT, &cfg->lineGradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->waveInfluenceLoc, &cfg->waveInfluence,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pointOpacityLoc, &cfg->pointOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->depthLayersLoc, &cfg->depthLayers,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->animPhaseLoc, &e->animPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wavePhaseLoc, &e->wavePhase,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->gridScaleLoc, &cfg->gridScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wanderAmpLoc, &cfg->wanderAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveAmpLoc, &cfg->waveAmp, SHADER_UNIFORM_FLOAT);
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

  int fillEn = cfg->fillEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->fillEnabledLoc, &fillEn, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->fillOpacityLoc, &cfg->fillOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fillThresholdLoc, &cfg->fillThreshold,
                 SHADER_UNIFORM_FLOAT);

  float waveCenter[2] = {
      (cfg->waveCenterX - 0.5f) * cfg->gridScale *
          ((float)GetScreenWidth() / (float)GetScreenHeight()),
      (cfg->waveCenterY - 0.5f) * cfg->gridScale};
  SetShaderValue(e->shader, e->waveCenterLoc, waveCenter, SHADER_UNIFORM_VEC2);

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
  ModEngineRegisterParam("constellation.waveAmp", &cfg->waveAmp, 0.0f, 4.0f);
  ModEngineRegisterParam("constellation.waveSpeed", &cfg->waveSpeed, 0.0f,
                         5.0f);
  ModEngineRegisterParam("constellation.fillOpacity", &cfg->fillOpacity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("constellation.wanderAmp", &cfg->wanderAmp, 0.0f,
                         0.5f);
  ModEngineRegisterParam("constellation.pointOpacity", &cfg->pointOpacity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("constellation.waveInfluence", &cfg->waveInfluence,
                         0.0f, 1.0f);
  ModEngineRegisterParam("constellation.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}
