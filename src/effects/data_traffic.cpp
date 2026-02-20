// Data traffic effect module implementation
// Scrolling lane grid of colored cells with FFT-driven brightness and
// randomized widths

#include "data_traffic.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool DataTrafficEffectInit(DataTrafficEffect *e, const DataTrafficConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/data_traffic.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->lanesLoc = GetShaderLocation(e->shader, "lanes");
  e->cellWidthLoc = GetShaderLocation(e->shader, "cellWidth");
  e->spacingLoc = GetShaderLocation(e->shader, "spacing");
  e->gapSizeLoc = GetShaderLocation(e->shader, "gapSize");
  e->scrollAngleLoc = GetShaderLocation(e->shader, "scrollAngle");
  e->scrollSpeedLoc = GetShaderLocation(e->shader, "scrollSpeed");
  e->widthVariationLoc = GetShaderLocation(e->shader, "widthVariation");
  e->colorMixLoc = GetShaderLocation(e->shader, "colorMix");
  e->jitterLoc = GetShaderLocation(e->shader, "jitter");
  e->changeRateLoc = GetShaderLocation(e->shader, "changeRate");
  e->sparkIntensityLoc = GetShaderLocation(e->shader, "sparkIntensity");
  e->breathProbLoc = GetShaderLocation(e->shader, "breathProb");
  e->breathPhaseLoc = GetShaderLocation(e->shader, "breathPhase");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->glowRadiusLoc = GetShaderLocation(e->shader, "glowRadius");
  e->twitchProbLoc = GetShaderLocation(e->shader, "twitchProb");
  e->twitchIntensityLoc = GetShaderLocation(e->shader, "twitchIntensity");
  e->splitProbLoc = GetShaderLocation(e->shader, "splitProb");
  e->mergeProbLoc = GetShaderLocation(e->shader, "mergeProb");
  e->fissionProbLoc = GetShaderLocation(e->shader, "fissionProb");
  e->phaseShiftProbLoc = GetShaderLocation(e->shader, "phaseShiftProb");
  e->phaseShiftIntensityLoc =
      GetShaderLocation(e->shader, "phaseShiftIntensity");
  e->springProbLoc = GetShaderLocation(e->shader, "springProb");
  e->springIntensityLoc = GetShaderLocation(e->shader, "springIntensity");
  e->widthSpringProbLoc = GetShaderLocation(e->shader, "widthSpringProb");
  e->widthSpringIntensityLoc =
      GetShaderLocation(e->shader, "widthSpringIntensity");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;
  e->breathPhase = 0.0f;

  return true;
}

void DataTrafficEffectSetup(DataTrafficEffect *e, const DataTrafficConfig *cfg,
                            float deltaTime, Texture2D fftTexture) {
  e->time += deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lanesLoc, &cfg->lanes, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->cellWidthLoc, &cfg->cellWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spacingLoc, &cfg->spacing, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gapSizeLoc, &cfg->gapSize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollAngleLoc, &cfg->scrollAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollSpeedLoc, &cfg->scrollSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->widthVariationLoc, &cfg->widthVariation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorMixLoc, &cfg->colorMix,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->jitterLoc, &cfg->jitter, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->changeRateLoc, &cfg->changeRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sparkIntensityLoc, &cfg->sparkIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->breathProbLoc, &cfg->breathProb,
                 SHADER_UNIFORM_FLOAT);
  e->breathPhase += cfg->breathRate * deltaTime;
  if (e->breathPhase > 100.0f) {
    e->breathPhase -= 100.0f;
  }
  SetShaderValue(e->shader, e->breathPhaseLoc, &e->breathPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowRadiusLoc, &cfg->glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twitchProbLoc, &cfg->twitchProb,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twitchIntensityLoc, &cfg->twitchIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->splitProbLoc, &cfg->splitProb,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->mergeProbLoc, &cfg->mergeProb,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fissionProbLoc, &cfg->fissionProb,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseShiftProbLoc, &cfg->phaseShiftProb,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseShiftIntensityLoc,
                 &cfg->phaseShiftIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->springProbLoc, &cfg->springProb,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->springIntensityLoc, &cfg->springIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->widthSpringProbLoc, &cfg->widthSpringProb,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->widthSpringIntensityLoc,
                 &cfg->widthSpringIntensity, SHADER_UNIFORM_FLOAT);

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

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void DataTrafficEffectUninit(DataTrafficEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

DataTrafficConfig DataTrafficConfigDefault(void) { return DataTrafficConfig{}; }

void DataTrafficRegisterParams(DataTrafficConfig *cfg) {
  ModEngineRegisterParam("dataTraffic.cellWidth", &cfg->cellWidth, 0.01f, 0.3f);
  ModEngineRegisterParam("dataTraffic.spacing", &cfg->spacing, 1.5f, 6.0f);
  ModEngineRegisterParam("dataTraffic.gapSize", &cfg->gapSize, 0.02f, 0.3f);
  ModEngineRegisterParam("dataTraffic.scrollAngle", &cfg->scrollAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("dataTraffic.widthVariation", &cfg->widthVariation,
                         0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.colorMix", &cfg->colorMix, 0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.jitter", &cfg->jitter, 0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.changeRate", &cfg->changeRate, 0.05f,
                         0.5f);
  ModEngineRegisterParam("dataTraffic.sparkIntensity", &cfg->sparkIntensity,
                         0.0f, 2.0f);
  ModEngineRegisterParam("dataTraffic.breathProb", &cfg->breathProb, 0.0f,
                         1.0f);
  ModEngineRegisterParam("dataTraffic.breathRate", &cfg->breathRate, 0.05f,
                         2.0f);
  ModEngineRegisterParam("dataTraffic.glowIntensity", &cfg->glowIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("dataTraffic.glowRadius", &cfg->glowRadius, 0.5f,
                         5.0f);
  ModEngineRegisterParam("dataTraffic.twitchProb", &cfg->twitchProb, 0.0f,
                         1.0f);
  ModEngineRegisterParam("dataTraffic.twitchIntensity", &cfg->twitchIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.splitProb", &cfg->splitProb, 0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.mergeProb", &cfg->mergeProb, 0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.fissionProb", &cfg->fissionProb, 0.0f,
                         1.0f);
  ModEngineRegisterParam("dataTraffic.phaseShiftProb", &cfg->phaseShiftProb,
                         0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.phaseShiftIntensity",
                         &cfg->phaseShiftIntensity, 0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.springProb", &cfg->springProb, 0.0f,
                         1.0f);
  ModEngineRegisterParam("dataTraffic.springIntensity", &cfg->springIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.widthSpringProb", &cfg->widthSpringProb,
                         0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.widthSpringIntensity",
                         &cfg->widthSpringIntensity, 0.0f, 1.0f);
  ModEngineRegisterParam("dataTraffic.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("dataTraffic.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("dataTraffic.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("dataTraffic.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("dataTraffic.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("dataTraffic.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupDataTraffic(PostEffect *pe) {
  DataTrafficEffectSetup(&pe->dataTraffic, &pe->effects.dataTraffic,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupDataTrafficBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.dataTraffic.blendIntensity,
                       pe->effects.dataTraffic.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_DATA_TRAFFIC_BLEND, DataTraffic, dataTraffic,
                   "Data Traffic Blend", SetupDataTrafficBlend, SetupDataTraffic)
// clang-format on
