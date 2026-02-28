// Motherboard effect module implementation
// Kali-family circuit fractals with three modes from the Circuits series

#include "motherboard.h"
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

bool MotherboardEffectInit(MotherboardEffect *e, const MotherboardConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/motherboard.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->modeLoc = GetShaderLocation(e->shader, "mode");
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
  e->traceWidthLoc = GetShaderLocation(e->shader, "traceWidth");
  e->panAccumLoc = GetShaderLocation(e->shader, "panAccum");
  e->flowAccumLoc = GetShaderLocation(e->shader, "flowAccum");
  e->flowIntensityLoc = GetShaderLocation(e->shader, "flowIntensity");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
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
  SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);
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
  SetShaderValue(e->shader, e->traceWidthLoc, &cfg->traceWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->panAccumLoc, &e->panAccum, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flowAccumLoc, &e->flowAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flowIntensityLoc, &cfg->flowIntensity,
                 SHADER_UNIFORM_FLOAT);
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
  ModEngineRegisterParam("motherboard.zoom", &cfg->zoom, 0.5f, 4.0f);
  ModEngineRegisterParam("motherboard.clampLo", &cfg->clampLo, 0.0f, 1.0f);
  ModEngineRegisterParam("motherboard.clampHi", &cfg->clampHi, 0.5f, 5.0f);
  ModEngineRegisterParam("motherboard.foldConstant", &cfg->foldConstant, 0.5f,
                         2.0f);
  ModEngineRegisterParam("motherboard.traceWidth", &cfg->traceWidth, 0.001f,
                         0.05f);
  ModEngineRegisterParam("motherboard.panSpeed", &cfg->panSpeed, -2.0f, 2.0f);
  ModEngineRegisterParam("motherboard.flowSpeed", &cfg->flowSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("motherboard.flowIntensity", &cfg->flowIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("motherboard.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
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

// === UI ===

static const char *MOTHERBOARD_MODE_NAMES = "Kali Dot\0Stepping\0Tiled Fold\0";

static void DrawMotherboardParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  MotherboardConfig *cfg = &e->motherboard;

  // Mode
  ImGui::Combo("Mode##motherboard", &cfg->mode, MOTHERBOARD_MODE_NAMES);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##motherboard", &cfg->baseFreq,
                    "motherboard.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##motherboard", &cfg->maxFreq,
                    "motherboard.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##motherboard", &cfg->gain, "motherboard.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##motherboard", &cfg->curve, "motherboard.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##motherboard", &cfg->baseBright,
                    "motherboard.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##motherboard", &cfg->iterations, 4, 16);
  ModulatableSlider("Zoom##motherboard", &cfg->zoom, "motherboard.zoom", "%.2f",
                    modSources);
  ModulatableSlider("Clamp Lo##motherboard", &cfg->clampLo,
                    "motherboard.clampLo", "%.2f", modSources);
  ModulatableSlider("Clamp Hi##motherboard", &cfg->clampHi,
                    "motherboard.clampHi", "%.2f", modSources);
  ModulatableSlider("Fold Constant##motherboard", &cfg->foldConstant,
                    "motherboard.foldConstant", "%.2f", modSources);

  // Rendering
  ImGui::SeparatorText("Rendering");
  ModulatableSliderLog("Trace Width##motherboard", &cfg->traceWidth,
                       "motherboard.traceWidth", "%.4f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Pan Speed##motherboard", &cfg->panSpeed,
                    "motherboard.panSpeed", "%.2f", modSources);
  ModulatableSlider("Flow Speed##motherboard", &cfg->flowSpeed,
                    "motherboard.flowSpeed", "%.2f", modSources);
  ModulatableSlider("Flow Intensity##motherboard", &cfg->flowIntensity,
                    "motherboard.flowIntensity", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Rotation Speed##motherboard", &cfg->rotationSpeed,
                            "motherboard.rotationSpeed", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(motherboard)
REGISTER_GENERATOR(TRANSFORM_MOTHERBOARD_BLEND, Motherboard, motherboard,
                   "Motherboard", SetupMotherboardBlend,
                   SetupMotherboard, 12, DrawMotherboardParams, DrawOutput_motherboard)
// clang-format on
