// Iris rings effect module implementation
// Concentric ring arcs driven by FFT energy with per-ring differential
// rotation, arc gating capped at half circle, and perspective tilt

#include "iris_rings.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool IrisRingsEffectInit(IrisRingsEffect *e, const IrisRingsConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/iris_rings.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->ringScaleLoc = GetShaderLocation(e->shader, "ringScale");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->tiltLoc = GetShaderLocation(e->shader, "tilt");
  e->tiltAngleLoc = GetShaderLocation(e->shader, "tiltAngle");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->rotationAccum = 0.0f;

  return true;
}

void IrisRingsEffectSetup(IrisRingsEffect *e, const IrisRingsConfig *cfg,
                          float deltaTime, Texture2D fftTexture) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;

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
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->ringScaleLoc, &cfg->ringScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tiltLoc, &cfg->tilt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tiltAngleLoc, &cfg->tiltAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void IrisRingsEffectUninit(IrisRingsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

IrisRingsConfig IrisRingsConfigDefault(void) { return IrisRingsConfig{}; }

void IrisRingsRegisterParams(IrisRingsConfig *cfg) {
  ModEngineRegisterParam("irisRings.ringScale", &cfg->ringScale, 0.05f, 0.8f);
  ModEngineRegisterParam("irisRings.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("irisRings.tilt", &cfg->tilt, 0.0f, 3.0f);
  ModEngineRegisterParam("irisRings.tiltAngle", &cfg->tiltAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("irisRings.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("irisRings.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("irisRings.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("irisRings.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("irisRings.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("irisRings.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupIrisRings(PostEffect *pe) {
  IrisRingsEffectSetup(&pe->irisRings, &pe->effects.irisRings,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupIrisRingsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.irisRings.blendIntensity,
                       pe->effects.irisRings.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_IRIS_RINGS_BLEND, IrisRings, irisRings,
                   "Iris Rings Blend", SetupIrisRingsBlend,
                   SetupIrisRings)
// clang-format on
