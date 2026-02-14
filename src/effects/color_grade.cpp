// Color grade effect module implementation

#include "color_grade.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

void ColorGradeRegisterParams(ColorGradeConfig *cfg) {
  ModEngineRegisterParam("colorGrade.hueShift", &cfg->hueShift, 0.0f, 1.0f);
  ModEngineRegisterParam("colorGrade.saturation", &cfg->saturation, 0.0f, 2.0f);
  ModEngineRegisterParam("colorGrade.brightness", &cfg->brightness, -2.0f,
                         2.0f);
  ModEngineRegisterParam("colorGrade.contrast", &cfg->contrast, 0.5f, 2.0f);
  ModEngineRegisterParam("colorGrade.temperature", &cfg->temperature, -1.0f,
                         1.0f);
  ModEngineRegisterParam("colorGrade.shadowsOffset", &cfg->shadowsOffset, -0.5f,
                         0.5f);
  ModEngineRegisterParam("colorGrade.midtonesOffset", &cfg->midtonesOffset,
                         -0.5f, 0.5f);
  ModEngineRegisterParam("colorGrade.highlightsOffset", &cfg->highlightsOffset,
                         -0.5f, 0.5f);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_COLOR_GRADE, ColorGrade, colorGrade, "Color Grade",
                "COL", 8, EFFECT_FLAG_NONE, SetupColorGrade, NULL)
// clang-format on
