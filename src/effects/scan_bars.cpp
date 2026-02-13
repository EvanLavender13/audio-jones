// Scan bars effect module implementation
// Generates scrolling bar patterns (linear, spokes, rings) with palette-driven
// color chaos

#include "scan_bars.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "render/color_lut.h"
#include <math.h>
#include <stddef.h>

bool ScanBarsEffectInit(ScanBarsEffect *e, const ScanBarsConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/scan_bars.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->modeLoc = GetShaderLocation(e->shader, "mode");
  e->angleLoc = GetShaderLocation(e->shader, "angle");
  e->barDensityLoc = GetShaderLocation(e->shader, "barDensity");
  e->convergenceLoc = GetShaderLocation(e->shader, "convergence");
  e->convergenceFreqLoc = GetShaderLocation(e->shader, "convergenceFreq");
  e->convergenceOffsetLoc = GetShaderLocation(e->shader, "convergenceOffset");
  e->sharpnessLoc = GetShaderLocation(e->shader, "sharpness");
  e->scrollPhaseLoc = GetShaderLocation(e->shader, "scrollPhase");
  e->colorPhaseLoc = GetShaderLocation(e->shader, "colorPhase");
  e->chaosFreqLoc = GetShaderLocation(e->shader, "chaosFreq");
  e->chaosIntensityLoc = GetShaderLocation(e->shader, "chaosIntensity");
  e->snapAmountLoc = GetShaderLocation(e->shader, "snapAmount");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->numOctavesLoc = GetShaderLocation(e->shader, "numOctaves");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->scrollPhase = 0.0f;
  e->colorPhase = 0.0f;

  return true;
}

// Apply snap quantization: smooth at snapAmount=0, lurching at higher values
static float SnapPhase(float phase, float snapAmount) {
  const float whole = floorf(phase);
  const float frac = phase - whole;
  return whole + powf(frac, snapAmount + 1.0f);
}

void ScanBarsEffectSetup(ScanBarsEffect *e, const ScanBarsConfig *cfg,
                         float deltaTime, Texture2D fftTexture) {
  e->scrollPhase += cfg->scrollSpeed * deltaTime;
  e->colorPhase += cfg->colorSpeed * deltaTime;

  float snappedScroll = SnapPhase(e->scrollPhase, cfg->snapAmount);
  float snappedColor = SnapPhase(e->colorPhase, cfg->snapAmount);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->angleLoc, &cfg->angle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->barDensityLoc, &cfg->barDensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->convergenceLoc, &cfg->convergence,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->convergenceFreqLoc, &cfg->convergenceFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->convergenceOffsetLoc, &cfg->convergenceOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sharpnessLoc, &cfg->sharpness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollPhaseLoc, &snappedScroll,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorPhaseLoc, &snappedColor,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->chaosFreqLoc, &cfg->chaosFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->chaosIntensityLoc, &cfg->chaosIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->snapAmountLoc, &cfg->snapAmount,
                 SHADER_UNIFORM_FLOAT);

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
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void ScanBarsEffectUninit(ScanBarsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

ScanBarsConfig ScanBarsConfigDefault(void) { return ScanBarsConfig{}; }

void ScanBarsRegisterParams(ScanBarsConfig *cfg) {
  ModEngineRegisterParam("scanBars.angle", &cfg->angle, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("scanBars.barDensity", &cfg->barDensity, 1.0f, 100.0f);
  ModEngineRegisterParam("scanBars.convergence", &cfg->convergence, 0.0f, 2.0f);
  ModEngineRegisterParam("scanBars.convergenceFreq", &cfg->convergenceFreq,
                         0.0f, 20.0f);
  ModEngineRegisterParam("scanBars.convergenceOffset", &cfg->convergenceOffset,
                         -1.0f, 1.0f);
  ModEngineRegisterParam("scanBars.sharpness", &cfg->sharpness, 0.01f, 1.0f);
  ModEngineRegisterParam("scanBars.scrollSpeed", &cfg->scrollSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("scanBars.colorSpeed", &cfg->colorSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("scanBars.chaosFreq", &cfg->chaosFreq, 0.0f, 50.0f);
  ModEngineRegisterParam("scanBars.chaosIntensity", &cfg->chaosIntensity, 0.0f,
                         5.0f);
  ModEngineRegisterParam("scanBars.snapAmount", &cfg->snapAmount, 0.0f, 2.0f);
  ModEngineRegisterParam("scanBars.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("scanBars.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("scanBars.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("scanBars.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("scanBars.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}
