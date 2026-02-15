#include "tone_warp.h"

#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <math.h>
#include <stddef.h>

bool ToneWarpEffectInit(ToneWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/tone_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->maxRadiusLoc = GetShaderLocation(e->shader, "maxRadius");
  e->segmentsLoc = GetShaderLocation(e->shader, "segments");
  e->pushPullBalanceLoc = GetShaderLocation(e->shader, "pushPullBalance");
  e->pushPullSmoothnessLoc = GetShaderLocation(e->shader, "pushPullSmoothness");
  e->phaseOffsetLoc = GetShaderLocation(e->shader, "phaseOffset");

  e->phaseAccum = 0.0f;

  return true;
}

void ToneWarpEffectSetup(ToneWarpEffect *e, const ToneWarpConfig *cfg,
                         float deltaTime, int screenWidth, int screenHeight,
                         Texture2D fftTexture) {
  e->phaseAccum += cfg->phaseSpeed * deltaTime;
  float phaseOffset = fmodf(e->phaseAccum, TWO_PI_F);

  float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);

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

  SetShaderValue(e->shader, e->maxRadiusLoc, &cfg->maxRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->segmentsLoc, &cfg->segments, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->pushPullBalanceLoc, &cfg->pushPullBalance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pushPullSmoothnessLoc, &cfg->pushPullSmoothness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseOffsetLoc, &phaseOffset,
                 SHADER_UNIFORM_FLOAT);
}

void ToneWarpEffectUninit(ToneWarpEffect *e) { UnloadShader(e->shader); }

ToneWarpConfig ToneWarpConfigDefault(void) { return ToneWarpConfig{}; }

void ToneWarpRegisterParams(ToneWarpConfig *cfg) {
  ModEngineRegisterParam("toneWarp.intensity", &cfg->intensity, 0.0f, 1.0f);
  ModEngineRegisterParam("toneWarp.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("toneWarp.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("toneWarp.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("toneWarp.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("toneWarp.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("toneWarp.maxRadius", &cfg->maxRadius, 0.1f, 1.0f);
  ModEngineRegisterParam("toneWarp.pushPullBalance", &cfg->pushPullBalance,
                         0.0f, 1.0f);
  ModEngineRegisterParam("toneWarp.pushPullSmoothness",
                         &cfg->pushPullSmoothness, 0.0f, 1.0f);
  ModEngineRegisterParam("toneWarp.phaseSpeed", &cfg->phaseSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
}

void SetupToneWarp(PostEffect *pe) {
  ToneWarpEffectSetup(&pe->toneWarp, &pe->effects.toneWarp,
                      pe->currentDeltaTime, pe->screenWidth, pe->screenHeight,
                      pe->fftTexture);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_TONE_WARP, ToneWarp, toneWarp, "Tone Warp", "WARP",
                1, EFFECT_FLAG_NONE, SetupToneWarp, NULL)
// clang-format on
