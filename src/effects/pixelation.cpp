// Pixelation effect module implementation

#include "pixelation.h"
#include <stddef.h>

bool PixelationEffectInit(PixelationEffect *e) {
  e->shader = LoadShader(NULL, "shaders/pixelation.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->cellCountLoc = GetShaderLocation(e->shader, "cellCount");
  e->ditherScaleLoc = GetShaderLocation(e->shader, "ditherScale");
  e->posterizeLevelsLoc = GetShaderLocation(e->shader, "posterizeLevels");

  return true;
}

void PixelationEffectSetup(PixelationEffect *e, const PixelationConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->cellCountLoc, &cfg->cellCount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ditherScaleLoc, &cfg->ditherScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->posterizeLevelsLoc, &cfg->posterizeLevels,
                 SHADER_UNIFORM_INT);
}

void PixelationEffectUninit(PixelationEffect *e) { UnloadShader(e->shader); }

PixelationConfig PixelationConfigDefault(void) { return PixelationConfig{}; }
