#include "fft_radial_warp.h"

#include "automation/modulation_engine.h"
#include <math.h>
#include <stddef.h>

#define TWO_PI 6.28318530718f

bool FftRadialWarpEffectInit(FftRadialWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/fft_radial_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->freqStartLoc = GetShaderLocation(e->shader, "freqStart");
  e->freqEndLoc = GetShaderLocation(e->shader, "freqEnd");
  e->maxRadiusLoc = GetShaderLocation(e->shader, "maxRadius");
  e->freqCurveLoc = GetShaderLocation(e->shader, "freqCurve");
  e->bassBoostLoc = GetShaderLocation(e->shader, "bassBoost");
  e->segmentsLoc = GetShaderLocation(e->shader, "segments");
  e->pushPullBalanceLoc = GetShaderLocation(e->shader, "pushPullBalance");
  e->pushPullSmoothnessLoc = GetShaderLocation(e->shader, "pushPullSmoothness");
  e->phaseOffsetLoc = GetShaderLocation(e->shader, "phaseOffset");

  e->phaseAccum = 0.0f;

  return true;
}

void FftRadialWarpEffectSetup(FftRadialWarpEffect *e,
                              const FftRadialWarpConfig *cfg, float deltaTime,
                              int screenWidth, int screenHeight,
                              Texture2D fftTexture) {
  e->phaseAccum += cfg->phaseSpeed * deltaTime;
  float phaseOffset = fmodf(e->phaseAccum, TWO_PI);

  float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqStartLoc, &cfg->freqStart,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqEndLoc, &cfg->freqEnd, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxRadiusLoc, &cfg->maxRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqCurveLoc, &cfg->freqCurve,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bassBoostLoc, &cfg->bassBoost,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->segmentsLoc, &cfg->segments, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->pushPullBalanceLoc, &cfg->pushPullBalance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pushPullSmoothnessLoc, &cfg->pushPullSmoothness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseOffsetLoc, &phaseOffset,
                 SHADER_UNIFORM_FLOAT);
}

void FftRadialWarpEffectUninit(FftRadialWarpEffect *e) {
  UnloadShader(e->shader);
}

FftRadialWarpConfig FftRadialWarpConfigDefault(void) {
  return FftRadialWarpConfig{};
}

void FftRadialWarpRegisterParams(FftRadialWarpConfig *cfg) {
  ModEngineRegisterParam("fftRadialWarp.intensity", &cfg->intensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("fftRadialWarp.freqStart", &cfg->freqStart, 0.0f,
                         1.0f);
  ModEngineRegisterParam("fftRadialWarp.freqEnd", &cfg->freqEnd, 0.0f, 1.0f);
  ModEngineRegisterParam("fftRadialWarp.maxRadius", &cfg->maxRadius, 0.1f,
                         1.0f);
  ModEngineRegisterParam("fftRadialWarp.freqCurve", &cfg->freqCurve, 0.5f,
                         3.0f);
  ModEngineRegisterParam("fftRadialWarp.bassBoost", &cfg->bassBoost, 0.0f,
                         2.0f);
  ModEngineRegisterParam("fftRadialWarp.pushPullBalance", &cfg->pushPullBalance,
                         0.0f, 1.0f);
  ModEngineRegisterParam("fftRadialWarp.pushPullSmoothness",
                         &cfg->pushPullSmoothness, 0.0f, 1.0f);
  ModEngineRegisterParam("fftRadialWarp.phaseSpeed", &cfg->phaseSpeed,
                         -3.14159265f, 3.14159265f);
}
