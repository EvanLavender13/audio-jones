// Muons effect module implementation
// Raymarched turbulent ring trails through a volumetric noise field

#include "muons.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool MuonsEffectInit(MuonsEffect *e, const MuonsConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/muons.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->turbulenceOctavesLoc = GetShaderLocation(e->shader, "turbulenceOctaves");
  e->turbulenceStrengthLoc = GetShaderLocation(e->shader, "turbulenceStrength");
  e->ringThicknessLoc = GetShaderLocation(e->shader, "ringThickness");
  e->cameraDistanceLoc = GetShaderLocation(e->shader, "cameraDistance");
  e->colorFreqLoc = GetShaderLocation(e->shader, "colorFreq");
  e->colorSpeedLoc = GetShaderLocation(e->shader, "colorSpeed");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->exposureLoc = GetShaderLocation(e->shader, "exposure");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

void MuonsEffectSetup(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime) {
  e->time += deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->turbulenceOctavesLoc, &cfg->turbulenceOctaves,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->turbulenceStrengthLoc, &cfg->turbulenceStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ringThicknessLoc, &cfg->ringThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraDistanceLoc, &cfg->cameraDistance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorFreqLoc, &cfg->colorFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorSpeedLoc, &cfg->colorSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->exposureLoc, &cfg->exposure,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void MuonsEffectUninit(MuonsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

MuonsConfig MuonsConfigDefault(void) { return MuonsConfig{}; }

void MuonsRegisterParams(MuonsConfig *cfg) {
  ModEngineRegisterParam("muons.turbulenceStrength", &cfg->turbulenceStrength,
                         0.0f, 2.0f);
  ModEngineRegisterParam("muons.ringThickness", &cfg->ringThickness, 0.005f,
                         0.1f);
  ModEngineRegisterParam("muons.cameraDistance", &cfg->cameraDistance, 3.0f,
                         20.0f);
  ModEngineRegisterParam("muons.colorFreq", &cfg->colorFreq, 0.5f, 50.0f);
  ModEngineRegisterParam("muons.colorSpeed", &cfg->colorSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("muons.brightness", &cfg->brightness, 0.1f, 5.0f);
  ModEngineRegisterParam("muons.exposure", &cfg->exposure, 500.0f, 10000.0f);
  ModEngineRegisterParam("muons.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupMuons(PostEffect *pe) {
  MuonsEffectSetup(&pe->muons, &pe->effects.muons, pe->currentDeltaTime);
}

void SetupMuonsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.muons.blendIntensity,
                       pe->effects.muons.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_MUONS_BLEND, Muons, muons, "Muons Blend",
                   SetupMuonsBlend, SetupMuons)
// clang-format on
