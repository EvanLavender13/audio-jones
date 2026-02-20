// Plaid effect module implementation
// Tartan fabric pattern with twill weave texture driven by FFT semitone energy

#include "plaid.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool PlaidEffectInit(PlaidEffect *e, const PlaidConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/plaid.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->bandCountLoc = GetShaderLocation(e->shader, "bandCount");
  e->accentWidthLoc = GetShaderLocation(e->shader, "accentWidth");
  e->threadDetailLoc = GetShaderLocation(e->shader, "threadDetail");
  e->twillRepeatLoc = GetShaderLocation(e->shader, "twillRepeat");
  e->morphAmountLoc = GetShaderLocation(e->shader, "morphAmount");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

void PlaidEffectSetup(PlaidEffect *e, const PlaidConfig *cfg, float deltaTime,
                      Texture2D fftTexture) {
  e->time += cfg->morphSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bandCountLoc, &cfg->bandCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->accentWidthLoc, &cfg->accentWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->threadDetailLoc, &cfg->threadDetail,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twillRepeatLoc, &cfg->twillRepeat,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->morphAmountLoc, &cfg->morphAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void PlaidEffectUninit(PlaidEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

PlaidConfig PlaidConfigDefault(void) { return PlaidConfig{}; }

void PlaidRegisterParams(PlaidConfig *cfg) {
  ModEngineRegisterParam("plaid.scale", &cfg->scale, 0.5f, 8.0f);
  ModEngineRegisterParam("plaid.accentWidth", &cfg->accentWidth, 0.05f, 0.5f);
  ModEngineRegisterParam("plaid.threadDetail", &cfg->threadDetail, 16.0f,
                         512.0f);
  ModEngineRegisterParam("plaid.morphSpeed", &cfg->morphSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("plaid.morphAmount", &cfg->morphAmount, 0.0f, 1.0f);
  ModEngineRegisterParam("plaid.glowIntensity", &cfg->glowIntensity, 0.0f,
                         2.0f);
  ModEngineRegisterParam("plaid.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("plaid.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("plaid.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("plaid.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("plaid.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("plaid.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupPlaid(PostEffect *pe) {
  PlaidEffectSetup(&pe->plaid, &pe->effects.plaid, pe->currentDeltaTime,
                   pe->fftTexture);
}

void SetupPlaidBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.plaid.blendIntensity,
                       pe->effects.plaid.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_PLAID_BLEND, Plaid, plaid,
                   "Plaid Blend", SetupPlaidBlend,
                   SetupPlaid)
// clang-format on
