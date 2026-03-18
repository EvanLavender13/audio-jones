// Triskelion generator - fractal tiled grid with iterated space-folding and
// concentric circle interference, colored via gradient LUT with FFT reactivity

#include "triskelion.h"
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

bool TriskelionEffectInit(TriskelionEffect *e, const TriskelionConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/triskelion.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->foldModeLoc = GetShaderLocation(e->shader, "foldMode");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->circleFreqLoc = GetShaderLocation(e->shader, "circleFreq");
  e->colorCyclesLoc = GetShaderLocation(e->shader, "colorCycles");
  e->rotationAngleLoc = GetShaderLocation(e->shader, "rotationAngle");
  e->scaleAngleLoc = GetShaderLocation(e->shader, "scaleAngle");
  e->scaleAmountLoc = GetShaderLocation(e->shader, "scaleAmount");
  e->colorPhaseLoc = GetShaderLocation(e->shader, "colorPhase");
  e->circlePhaseLoc = GetShaderLocation(e->shader, "circlePhase");
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

  e->rotationAngle = 0.0f;
  e->scaleAngle = 0.0f;
  e->colorPhase = 0.0f;
  e->circlePhase = 0.0f;

  return true;
}

void TriskelionEffectSetup(TriskelionEffect *e, TriskelionConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture) {
  e->rotationAngle += cfg->rotationSpeed * deltaTime;
  e->scaleAngle += cfg->scaleSpeed * deltaTime;
  e->colorPhase += cfg->colorSpeed * deltaTime;
  e->circlePhase += cfg->circleSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->foldModeLoc, &cfg->foldMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->circleFreqLoc, &cfg->circleFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorCyclesLoc, &cfg->colorCycles,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAngleLoc, &e->rotationAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleAngleLoc, &e->scaleAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleAmountLoc, &cfg->scaleAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorPhaseLoc, &e->colorPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->circlePhaseLoc, &e->circlePhase,
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
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
}

void TriskelionEffectUninit(TriskelionEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void TriskelionRegisterParams(TriskelionConfig *cfg) {
  ModEngineRegisterParam("triskelion.circleFreq", &cfg->circleFreq, 1.0f,
                         20.0f);
  ModEngineRegisterParam("triskelion.colorCycles", &cfg->colorCycles, 0.1f,
                         10.0f);
  ModEngineRegisterParam("triskelion.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("triskelion.scaleSpeed", &cfg->scaleSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("triskelion.scaleAmount", &cfg->scaleAmount, 0.01f,
                         0.3f);
  ModEngineRegisterParam("triskelion.colorSpeed", &cfg->colorSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("triskelion.circleSpeed", &cfg->circleSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("triskelion.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("triskelion.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("triskelion.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("triskelion.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("triskelion.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("triskelion.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupTriskelion(PostEffect *pe) {
  TriskelionEffectSetup(&pe->triskelion, &pe->effects.triskelion,
                        pe->currentDeltaTime, pe->fftTexture);
}

void SetupTriskelionBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.triskelion.blendIntensity,
                       pe->effects.triskelion.blendMode);
}

// === UI ===

static void DrawTriskelionParams(EffectConfig *e, const ModSources *modSources,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;
  TriskelionConfig *cfg = &e->triskelion;

  // Geometry
  ImGui::SeparatorText("Geometry");
  const char *foldModes[] = {"Square", "Hex"};
  ImGui::Combo("Fold Mode##triskelion", &cfg->foldMode, foldModes, 2);
  ImGui::SliderInt("Layers##triskelion", &cfg->layers, 4, 32);
  ModulatableSlider("Circle Freq##triskelion", &cfg->circleFreq,
                    "triskelion.circleFreq", "%.2f", modSources);
  ModulatableSlider("Color Cycles##triskelion", &cfg->colorCycles,
                    "triskelion.colorCycles", "%.1f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotation Speed##triskelion", &cfg->rotationSpeed,
                            "triskelion.rotationSpeed", modSources);
  ModulatableSliderSpeedDeg("Scale Speed##triskelion", &cfg->scaleSpeed,
                            "triskelion.scaleSpeed", modSources);
  ModulatableSlider("Scale Amount##triskelion", &cfg->scaleAmount,
                    "triskelion.scaleAmount", "%.3f", modSources);
  ModulatableSliderSpeedDeg("Color Speed##triskelion", &cfg->colorSpeed,
                            "triskelion.colorSpeed", modSources);
  ModulatableSliderSpeedDeg("Circle Speed##triskelion", &cfg->circleSpeed,
                            "triskelion.circleSpeed", modSources);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##triskelion", &cfg->baseFreq,
                    "triskelion.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##triskelion", &cfg->maxFreq,
                    "triskelion.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##triskelion", &cfg->gain, "triskelion.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##triskelion", &cfg->curve, "triskelion.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##triskelion", &cfg->baseBright,
                    "triskelion.baseBright", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(triskelion)
REGISTER_GENERATOR(TRANSFORM_TRISKELION_BLEND, Triskelion, triskelion,
                   "Triskelion", SetupTriskelionBlend,
                   SetupTriskelion, 10, DrawTriskelionParams, DrawOutput_triskelion)
// clang-format on
