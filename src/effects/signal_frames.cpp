// Signal frames effect module implementation
// FFT-driven concentric rounded-rectangle outlines with per-octave sizing,
// orbital motion, sweep glow, and gradient coloring

#include "signal_frames.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool SignalFramesEffectInit(SignalFramesEffect *e,
                            const SignalFramesConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/signal_frames.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->rotationBiasLoc = GetShaderLocation(e->shader, "rotationBias");
  e->orbitRadiusLoc = GetShaderLocation(e->shader, "orbitRadius");
  e->orbitBiasLoc = GetShaderLocation(e->shader, "orbitBias");
  e->orbitAccumLoc = GetShaderLocation(e->shader, "orbitAccum");
  e->sizeMinLoc = GetShaderLocation(e->shader, "sizeMin");
  e->sizeMaxLoc = GetShaderLocation(e->shader, "sizeMax");
  e->aspectRatioLoc = GetShaderLocation(e->shader, "aspectRatio");
  e->outlineThicknessLoc = GetShaderLocation(e->shader, "outlineThickness");
  e->glowWidthLoc = GetShaderLocation(e->shader, "glowWidth");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->sweepAccumLoc = GetShaderLocation(e->shader, "sweepAccum");
  e->sweepIntensityLoc = GetShaderLocation(e->shader, "sweepIntensity");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->rotationAccum = 0.0f;
  e->sweepAccum = 0.0f;
  e->orbitAccum = 0.0f;

  return true;
}

void SignalFramesEffectSetup(SignalFramesEffect *e,
                             const SignalFramesConfig *cfg, float deltaTime,
                             Texture2D fftTexture) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;
  e->sweepAccum += cfg->sweepSpeed * deltaTime;
  e->orbitAccum += cfg->orbitSpeed * deltaTime;

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
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationBiasLoc, &cfg->rotationBias,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitRadiusLoc, &cfg->orbitRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitBiasLoc, &cfg->orbitBias,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitAccumLoc, &e->orbitAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sizeMinLoc, &cfg->sizeMin, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sizeMaxLoc, &cfg->sizeMax, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->aspectRatioLoc, &cfg->aspectRatio,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->outlineThicknessLoc, &cfg->outlineThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowWidthLoc, &cfg->glowWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sweepAccumLoc, &e->sweepAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sweepIntensityLoc, &cfg->sweepIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void SignalFramesEffectUninit(SignalFramesEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

SignalFramesConfig SignalFramesConfigDefault(void) {
  return SignalFramesConfig{};
}

void SignalFramesRegisterParams(SignalFramesConfig *cfg) {
  ModEngineRegisterParam("signalFrames.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("signalFrames.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("signalFrames.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("signalFrames.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("signalFrames.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("signalFrames.rotationSpeed", &cfg->rotationSpeed,
                         -3.0f, 3.0f);
  ModEngineRegisterParam("signalFrames.orbitRadius", &cfg->orbitRadius, 0.0f,
                         1.5f);
  ModEngineRegisterParam("signalFrames.sizeMin", &cfg->sizeMin, 0.01f, 0.5f);
  ModEngineRegisterParam("signalFrames.sizeMax", &cfg->sizeMax, 0.1f, 1.5f);
  ModEngineRegisterParam("signalFrames.aspectRatio", &cfg->aspectRatio, 0.2f,
                         5.0f);
  ModEngineRegisterParam("signalFrames.outlineThickness",
                         &cfg->outlineThickness, 0.002f, 0.05f);
  ModEngineRegisterParam("signalFrames.glowWidth", &cfg->glowWidth, 0.001f,
                         0.05f);
  ModEngineRegisterParam("signalFrames.glowIntensity", &cfg->glowIntensity,
                         0.5f, 10.0f);
  ModEngineRegisterParam("signalFrames.sweepSpeed", &cfg->sweepSpeed, 0.0f,
                         3.0f);
  ModEngineRegisterParam("signalFrames.sweepIntensity", &cfg->sweepIntensity,
                         0.0f, 0.1f);
  ModEngineRegisterParam("signalFrames.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupSignalFrames(PostEffect *pe) {
  SignalFramesEffectSetup(&pe->signalFrames, &pe->effects.signalFrames,
                          pe->currentDeltaTime, pe->fftTexture);
}

void SetupSignalFramesBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.signalFrames.blendIntensity,
                       pe->effects.signalFrames.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_SIGNAL_FRAMES_BLEND, SignalFrames, signalFrames,
                   "Signal Frames Blend", SetupSignalFramesBlend,
                   SetupSignalFrames)
// clang-format on
