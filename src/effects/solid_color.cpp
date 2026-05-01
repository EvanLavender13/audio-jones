// Solid color effect module implementation
// Fills screen with a configurable color (solid/rainbow/gradient) for blending

#include "solid_color.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"

bool SolidColorEffectInit(SolidColorEffect *e, const SolidColorConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/solid_color.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->colorLUTLoc = GetShaderLocation(e->shader, "colorLUT");

  e->colorLUT = ColorLUTInit(&cfg->gradient);
  if (e->colorLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  return true;
}

void SolidColorEffectSetup(SolidColorEffect *e, const SolidColorConfig *cfg) {
  ColorLUTUpdate(e->colorLUT, &cfg->gradient);
  SetShaderValueTexture(e->shader, e->colorLUTLoc,
                        ColorLUTGetTexture(e->colorLUT));
}

void SolidColorEffectUninit(SolidColorEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
}

void SolidColorRegisterParams(SolidColorConfig *cfg) {
  ModEngineRegisterParam("solidColor.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

SolidColorEffect *GetSolidColorEffect(PostEffect *pe) {
  return (SolidColorEffect *)pe->effectStates[TRANSFORM_SOLID_COLOR];
}

void SetupSolidColor(PostEffect *pe) {
  SolidColorEffectSetup(GetSolidColorEffect(pe), &pe->effects.solidColor);
}

void SetupSolidColorBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.solidColor.blendIntensity,
                       pe->effects.solidColor.blendMode);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(solidColor)
REGISTER_GENERATOR(TRANSFORM_SOLID_COLOR, SolidColor, solidColor, "Solid Color",
                   SetupSolidColorBlend, SetupSolidColor, 12, NULL, DrawOutput_solidColor)
// clang-format on
