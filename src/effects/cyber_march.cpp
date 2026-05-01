// Cyber March effect module implementation
// Raymarched folded-space fractal with FFT-reactive coloring and gradient LUT

#include "cyber_march.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
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

bool CyberMarchEffectInit(CyberMarchEffect *e, const CyberMarchConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/cyber_march.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->flyPhaseLoc = GetShaderLocation(e->shader, "flyPhase");
  e->morphPhaseLoc = GetShaderLocation(e->shader, "morphPhase");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->stepSizeLoc = GetShaderLocation(e->shader, "stepSize");
  e->fovLoc = GetShaderLocation(e->shader, "fov");
  e->domainSizeLoc = GetShaderLocation(e->shader, "domainSize");
  e->foldIterationsLoc = GetShaderLocation(e->shader, "foldIterations");
  e->foldScaleLoc = GetShaderLocation(e->shader, "foldScale");
  e->morphAmountLoc = GetShaderLocation(e->shader, "morphAmount");
  e->foldOffsetLoc = GetShaderLocation(e->shader, "foldOffset");
  e->boxFoldLoc = GetShaderLocation(e->shader, "boxFold");
  e->initialScaleLoc = GetShaderLocation(e->shader, "initialScale");
  e->colorSpreadLoc = GetShaderLocation(e->shader, "colorSpread");
  e->tonemapGainLoc = GetShaderLocation(e->shader, "tonemapGain");
  e->yawLoc = GetShaderLocation(e->shader, "yaw");
  e->pitchLoc = GetShaderLocation(e->shader, "pitch");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->colorFreqMapLoc = GetShaderLocation(e->shader, "colorFreqMap");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->flyPhase = 0.0f;
  e->morphPhase = 0.0f;

  return true;
}

static void BindUniforms(const CyberMarchEffect *e,
                         const CyberMarchConfig *cfg) {
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->stepSizeLoc, &cfg->stepSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fovLoc, &cfg->fov, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->domainSizeLoc, &cfg->domainSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->foldIterationsLoc, &cfg->foldIterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->foldScaleLoc, &cfg->foldScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->morphAmountLoc, &cfg->morphAmount,
                 SHADER_UNIFORM_FLOAT);
  const float foldOff[3] = {cfg->foldOffsetX, cfg->foldOffsetY,
                            cfg->foldOffsetZ};
  SetShaderValue(e->shader, e->foldOffsetLoc, foldOff, SHADER_UNIFORM_VEC3);
  SetShaderValue(e->shader, e->boxFoldLoc, &cfg->boxFold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->initialScaleLoc, &cfg->initialScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorSpreadLoc, &cfg->colorSpread,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tonemapGainLoc, &cfg->tonemapGain,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  const int colorFreqMapInt = cfg->colorFreqMap ? 1 : 0;
  SetShaderValue(e->shader, e->colorFreqMapLoc, &colorFreqMapInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void CyberMarchEffectSetup(CyberMarchEffect *e, CyberMarchConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture) {
  e->flyPhase += cfg->flySpeed * deltaTime;
  e->morphPhase += cfg->morphSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flyPhaseLoc, &e->flyPhase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->morphPhaseLoc, &e->morphPhase,
                 SHADER_UNIFORM_FLOAT);

  BindUniforms(e, cfg);

  float yawVal;
  float pitchVal;
  DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &yawVal, &pitchVal);
  const float maxPitch = 0.785f;
  pitchVal = fmaxf(-maxPitch, fminf(maxPitch, pitchVal));
  SetShaderValue(e->shader, e->yawLoc, &yawVal, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pitchLoc, &pitchVal, SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void CyberMarchEffectUninit(CyberMarchEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void CyberMarchRegisterParams(CyberMarchConfig *cfg) {
  ModEngineRegisterParam("cyberMarch.stepSize", &cfg->stepSize, 0.1f, 1.0f);
  ModEngineRegisterParam("cyberMarch.fov", &cfg->fov, 0.2f, 1.0f);
  ModEngineRegisterParam("cyberMarch.domainSize", &cfg->domainSize, 10.0f,
                         60.0f);
  ModEngineRegisterParam("cyberMarch.foldScale", &cfg->foldScale, 1.0f, 2.0f);
  ModEngineRegisterParam("cyberMarch.morphAmount", &cfg->morphAmount, 0.0f,
                         0.5f);
  ModEngineRegisterParam("cyberMarch.foldOffsetX", &cfg->foldOffsetX, 1.0f,
                         200.0f);
  ModEngineRegisterParam("cyberMarch.foldOffsetY", &cfg->foldOffsetY, 1.0f,
                         200.0f);
  ModEngineRegisterParam("cyberMarch.foldOffsetZ", &cfg->foldOffsetZ, 1.0f,
                         200.0f);
  ModEngineRegisterParam("cyberMarch.boxFold", &cfg->boxFold, 0.05f, 1.0f);
  ModEngineRegisterParam("cyberMarch.initialScale", &cfg->initialScale, 1.0f,
                         10.0f);
  ModEngineRegisterParam("cyberMarch.colorSpread", &cfg->colorSpread, 0.01f,
                         2.0f);
  ModEngineRegisterParam("cyberMarch.tonemapGain", &cfg->tonemapGain, 0.0001f,
                         0.003f);
  ModEngineRegisterParam("cyberMarch.flySpeed", &cfg->flySpeed, 0.0f, 20.0f);
  ModEngineRegisterParam("cyberMarch.morphSpeed", &cfg->morphSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("cyberMarch.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("cyberMarch.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("cyberMarch.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("cyberMarch.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("cyberMarch.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("cyberMarch.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 2.0f);
  ModEngineRegisterParam("cyberMarch.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("cyberMarch.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

CyberMarchEffect *GetCyberMarchEffect(PostEffect *pe) {
  return (CyberMarchEffect *)pe->effectStates[TRANSFORM_CYBER_MARCH_BLEND];
}

void SetupCyberMarch(PostEffect *pe) {
  CyberMarchEffectSetup(GetCyberMarchEffect(pe), &pe->effects.cyberMarch,
                        pe->currentDeltaTime, pe->fftTexture);
}

void SetupCyberMarchBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.cyberMarch.blendIntensity,
                       pe->effects.cyberMarch.blendMode);
}

// === UI ===

static void DrawCyberMarchParams(EffectConfig *e, const ModSources *modSources,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;
  CyberMarchConfig *cm = &e->cyberMarch;

  // Audio
  ImGui::SeparatorText("Audio");
  ImGui::Checkbox("Color Freq Map##cyberMarch", &cm->colorFreqMap);
  ModulatableSlider("Base Freq (Hz)##cyberMarch", &cm->baseFreq,
                    "cyberMarch.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##cyberMarch", &cm->maxFreq,
                    "cyberMarch.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##cyberMarch", &cm->gain, "cyberMarch.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##cyberMarch", &cm->curve, "cyberMarch.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##cyberMarch", &cm->baseBright,
                    "cyberMarch.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("March Steps##cyberMarch", &cm->marchSteps, 30, 100);
  ModulatableSlider("Step Size##cyberMarch", &cm->stepSize,
                    "cyberMarch.stepSize", "%.2f", modSources);
  ModulatableSlider("FOV##cyberMarch", &cm->fov, "cyberMarch.fov", "%.2f",
                    modSources);
  ModulatableSlider("Domain Size##cyberMarch", &cm->domainSize,
                    "cyberMarch.domainSize", "%.1f", modSources);
  ImGui::SliderInt("Fold Iterations##cyberMarch", &cm->foldIterations, 2, 12);
  ModulatableSlider("Fold Scale##cyberMarch", &cm->foldScale,
                    "cyberMarch.foldScale", "%.2f", modSources);
  ModulatableSlider("Morph Amount##cyberMarch", &cm->morphAmount,
                    "cyberMarch.morphAmount", "%.2f", modSources);
  ModulatableSlider("Morph Speed##cyberMarch", &cm->morphSpeed,
                    "cyberMarch.morphSpeed", "%.2f", modSources);
  ModulatableSlider("Box Fold##cyberMarch", &cm->boxFold, "cyberMarch.boxFold",
                    "%.2f", modSources);
  ModulatableSlider("Initial Scale##cyberMarch", &cm->initialScale,
                    "cyberMarch.initialScale", "%.1f", modSources);
  ModulatableSlider("Fold Offset X##cyberMarch", &cm->foldOffsetX,
                    "cyberMarch.foldOffsetX", "%.1f", modSources);
  ModulatableSlider("Fold Offset Y##cyberMarch", &cm->foldOffsetY,
                    "cyberMarch.foldOffsetY", "%.1f", modSources);
  ModulatableSlider("Fold Offset Z##cyberMarch", &cm->foldOffsetZ,
                    "cyberMarch.foldOffsetZ", "%.1f", modSources);
  ModulatableSlider("Color Spread##cyberMarch", &cm->colorSpread,
                    "cyberMarch.colorSpread", "%.2f", modSources);
  ModulatableSliderLog("Tonemap Gain##cyberMarch", &cm->tonemapGain,
                       "cyberMarch.tonemapGain", "%.4f", modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  ModulatableSlider("Fly Speed##cyberMarch", &cm->flySpeed,
                    "cyberMarch.flySpeed", "%.2f", modSources);
  DrawLissajousControls(&cm->lissajous, "cm_cam", "cyberMarch.lissajous",
                        modSources, 2.0f);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(cyberMarch)
REGISTER_GENERATOR(TRANSFORM_CYBER_MARCH_BLEND, CyberMarch, cyberMarch, "Cyber March",
                   SetupCyberMarchBlend, SetupCyberMarch, 13, DrawCyberMarchParams, DrawOutput_cyberMarch)
// clang-format on
