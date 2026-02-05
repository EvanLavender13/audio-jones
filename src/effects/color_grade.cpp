// Color grade effect module implementation

#include "color_grade.h"
#include <stddef.h>

bool ColorGradeEffectInit(ColorGradeEffect *e) {
  e->shader = LoadShader(NULL, "shaders/color_grade.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->hueShiftLoc = GetShaderLocation(e->shader, "hueShift");
  e->saturationLoc = GetShaderLocation(e->shader, "saturation");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->contrastLoc = GetShaderLocation(e->shader, "contrast");
  e->temperatureLoc = GetShaderLocation(e->shader, "temperature");
  e->shadowsOffsetLoc = GetShaderLocation(e->shader, "shadowsOffset");
  e->midtonesOffsetLoc = GetShaderLocation(e->shader, "midtonesOffset");
  e->highlightsOffsetLoc = GetShaderLocation(e->shader, "highlightsOffset");

  return true;
}

void ColorGradeEffectSetup(ColorGradeEffect *e, const ColorGradeConfig *cfg) {
  SetShaderValue(e->shader, e->hueShiftLoc, &cfg->hueShift,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->saturationLoc, &cfg->saturation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->contrastLoc, &cfg->contrast,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->temperatureLoc, &cfg->temperature,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shadowsOffsetLoc, &cfg->shadowsOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->midtonesOffsetLoc, &cfg->midtonesOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->highlightsOffsetLoc, &cfg->highlightsOffset,
                 SHADER_UNIFORM_FLOAT);
}

void ColorGradeEffectUninit(ColorGradeEffect *e) { UnloadShader(e->shader); }

ColorGradeConfig ColorGradeConfigDefault(void) { return ColorGradeConfig{}; }
