// Palette quantization effect module implementation

#include "palette_quantization.h"
#include <stddef.h>

bool PaletteQuantizationEffectInit(PaletteQuantizationEffect *e) {
  e->shader = LoadShader(NULL, "shaders/palette_quantization.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->colorLevelsLoc = GetShaderLocation(e->shader, "colorLevels");
  e->ditherStrengthLoc = GetShaderLocation(e->shader, "ditherStrength");
  e->bayerSizeLoc = GetShaderLocation(e->shader, "bayerSize");

  return true;
}

void PaletteQuantizationEffectSetup(PaletteQuantizationEffect *e,
                                    const PaletteQuantizationConfig *cfg) {
  SetShaderValue(e->shader, e->colorLevelsLoc, &cfg->colorLevels,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ditherStrengthLoc, &cfg->ditherStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bayerSizeLoc, &cfg->bayerSize,
                 SHADER_UNIFORM_INT);
}

void PaletteQuantizationEffectUninit(PaletteQuantizationEffect *e) {
  UnloadShader(e->shader);
}

PaletteQuantizationConfig PaletteQuantizationConfigDefault(void) {
  return PaletteQuantizationConfig{};
}
