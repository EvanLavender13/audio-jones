// Filaments effect module implementation
// Tangled radial line segments driven by FFT semitone energy — rotating
// endpoint geometry, per-segment FFT warp, triangle-wave noise, additive glow

#include "filaments.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool FilamentsEffectInit(FilamentsEffect *e, const FilamentsConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/filaments.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->filamentsLoc = GetShaderLocation(e->shader, "filaments");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->radiusLoc = GetShaderLocation(e->shader, "radius");
  e->spreadLoc = GetShaderLocation(e->shader, "spread");
  e->stepAngleLoc = GetShaderLocation(e->shader, "stepAngle");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->falloffExponentLoc = GetShaderLocation(e->shader, "falloffExponent");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->noiseStrengthLoc = GetShaderLocation(e->shader, "noiseStrength");
  e->noiseTimeLoc = GetShaderLocation(e->shader, "noiseTime");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->rotationAccum = 0.0f;
  e->noiseTime = 0.0f;

  return true;
}

void FilamentsEffectSetup(FilamentsEffect *e, const FilamentsConfig *cfg,
                          float deltaTime, Texture2D fftTexture) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;
  e->noiseTime += cfg->noiseSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->filamentsLoc, &cfg->filaments,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radiusLoc, &cfg->radius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spreadLoc, &cfg->spread, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stepAngleLoc, &cfg->stepAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->falloffExponentLoc, &cfg->falloffExponent,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseStrengthLoc, &cfg->noiseStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseTimeLoc, &e->noiseTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void FilamentsEffectUninit(FilamentsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

FilamentsConfig FilamentsConfigDefault(void) { return FilamentsConfig{}; }

void FilamentsRegisterParams(FilamentsConfig *cfg) {
  ModEngineRegisterParam("filaments.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("filaments.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("filaments.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("filaments.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("filaments.radius", &cfg->radius, 0.1f, 2.0f);
  ModEngineRegisterParam("filaments.spread", &cfg->spread, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("filaments.stepAngle", &cfg->stepAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("filaments.glowIntensity", &cfg->glowIntensity, 0.001f,
                         0.05f);
  ModEngineRegisterParam("filaments.falloffExponent", &cfg->falloffExponent,
                         0.8f, 2.0f);
  ModEngineRegisterParam("filaments.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("filaments.noiseStrength", &cfg->noiseStrength, 0.0f,
                         1.0f);
  ModEngineRegisterParam("filaments.noiseSpeed", &cfg->noiseSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("filaments.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("filaments.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupFilaments(PostEffect *pe) {
  FilamentsEffectSetup(&pe->filaments, &pe->effects.filaments,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupFilamentsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.filaments.blendIntensity,
                       pe->effects.filaments.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_FILAMENTS_BLEND, Filaments, filaments,
                   "Filaments Blend", SetupFilamentsBlend, SetupFilaments)
// clang-format on
