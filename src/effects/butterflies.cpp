// Butterflies effect module implementation
// Procedural butterfly field with Fourier-curve wings, gradient LUT colors,
// per-tile FFT brightness

#include "butterflies.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool ButterfliesEffectInit(ButterfliesEffect *e, const ButterfliesConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/butterflies.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->scrollPhaseLoc = GetShaderLocation(e->shader, "scrollPhase");
  e->driftPhaseLoc = GetShaderLocation(e->shader, "driftPhase");
  e->flapPhaseLoc = GetShaderLocation(e->shader, "flapPhase");
  e->shiftPhaseLoc = GetShaderLocation(e->shader, "shiftPhase");
  e->flapSpeedLoc = GetShaderLocation(e->shader, "flapSpeed");
  e->spreadLoc = GetShaderLocation(e->shader, "spread");
  e->patternDetailLoc = GetShaderLocation(e->shader, "patternDetail");
  e->wingShapeLoc = GetShaderLocation(e->shader, "wingShape");
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

  e->scrollPhase = 0.0f;
  e->driftPhase = 0.0f;
  e->flapPhase = 0.0f;
  e->shiftPhase = 0.0f;

  return true;
}

void ButterfliesEffectSetup(ButterfliesEffect *e, const ButterfliesConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture) {
  const float sampleRate = (float)AUDIO_SAMPLE_RATE;
  e->scrollPhase += cfg->scrollSpeed * deltaTime;
  e->driftPhase += cfg->driftSpeed * deltaTime;
  e->flapPhase += cfg->flapSpeed * deltaTime;
  e->shiftPhase += cfg->shiftSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollPhaseLoc, &e->scrollPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftPhaseLoc, &e->driftPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flapPhaseLoc, &e->flapPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shiftPhaseLoc, &e->shiftPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flapSpeedLoc, &cfg->flapSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spreadLoc, &cfg->spread, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->patternDetailLoc, &cfg->patternDetail,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wingShapeLoc, &cfg->wingShape,
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

void ButterfliesEffectUninit(ButterfliesEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void ButterfliesRegisterParams(ButterfliesConfig *cfg) {
  ModEngineRegisterParam("butterflies.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("butterflies.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("butterflies.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("butterflies.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("butterflies.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("butterflies.spread", &cfg->spread, 0.2f, 5.0f);
  ModEngineRegisterParam("butterflies.patternDetail", &cfg->patternDetail, 0.1f,
                         3.0f);
  ModEngineRegisterParam("butterflies.wingShape", &cfg->wingShape, 0.2f, 3.0f);
  ModEngineRegisterParam("butterflies.scrollSpeed", &cfg->scrollSpeed, -2.0f,
                         2.0f);
  ModEngineRegisterParam("butterflies.driftSpeed", &cfg->driftSpeed, -1.0f,
                         1.0f);
  ModEngineRegisterParam("butterflies.flapSpeed", &cfg->flapSpeed, 0.0f, 1.0f);
  ModEngineRegisterParam("butterflies.shiftSpeed", &cfg->shiftSpeed, 0.1f,
                         4.0f);
  ModEngineRegisterParam("butterflies.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

ButterfliesEffect *GetButterfliesEffect(PostEffect *pe) {
  return (ButterfliesEffect *)pe->effectStates[TRANSFORM_BUTTERFLIES_BLEND];
}

void SetupButterflies(PostEffect *pe) {
  ButterfliesEffectSetup(GetButterfliesEffect(pe), &pe->effects.butterflies,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupButterfliesBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.butterflies.blendIntensity,
                       pe->effects.butterflies.blendMode);
}

// === UI ===

static void DrawButterfliesParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  ButterfliesConfig *b = &e->butterflies;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##butterflies", &b->baseFreq,
                    "butterflies.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##butterflies", &b->maxFreq,
                    "butterflies.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##butterflies", &b->gain, "butterflies.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##butterflies", &b->curve, "butterflies.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##butterflies", &b->baseBright,
                    "butterflies.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Spread##butterflies", &b->spread, "butterflies.spread",
                    "%.2f", modSources);
  ModulatableSlider("Pattern Detail##butterflies", &b->patternDetail,
                    "butterflies.patternDetail", "%.2f", modSources);
  ModulatableSlider("Wing Shape##butterflies", &b->wingShape,
                    "butterflies.wingShape", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Scroll Speed##butterflies", &b->scrollSpeed,
                    "butterflies.scrollSpeed", "%.2f", modSources);
  ModulatableSlider("Drift Speed##butterflies", &b->driftSpeed,
                    "butterflies.driftSpeed", "%.2f", modSources);
  ModulatableSliderLog("Flap Rate##butterflies", &b->flapSpeed,
                       "butterflies.flapSpeed", "%.3f", modSources);
  ModulatableSlider("Shift Rate##butterflies", &b->shiftSpeed,
                    "butterflies.shiftSpeed", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(butterflies)
REGISTER_GENERATOR(TRANSFORM_BUTTERFLIES_BLEND, Butterflies, butterflies, "Butterflies",
                   SetupButterfliesBlend, SetupButterflies, 15, DrawButterfliesParams, DrawOutput_butterflies)
// clang-format on
