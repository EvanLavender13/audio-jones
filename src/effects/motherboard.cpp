// Motherboard effect module implementation
// Iterative fold-and-glow circuit trace pattern driven by FFT semitone energy

#include "motherboard.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool MotherboardEffectInit(MotherboardEffect *e, const MotherboardConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/motherboard.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->zoomLoc = GetShaderLocation(e->shader, "zoom");
  e->clampLoLoc = GetShaderLocation(e->shader, "clampLo");
  e->clampHiLoc = GetShaderLocation(e->shader, "clampHi");
  e->foldConstantLoc = GetShaderLocation(e->shader, "foldConstant");
  e->rotAngleLoc = GetShaderLocation(e->shader, "rotAngle");
  e->panAccumLoc = GetShaderLocation(e->shader, "panAccum");
  e->flowAccumLoc = GetShaderLocation(e->shader, "flowAccum");
  e->flowIntensityLoc = GetShaderLocation(e->shader, "flowIntensity");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->accentIntensityLoc = GetShaderLocation(e->shader, "accentIntensity");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->panAccum = 0.0f;
  e->flowAccum = 0.0f;
  e->rotationAccum = 0.0f;

  return true;
}

void MotherboardEffectSetup(MotherboardEffect *e, const MotherboardConfig *cfg,
                            float deltaTime, Texture2D fftTexture) {
  e->panAccum += cfg->panSpeed * deltaTime;
  e->flowAccum += cfg->flowSpeed * deltaTime;
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
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->zoomLoc, &cfg->zoom, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->clampLoLoc, &cfg->clampLo, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->clampHiLoc, &cfg->clampHi, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->foldConstantLoc, &cfg->foldConstant,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotAngleLoc, &cfg->rotAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->panAccumLoc, &e->panAccum, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flowAccumLoc, &e->flowAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flowIntensityLoc, &cfg->flowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->accentIntensityLoc, &cfg->accentIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void MotherboardEffectUninit(MotherboardEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

MotherboardConfig MotherboardConfigDefault(void) { return MotherboardConfig{}; }

void MotherboardRegisterParams(MotherboardConfig *cfg) {
  ModEngineRegisterParam("motherboard.zoom", &cfg->zoom, 0.5f, 4.0f);
  ModEngineRegisterParam("motherboard.clampLo", &cfg->clampLo, 0.01f, 1.0f);
  ModEngineRegisterParam("motherboard.clampHi", &cfg->clampHi, 0.5f, 5.0f);
  ModEngineRegisterParam("motherboard.foldConstant", &cfg->foldConstant, 0.5f,
                         2.0f);
  ModEngineRegisterParam("motherboard.rotAngle", &cfg->rotAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("motherboard.panSpeed", &cfg->panSpeed, -2.0f, 2.0f);
  ModEngineRegisterParam("motherboard.flowSpeed", &cfg->flowSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("motherboard.flowIntensity", &cfg->flowIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("motherboard.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("motherboard.glowIntensity", &cfg->glowIntensity,
                         0.001f, 0.1f);
  ModEngineRegisterParam("motherboard.accentIntensity", &cfg->accentIntensity,
                         0.0f, 0.1f);
  ModEngineRegisterParam("motherboard.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("motherboard.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("motherboard.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("motherboard.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("motherboard.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("motherboard.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupMotherboard(PostEffect *pe) {
  MotherboardEffectSetup(&pe->motherboard, &pe->effects.motherboard,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupMotherboardBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.motherboard.blendIntensity,
                       pe->effects.motherboard.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_MOTHERBOARD_BLEND, Motherboard, motherboard,
                   "Motherboard Blend", SetupMotherboardBlend,
                   SetupMotherboard)
// clang-format on
