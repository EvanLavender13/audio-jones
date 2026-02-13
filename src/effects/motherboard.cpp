// Motherboard effect module implementation
// Iterative fold-and-glow circuit trace pattern driven by FFT semitone energy

#include "motherboard.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "render/color_lut.h"
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
  e->numOctavesLoc = GetShaderLocation(e->shader, "numOctaves");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->rangeXLoc = GetShaderLocation(e->shader, "rangeX");
  e->rangeYLoc = GetShaderLocation(e->shader, "rangeY");
  e->sizeLoc = GetShaderLocation(e->shader, "size");
  e->fallOffLoc = GetShaderLocation(e->shader, "fallOff");
  e->rotAngleLoc = GetShaderLocation(e->shader, "rotAngle");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->accentIntensityLoc = GetShaderLocation(e->shader, "accentIntensity");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;
  e->rotationAccum = 0.0f;

  return true;
}

void MotherboardEffectSetup(MotherboardEffect *e, const MotherboardConfig *cfg,
                            float deltaTime, Texture2D fftTexture) {
  e->time += deltaTime;
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
  int numOctavesInt = (int)cfg->numOctaves;
  SetShaderValue(e->shader, e->numOctavesLoc, &numOctavesInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->rangeXLoc, &cfg->rangeX, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rangeYLoc, &cfg->rangeY, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sizeLoc, &cfg->size, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fallOffLoc, &cfg->fallOff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotAngleLoc, &cfg->rotAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->accentIntensityLoc, &cfg->accentIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
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
  ModEngineRegisterParam("motherboard.numOctaves", &cfg->numOctaves, 1.0f,
                         8.0f);
  ModEngineRegisterParam("motherboard.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("motherboard.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("motherboard.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("motherboard.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("motherboard.rangeX", &cfg->rangeX, 0.0f, 2.0f);
  ModEngineRegisterParam("motherboard.rangeY", &cfg->rangeY, 0.0f, 1.0f);
  ModEngineRegisterParam("motherboard.size", &cfg->size, 0.0f, 1.0f);
  ModEngineRegisterParam("motherboard.fallOff", &cfg->fallOff, 1.0f, 3.0f);
  ModEngineRegisterParam("motherboard.rotAngle", &cfg->rotAngle, 0.0f,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("motherboard.glowIntensity", &cfg->glowIntensity,
                         0.001f, 0.1f);
  ModEngineRegisterParam("motherboard.accentIntensity", &cfg->accentIntensity,
                         0.0f, 0.2f);
  ModEngineRegisterParam("motherboard.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("motherboard.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}
