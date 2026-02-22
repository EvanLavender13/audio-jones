// False color effect module implementation

#include "false_color.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool FalseColorEffectInit(FalseColorEffect *e, const FalseColorConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/false_color.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "texture1");

  e->lut = ColorLUTInit(&cfg->gradient);
  if (e->lut == NULL) {
    UnloadShader(e->shader);
    return false;
  }

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

void FalseColorRegisterParams(FalseColorConfig *cfg) {
  ModEngineRegisterParam("falseColor.intensity", &cfg->intensity, 0.0f, 1.0f);
}

// === UI ===

static void DrawFalseColorParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  (void)glow;
  FalseColorConfig *fc = &e->falseColor;

  ImGuiDrawColorMode(&fc->gradient);

  ModulatableSlider("Intensity##falsecolor", &fc->intensity,
                    "falseColor.intensity", "%.2f", ms);
}

void SetupFalseColor(PostEffect *pe) {
  FalseColorEffectSetup(&pe->falseColor, &pe->effects.falseColor);
}

// clang-format off
REGISTER_EFFECT_CFG(TRANSFORM_FALSE_COLOR, FalseColor, falseColor,
                    "False Color", "COL", 8, EFFECT_FLAG_NONE,
                    SetupFalseColor, NULL, DrawFalseColorParams)
// clang-format on
