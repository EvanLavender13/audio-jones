// Synthwave effect module implementation

#include "synthwave.h"
#include <stddef.h>

bool SynthwaveEffectInit(SynthwaveEffect *e) {
  e->shader = LoadShader(NULL, "shaders/synthwave.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->horizonYLoc = GetShaderLocation(e->shader, "horizonY");
  e->colorMixLoc = GetShaderLocation(e->shader, "colorMix");
  e->palettePhaseLoc = GetShaderLocation(e->shader, "palettePhase");
  e->gridSpacingLoc = GetShaderLocation(e->shader, "gridSpacing");
  e->gridThicknessLoc = GetShaderLocation(e->shader, "gridThickness");
  e->gridOpacityLoc = GetShaderLocation(e->shader, "gridOpacity");
  e->gridGlowLoc = GetShaderLocation(e->shader, "gridGlow");
  e->gridColorLoc = GetShaderLocation(e->shader, "gridColor");
  e->stripeCountLoc = GetShaderLocation(e->shader, "stripeCount");
  e->stripeSoftnessLoc = GetShaderLocation(e->shader, "stripeSoftness");
  e->stripeIntensityLoc = GetShaderLocation(e->shader, "stripeIntensity");
  e->sunColorLoc = GetShaderLocation(e->shader, "sunColor");
  e->horizonIntensityLoc = GetShaderLocation(e->shader, "horizonIntensity");
  e->horizonFalloffLoc = GetShaderLocation(e->shader, "horizonFalloff");
  e->horizonColorLoc = GetShaderLocation(e->shader, "horizonColor");
  e->gridTimeLoc = GetShaderLocation(e->shader, "gridTime");
  e->stripeTimeLoc = GetShaderLocation(e->shader, "stripeTime");

  e->gridTime = 0.0f;
  e->stripeTime = 0.0f;

  return true;
}

void SynthwaveEffectSetup(SynthwaveEffect *e, const SynthwaveConfig *cfg,
                          float deltaTime) {
  e->gridTime += cfg->gridScrollSpeed * deltaTime;
  e->stripeTime += cfg->stripeScrollSpeed * deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->horizonYLoc, &cfg->horizonY,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorMixLoc, &cfg->colorMix,
                 SHADER_UNIFORM_FLOAT);

  float palettePhase[3] = {cfg->palettePhaseR, cfg->palettePhaseG,
                           cfg->palettePhaseB};
  SetShaderValue(e->shader, e->palettePhaseLoc, palettePhase,
                 SHADER_UNIFORM_VEC3);

  SetShaderValue(e->shader, e->gridSpacingLoc, &cfg->gridSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gridThicknessLoc, &cfg->gridThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gridOpacityLoc, &cfg->gridOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gridGlowLoc, &cfg->gridGlow,
                 SHADER_UNIFORM_FLOAT);

  float gridColor[3] = {cfg->gridR, cfg->gridG, cfg->gridB};
  SetShaderValue(e->shader, e->gridColorLoc, gridColor, SHADER_UNIFORM_VEC3);

  SetShaderValue(e->shader, e->stripeCountLoc, &cfg->stripeCount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stripeSoftnessLoc, &cfg->stripeSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stripeIntensityLoc, &cfg->stripeIntensity,
                 SHADER_UNIFORM_FLOAT);

  float sunColor[3] = {cfg->sunR, cfg->sunG, cfg->sunB};
  SetShaderValue(e->shader, e->sunColorLoc, sunColor, SHADER_UNIFORM_VEC3);

  SetShaderValue(e->shader, e->horizonIntensityLoc, &cfg->horizonIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->horizonFalloffLoc, &cfg->horizonFalloff,
                 SHADER_UNIFORM_FLOAT);

  float horizonColor[3] = {cfg->horizonR, cfg->horizonG, cfg->horizonB};
  SetShaderValue(e->shader, e->horizonColorLoc, horizonColor,
                 SHADER_UNIFORM_VEC3);

  SetShaderValue(e->shader, e->gridTimeLoc, &e->gridTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stripeTimeLoc, &e->stripeTime,
                 SHADER_UNIFORM_FLOAT);
}

void SynthwaveEffectUninit(SynthwaveEffect *e) { UnloadShader(e->shader); }

SynthwaveConfig SynthwaveConfigDefault(void) { return SynthwaveConfig{}; }
