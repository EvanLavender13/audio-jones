// Color grade effect module implementation

#include "color_grade.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
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

void ColorGradeEffectSetup(const ColorGradeEffect *e,
                           const ColorGradeConfig *cfg) {
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

void ColorGradeEffectUninit(const ColorGradeEffect *e) {
  UnloadShader(e->shader);
}

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

// === UI ===

static void DrawColorGradeParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  (void)glow;
  ColorGradeConfig *cg = &e->colorGrade;

  ModulatableSlider("Hue Shift##colorgrade", &cg->hueShift,
                    "colorGrade.hueShift", "%.0f °", ms, 360.0f);
  ModulatableSlider("Saturation##colorgrade", &cg->saturation,
                    "colorGrade.saturation", "%.2f", ms);
  ModulatableSlider("Brightness##colorgrade", &cg->brightness,
                    "colorGrade.brightness", "%.2f F", ms);
  ModulatableSlider("Contrast##colorgrade", &cg->contrast,
                    "colorGrade.contrast", "%.2f", ms);
  ModulatableSlider("Temperature##colorgrade", &cg->temperature,
                    "colorGrade.temperature", "%.2f", ms);

  ImGui::SeparatorText("Lift/Gamma/Gain");
  ModulatableSlider("Shadows##colorgrade", &cg->shadowsOffset,
                    "colorGrade.shadowsOffset", "%.2f", ms);
  ModulatableSlider("Midtones##colorgrade", &cg->midtonesOffset,
                    "colorGrade.midtonesOffset", "%.2f", ms);
  ModulatableSlider("Highlights##colorgrade", &cg->highlightsOffset,
                    "colorGrade.highlightsOffset", "%.2f", ms);
}

ColorGradeEffect *GetColorGradeEffect(PostEffect *pe) {
  return (ColorGradeEffect *)pe->effectStates[TRANSFORM_COLOR_GRADE];
}

void SetupColorGrade(PostEffect *pe) {
  ColorGradeEffectSetup(GetColorGradeEffect(pe), &pe->effects.colorGrade);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_COLOR_GRADE, ColorGrade, colorGrade, "Color Grade",
                "COL", 8, EFFECT_FLAG_NONE, SetupColorGrade, NULL,
                DrawColorGradeParams)
// clang-format on
