// Solid color effect module implementation
// Fills screen with a configurable color (solid/rainbow/gradient) for blending

#include "solid_color.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"

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

void SetupSolidColor(PostEffect *pe) {
  SolidColorEffectSetup(&pe->solidColor, &pe->effects.solidColor);
}

void SetupSolidColorBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.solidColor.blendIntensity,
                       pe->effects.solidColor.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_SOLID_COLOR, SolidColor, solidColor, "Solid Color",
                   SetupSolidColorBlend, SetupSolidColor)
// clang-format on
