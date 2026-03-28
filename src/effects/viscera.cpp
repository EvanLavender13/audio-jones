// Viscera effect module implementation
// Organic domain-warped noise with bump-mapped lighting and radial pulsation

#include "viscera.h"
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

bool VisceraEffectInit(VisceraEffect *e, const VisceraConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/viscera.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->twistAngleLoc = GetShaderLocation(e->shader, "twistAngle");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->freqGrowthLoc = GetShaderLocation(e->shader, "freqGrowth");
  e->warpIntensityLoc = GetShaderLocation(e->shader, "warpIntensity");
  e->pulseAmplitudeLoc = GetShaderLocation(e->shader, "pulseAmplitude");
  e->pulseFreqLoc = GetShaderLocation(e->shader, "pulseFreq");
  e->pulseRadialLoc = GetShaderLocation(e->shader, "pulseRadial");
  e->bumpStrengthLoc = GetShaderLocation(e->shader, "bumpDepth");
  e->specPowerLoc = GetShaderLocation(e->shader, "specPower");
  e->specIntensityLoc = GetShaderLocation(e->shader, "specIntensity");
  e->heightScaleLoc = GetShaderLocation(e->shader, "heightScale");
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

  return true;
}

void VisceraEffectSetup(VisceraEffect *e, const VisceraConfig *cfg,
                        float deltaTime, const Texture2D &fftTexture) {
  e->time += cfg->animSpeed * deltaTime;
  e->currentFFTTexture = fftTexture;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAngleLoc, &cfg->twistAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->freqGrowthLoc, &cfg->freqGrowth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpIntensityLoc, &cfg->warpIntensity,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->pulseAmplitudeLoc, &cfg->pulseAmplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pulseFreqLoc, &cfg->pulseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pulseRadialLoc, &cfg->pulseRadial,
                 SHADER_UNIFORM_FLOAT);

  const float bumpDepth = 1.0f / cfg->bumpStrength;
  SetShaderValue(e->shader, e->bumpStrengthLoc, &bumpDepth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->specPowerLoc, &cfg->specPower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->specIntensityLoc, &cfg->specIntensity,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->heightScaleLoc, &cfg->heightScale,
                 SHADER_UNIFORM_FLOAT);

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
  SetShaderValueTexture(e->shader, e->fftTextureLoc, e->currentFFTTexture);
}

void VisceraEffectUninit(VisceraEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void VisceraRegisterParams(VisceraConfig *cfg) {
  ModEngineRegisterParam("viscera.scale", &cfg->scale, 1.0f, 20.0f);
  ModEngineRegisterParam("viscera.twistAngle", &cfg->twistAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("viscera.freqGrowth", &cfg->freqGrowth, 1.05f, 1.5f);
  ModEngineRegisterParam("viscera.warpIntensity", &cfg->warpIntensity, 0.0f,
                         2.0f);
  ModEngineRegisterParam("viscera.animSpeed", &cfg->animSpeed, 0.1f, 8.0f);
  ModEngineRegisterParam("viscera.pulseAmplitude", &cfg->pulseAmplitude, 0.0f,
                         2.0f);
  ModEngineRegisterParam("viscera.pulseFreq", &cfg->pulseFreq, 0.5f, 8.0f);
  ModEngineRegisterParam("viscera.pulseRadial", &cfg->pulseRadial, 1.0f, 12.0f);
  ModEngineRegisterParam("viscera.bumpStrength", &cfg->bumpStrength, 2.0f,
                         100.0f);
  ModEngineRegisterParam("viscera.specPower", &cfg->specPower, 4.0f, 128.0f);
  ModEngineRegisterParam("viscera.specIntensity", &cfg->specIntensity, 0.0f,
                         2.0f);
  ModEngineRegisterParam("viscera.heightScale", &cfg->heightScale, 0.5f, 3.0f);
  ModEngineRegisterParam("viscera.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("viscera.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("viscera.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("viscera.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("viscera.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("viscera.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupViscera(PostEffect *pe) {
  VisceraEffectSetup(&pe->viscera, &pe->effects.viscera, pe->currentDeltaTime,
                     pe->fftTexture);
}

void SetupVisceraBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.viscera.blendIntensity,
                       pe->effects.viscera.blendMode);
}

// === UI ===

static void DrawVisceraParams(EffectConfig *e, const ModSources *modSources,
                              ImU32 categoryGlow) {
  (void)categoryGlow;
  VisceraConfig *v = &e->viscera;

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Scale##viscera", &v->scale, "viscera.scale", "%.1f",
                    modSources);
  ModulatableSliderAngleDeg("Twist Angle##viscera", &v->twistAngle,
                            "viscera.twistAngle", modSources);
  ImGui::SliderInt("Iterations##viscera", &v->iterations, 4, 24);
  ModulatableSlider("Freq Growth##viscera", &v->freqGrowth,
                    "viscera.freqGrowth", "%.2f", modSources);
  ModulatableSlider("Warp Intensity##viscera", &v->warpIntensity,
                    "viscera.warpIntensity", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Anim Speed##viscera", &v->animSpeed, "viscera.animSpeed",
                    "%.2f", modSources);

  // Pulsation
  ImGui::SeparatorText("Pulsation");
  ModulatableSlider("Pulse Amplitude##viscera", &v->pulseAmplitude,
                    "viscera.pulseAmplitude", "%.2f", modSources);
  ModulatableSlider("Pulse Freq##viscera", &v->pulseFreq, "viscera.pulseFreq",
                    "%.2f", modSources);
  ModulatableSlider("Pulse Radial##viscera", &v->pulseRadial,
                    "viscera.pulseRadial", "%.1f", modSources);

  // Lighting
  ImGui::SeparatorText("Lighting");
  ModulatableSlider("Bump Strength##viscera", &v->bumpStrength,
                    "viscera.bumpStrength", "%.1f", modSources);
  ModulatableSlider("Spec Power##viscera", &v->specPower, "viscera.specPower",
                    "%.1f", modSources);
  ModulatableSlider("Spec Intensity##viscera", &v->specIntensity,
                    "viscera.specIntensity", "%.2f", modSources);

  // Height
  ImGui::SeparatorText("Height");
  ModulatableSlider("Height Scale##viscera", &v->heightScale,
                    "viscera.heightScale", "%.2f", modSources);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##viscera", &v->baseFreq, "viscera.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##viscera", &v->maxFreq, "viscera.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##viscera", &v->gain, "viscera.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##viscera", &v->curve, "viscera.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##viscera", &v->baseBright,
                    "viscera.baseBright", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(viscera)
REGISTER_GENERATOR(TRANSFORM_VISCERA_BLEND, Viscera, viscera, "Viscera",
                   SetupVisceraBlend, SetupViscera, 12,
                   DrawVisceraParams, DrawOutput_viscera)
// clang-format on
