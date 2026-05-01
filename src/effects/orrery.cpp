// Orrery effect module implementation
// Hierarchical ring tree with FFT-driven brightness and connecting lines

#include "orrery.h"
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
#include <math.h>
#include <stddef.h>

bool OrreryEffectInit(OrreryEffect *e, const OrreryConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/orrery.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->levelPhaseLoc = GetShaderLocation(e->shader, "levelPhase");
  e->variationPhaseLoc = GetShaderLocation(e->shader, "variationPhase");
  e->branchesLoc = GetShaderLocation(e->shader, "branches");
  e->depthLoc = GetShaderLocation(e->shader, "depth");
  e->rootRadiusLoc = GetShaderLocation(e->shader, "rootRadius");
  e->radiusDecayLoc = GetShaderLocation(e->shader, "radiusDecay");
  e->radiusVariationLoc = GetShaderLocation(e->shader, "radiusVariation");
  e->strokeWidthLoc = GetShaderLocation(e->shader, "strokeWidth");
  e->strokeTaperLoc = GetShaderLocation(e->shader, "strokeTaper");
  e->lineModeLoc = GetShaderLocation(e->shader, "lineMode");
  e->lineSeedLoc = GetShaderLocation(e->shader, "lineSeed");
  e->lineCountLoc = GetShaderLocation(e->shader, "lineCount");
  e->lineBrightnessLoc = GetShaderLocation(e->shader, "lineBrightness");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  for (int i = 0; i < 5; i++) {
    e->levelPhase[i] = 0.0f;
    e->variationPhase[i] = 0.0f;
  }

  return true;
}

void OrreryEffectSetup(OrreryEffect *e, const OrreryConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture) {
  for (int L = 0; L < 5; L++) {
    const float scale = powf(cfg->speedScale, static_cast<float>(L));
    e->levelPhase[L] += cfg->baseSpeed * scale * deltaTime;
    e->variationPhase[L] += cfg->speedVariation * scale * deltaTime;
  }

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueV(e->shader, e->levelPhaseLoc, e->levelPhase,
                  SHADER_UNIFORM_FLOAT, 5);
  SetShaderValueV(e->shader, e->variationPhaseLoc, e->variationPhase,
                  SHADER_UNIFORM_FLOAT, 5);
  SetShaderValue(e->shader, e->branchesLoc, &cfg->branches, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->depthLoc, &cfg->depth, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->rootRadiusLoc, &cfg->rootRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radiusDecayLoc, &cfg->radiusDecay,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radiusVariationLoc, &cfg->radiusVariation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strokeWidthLoc, &cfg->strokeWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strokeTaperLoc, &cfg->strokeTaper,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineModeLoc, &cfg->lineMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lineSeedLoc, &cfg->lineSeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineCountLoc, &cfg->lineCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lineBrightnessLoc, &cfg->lineBrightness,
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

void OrreryEffectUninit(OrreryEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void OrreryRegisterParams(OrreryConfig *cfg) {
  ModEngineRegisterParam("orrery.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("orrery.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("orrery.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("orrery.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("orrery.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("orrery.rootRadius", &cfg->rootRadius, 0.1f, 0.8f);
  ModEngineRegisterParam("orrery.radiusDecay", &cfg->radiusDecay, 0.0f, 0.9f);
  ModEngineRegisterParam("orrery.radiusVariation", &cfg->radiusVariation, 0.0f,
                         1.0f);
  ModEngineRegisterParam("orrery.baseSpeed", &cfg->baseSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("orrery.speedScale", &cfg->speedScale, 1.0f, 4.0f);
  ModEngineRegisterParam("orrery.speedVariation", &cfg->speedVariation, 0.0f,
                         1.0f);
  ModEngineRegisterParam("orrery.strokeWidth", &cfg->strokeWidth, 0.001f,
                         0.02f);
  ModEngineRegisterParam("orrery.strokeTaper", &cfg->strokeTaper, 0.0f, 1.0f);
  ModEngineRegisterParam("orrery.lineSeed", &cfg->lineSeed, 0.0f, 100.0f);
  ModEngineRegisterParam("orrery.lineBrightness", &cfg->lineBrightness, 0.0f,
                         1.0f);
  ModEngineRegisterParam("orrery.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

OrreryEffect *GetOrreryEffect(PostEffect *pe) {
  return (OrreryEffect *)pe->effectStates[TRANSFORM_ORRERY_BLEND];
}

void SetupOrrery(PostEffect *pe) {
  OrreryEffectSetup(GetOrreryEffect(pe), &pe->effects.orrery,
                    pe->currentDeltaTime, pe->fftTexture);
}

void SetupOrreryBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.orrery.blendIntensity,
                       pe->effects.orrery.blendMode);
}

// === UI ===

static void DrawOrreryParams(EffectConfig *e, const ModSources *modSources,
                             ImU32 categoryGlow) {
  (void)categoryGlow;
  OrreryConfig *cfg = &e->orrery;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##orrery", &cfg->baseFreq, "orrery.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##orrery", &cfg->maxFreq, "orrery.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##orrery", &cfg->gain, "orrery.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##orrery", &cfg->curve, "orrery.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##orrery", &cfg->baseBright,
                    "orrery.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Branches##orrery", &cfg->branches, 2, 5);
  ImGui::SliderInt("Depth##orrery", &cfg->depth, 2, 4);
  ModulatableSlider("Root Radius##orrery", &cfg->rootRadius,
                    "orrery.rootRadius", "%.2f", modSources);
  ModulatableSlider("Radius Decay##orrery", &cfg->radiusDecay,
                    "orrery.radiusDecay", "%.2f", modSources);
  ModulatableSlider("Radius Variation##orrery", &cfg->radiusVariation,
                    "orrery.radiusVariation", "%.2f", modSources);

  // Lines
  ImGui::SeparatorText("Lines");
  ImGui::Combo("Line Mode##orrery", &cfg->lineMode, "Seeded\0Siblings\0");
  ModulatableSlider("Line Seed##orrery", &cfg->lineSeed, "orrery.lineSeed",
                    "%.1f", modSources);
  ImGui::SliderInt("Line Count##orrery", &cfg->lineCount, 0, 20);
  ModulatableSlider("Line Brightness##orrery", &cfg->lineBrightness,
                    "orrery.lineBrightness", "%.2f", modSources);

  // Stroke
  ImGui::SeparatorText("Stroke");
  ModulatableSliderLog("Stroke Width##orrery", &cfg->strokeWidth,
                       "orrery.strokeWidth", "%.3f", modSources);
  ModulatableSlider("Stroke Taper##orrery", &cfg->strokeTaper,
                    "orrery.strokeTaper", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Base Speed##orrery", &cfg->baseSpeed,
                            "orrery.baseSpeed", modSources);
  ModulatableSlider("Speed Scale##orrery", &cfg->speedScale,
                    "orrery.speedScale", "%.2f", modSources);
  ModulatableSlider("Speed Variation##orrery", &cfg->speedVariation,
                    "orrery.speedVariation", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(orrery)
REGISTER_GENERATOR(TRANSFORM_ORRERY_BLEND, Orrery, orrery,
                   "Orrery", SetupOrreryBlend,
                   SetupOrrery, 10, DrawOrreryParams, DrawOutput_orrery)
// clang-format on
