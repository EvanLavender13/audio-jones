// Drekker Paint painterly effect module implementation

#include "drekker_paint.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool DrekkerPaintEffectInit(DrekkerPaintEffect *e) {
  e->shader = LoadShader(NULL, "shaders/drekker_paint.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->xDivLoc = GetShaderLocation(e->shader, "xDiv");
  e->yDivLoc = GetShaderLocation(e->shader, "yDiv");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->gapSizeLoc = GetShaderLocation(e->shader, "gapSize");
  e->diagSlantLoc = GetShaderLocation(e->shader, "diagSlant");
  e->strokeSpreadLoc = GetShaderLocation(e->shader, "strokeSpread");

  return true;
}

void DrekkerPaintEffectSetup(const DrekkerPaintEffect *e,
                             const DrekkerPaintConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->xDivLoc, &cfg->xDiv, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->yDivLoc, &cfg->yDiv, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gapSizeLoc, &cfg->gapSize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->diagSlantLoc, &cfg->diagSlant,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strokeSpreadLoc, &cfg->strokeSpread,
                 SHADER_UNIFORM_FLOAT);
}

void DrekkerPaintEffectUninit(const DrekkerPaintEffect *e) {
  UnloadShader(e->shader);
}

void DrekkerPaintRegisterParams(DrekkerPaintConfig *cfg) {
  ModEngineRegisterParam("drekkerPaint.xDiv", &cfg->xDiv, 2.0f, 30.0f);
  ModEngineRegisterParam("drekkerPaint.yDiv", &cfg->yDiv, 2.0f, 20.0f);
  ModEngineRegisterParam("drekkerPaint.curve", &cfg->curve, 0.001f, 0.1f);
  ModEngineRegisterParam("drekkerPaint.gapSize", &cfg->gapSize, 0.01f, 0.4f);
  ModEngineRegisterParam("drekkerPaint.diagSlant", &cfg->diagSlant, 0.0f, 0.3f);
  ModEngineRegisterParam("drekkerPaint.strokeSpread", &cfg->strokeSpread, 50.0f,
                         500.0f);
}

DrekkerPaintEffect *GetDrekkerPaintEffect(PostEffect *pe) {
  return (DrekkerPaintEffect *)pe->effectStates[TRANSFORM_DREKKER_PAINT];
}

void SetupDrekkerPaint(PostEffect *pe) {
  DrekkerPaintEffectSetup(GetDrekkerPaintEffect(pe), &pe->effects.drekkerPaint);
}

// === UI ===

static void DrawDrekkerPaintParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  DrekkerPaintConfig *cfg = &e->drekkerPaint;

  ImGui::SeparatorText("Geometry");
  ModulatableSlider("X Div##drekkerPaint", &cfg->xDiv, "drekkerPaint.xDiv",
                    "%.0f", ms);
  ModulatableSlider("Y Div##drekkerPaint", &cfg->yDiv, "drekkerPaint.yDiv",
                    "%.0f", ms);
  ModulatableSlider("Diag Slant##drekkerPaint", &cfg->diagSlant,
                    "drekkerPaint.diagSlant", "%.2f", ms);

  ImGui::SeparatorText("Strokes");
  ModulatableSlider("Curve##drekkerPaint", &cfg->curve, "drekkerPaint.curve",
                    "%.3f", ms);
  ModulatableSlider("Gap Size##drekkerPaint", &cfg->gapSize,
                    "drekkerPaint.gapSize", "%.2f", ms);
  ModulatableSlider("Stroke Spread##drekkerPaint", &cfg->strokeSpread,
                    "drekkerPaint.strokeSpread", "%.0f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_DREKKER_PAINT, DrekkerPaint, drekkerPaint,
                "Drekker Paint", "ART", 4, EFFECT_FLAG_NONE,
                SetupDrekkerPaint, NULL, DrawDrekkerPaintParams)
// clang-format on
