// False color effect module implementation

#include "false_color.h"
#include "render/color_lut.h"
#include <stddef.h>

bool FalseColorEffectInit(FalseColorEffect *e, const FalseColorConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/false_color.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "texture1");

  e->lut = ColorLUTInit(&cfg->gradient);

  return true;
}

void FalseColorEffectSetup(FalseColorEffect *e, const FalseColorConfig *cfg) {
  ColorLUTUpdate(e->lut, &cfg->gradient);

  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->lut));
}

void FalseColorEffectUninit(FalseColorEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->lut);
}

FalseColorConfig FalseColorConfigDefault(void) { return FalseColorConfig{}; }
