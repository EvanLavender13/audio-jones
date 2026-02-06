// Solid color effect module implementation
// Fills screen with a configurable color (solid/rainbow/gradient) for blending

#include "solid_color.h"
#include "automation/modulation_engine.h"
#include "render/color_lut.h"

bool SolidColorEffectInit(SolidColorEffect *e, const SolidColorConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/solid_color.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->colorLUTLoc = GetShaderLocation(e->shader, "colorLUT");

  e->colorLUT = ColorLUTInit(&cfg->color);
  if (e->colorLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  return true;
}

void SolidColorEffectSetup(SolidColorEffect *e, const SolidColorConfig *cfg) {
  ColorLUTUpdate(e->colorLUT, &cfg->color);
  SetShaderValueTexture(e->shader, e->colorLUTLoc,
                        ColorLUTGetTexture(e->colorLUT));
}

void SolidColorEffectUninit(SolidColorEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
}

SolidColorConfig SolidColorConfigDefault(void) { return SolidColorConfig{}; }

void SolidColorRegisterParams(SolidColorConfig *cfg) {
  ModEngineRegisterParam("solidColor.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}
