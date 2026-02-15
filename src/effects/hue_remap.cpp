// Hue remap effect module implementation

#include "hue_remap.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool HueRemapEffectInit(HueRemapEffect *e, const HueRemapConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/hue_remap.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->shiftLoc = GetShaderLocation(e->shader, "shift");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->radialLoc = GetShaderLocation(e->shader, "radial");
  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "texture1");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  return true;
}

void HueRemapEffectSetup(HueRemapEffect *e, const HueRemapConfig *cfg) {
  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  SetShaderValue(e->shader, e->shiftLoc, &cfg->shift, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radialLoc, &cfg->radial, SHADER_UNIFORM_FLOAT);

  float center[2] = {cfg->cx, cfg->cy};
  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void HueRemapEffectUninit(HueRemapEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

HueRemapConfig HueRemapConfigDefault(void) { return HueRemapConfig{}; }

void HueRemapRegisterParams(HueRemapConfig *cfg) {
  ModEngineRegisterParam("hueRemap.shift", &cfg->shift, 0.0f, 1.0f);
  ModEngineRegisterParam("hueRemap.intensity", &cfg->intensity, 0.0f, 1.0f);
  ModEngineRegisterParam("hueRemap.radial", &cfg->radial, -1.0f, 1.0f);
  ModEngineRegisterParam("hueRemap.cx", &cfg->cx, 0.0f, 1.0f);
  ModEngineRegisterParam("hueRemap.cy", &cfg->cy, 0.0f, 1.0f);
}

void SetupHueRemap(PostEffect *pe) {
  HueRemapEffectSetup(&pe->hueRemap, &pe->effects.hueRemap);
}

// clang-format off
REGISTER_EFFECT_CFG(TRANSFORM_HUE_REMAP, HueRemap, hueRemap, "Hue Remap",
                    "COL", 8, EFFECT_FLAG_NONE, SetupHueRemap, NULL)
// clang-format on
