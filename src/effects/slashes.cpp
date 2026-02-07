// Slashes effect module implementation
// FFT-driven diagonal bar field — semitone-mapped bars with envelope decay,
// random scatter, thickness variation, and gradient-colored additive glow

#include "slashes.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "render/color_lut.h"
#include <stddef.h>

bool SlashesEffectInit(SlashesEffect *e, const SlashesConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/slashes.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->numOctavesLoc = GetShaderLocation(e->shader, "numOctaves");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->tickAccumLoc = GetShaderLocation(e->shader, "tickAccum");
  e->envelopeSharpLoc = GetShaderLocation(e->shader, "envelopeSharp");
  e->maxBarLengthLoc = GetShaderLocation(e->shader, "maxBarLength");
  e->barThicknessLoc = GetShaderLocation(e->shader, "barThickness");
  e->thicknessVariationLoc = GetShaderLocation(e->shader, "thicknessVariation");
  e->scatterLoc = GetShaderLocation(e->shader, "scatter");
  e->glowSoftnessLoc = GetShaderLocation(e->shader, "glowSoftness");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->rotationDepthLoc = GetShaderLocation(e->shader, "rotationDepth");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->tickAccum = 0.0f;

  return true;
}

void SlashesEffectSetup(SlashesEffect *e, const SlashesConfig *cfg,
                        float deltaTime, Texture2D fftTexture) {
  e->tickAccum += cfg->tickRate * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->numOctavesLoc, &cfg->numOctaves,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tickAccumLoc, &e->tickAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->envelopeSharpLoc, &cfg->envelopeSharp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxBarLengthLoc, &cfg->maxBarLength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->barThicknessLoc, &cfg->barThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thicknessVariationLoc, &cfg->thicknessVariation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scatterLoc, &cfg->scatter, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowSoftnessLoc, &cfg->glowSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationDepthLoc, &cfg->rotationDepth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void SlashesEffectUninit(SlashesEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

SlashesConfig SlashesConfigDefault(void) { return SlashesConfig{}; }

void SlashesRegisterParams(SlashesConfig *cfg) {
  ModEngineRegisterParam("slashes.baseFreq", &cfg->baseFreq, 20.0f, 2000.0f);
  ModEngineRegisterParam("slashes.gain", &cfg->gain, 0.1f, 20.0f);
  ModEngineRegisterParam("slashes.curve", &cfg->curve, 0.1f, 5.0f);
  ModEngineRegisterParam("slashes.tickRate", &cfg->tickRate, 0.5f, 20.0f);
  ModEngineRegisterParam("slashes.envelopeSharp", &cfg->envelopeSharp, 1.0f,
                         10.0f);
  ModEngineRegisterParam("slashes.maxBarLength", &cfg->maxBarLength, 0.1f,
                         1.5f);
  ModEngineRegisterParam("slashes.barThickness", &cfg->barThickness, 0.001f,
                         0.05f);
  ModEngineRegisterParam("slashes.thicknessVariation", &cfg->thicknessVariation,
                         0.0f, 1.0f);
  ModEngineRegisterParam("slashes.scatter", &cfg->scatter, 0.0f, 1.0f);
  ModEngineRegisterParam("slashes.glowSoftness", &cfg->glowSoftness, 0.001f,
                         0.05f);
  ModEngineRegisterParam("slashes.baseBright", &cfg->baseBright, 0.0f, 0.5f);
  ModEngineRegisterParam("slashes.rotationDepth", &cfg->rotationDepth, 0.0f,
                         1.0f);
  ModEngineRegisterParam("slashes.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}
