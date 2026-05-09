// DiamondWeave effect module implementation
// Twisted diamond tile grid with FFT-driven ringing patterns and radial twist

#include "diamond_weave.h"
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

bool DiamondWeaveEffectInit(DiamondWeaveEffect *e,
                            const DiamondWeaveConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/diamond_weave.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->phaseAccumLoc = GetShaderLocation(e->shader, "phaseAccum");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->twistAccumLoc = GetShaderLocation(e->shader, "twistAccum");
  e->driftAccumLoc = GetShaderLocation(e->shader, "driftAccum");
  e->cellSizeLoc = GetShaderLocation(e->shader, "cellSize");
  e->baseAngleLoc = GetShaderLocation(e->shader, "baseAngle");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
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

  e->phaseAccum = 0.0f;
  e->rotationAccum = 0.0f;
  e->twistAccum = 0.0f;
  e->driftAccum = 0.0f;

  return true;
}

void DiamondWeaveEffectSetup(DiamondWeaveEffect *e,
                             const DiamondWeaveConfig *cfg, float deltaTime,
                             const Texture2D &fftTexture) {
  e->phaseAccum += cfg->phaseSpeed * deltaTime;
  e->rotationAccum += cfg->rotationSpeed * deltaTime;
  e->twistAccum += cfg->twistSpeed * deltaTime;
  e->driftAccum += cfg->driftSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseAccumLoc, &e->phaseAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAccumLoc, &e->twistAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftAccumLoc, &e->driftAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellSizeLoc, &cfg->cellSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseAngleLoc, &cfg->baseAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void DiamondWeaveEffectUninit(DiamondWeaveEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void DiamondWeaveRegisterParams(DiamondWeaveConfig *cfg) {
  ModEngineRegisterParam("diamondWeave.cellSize", &cfg->cellSize, 20.0f,
                         200.0f);
  ModEngineRegisterParam("diamondWeave.baseAngle", &cfg->baseAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("diamondWeave.phaseSpeed", &cfg->phaseSpeed, 0.05f,
                         1.0f);
  ModEngineRegisterParam("diamondWeave.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("diamondWeave.twistSpeed", &cfg->twistSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("diamondWeave.driftSpeed", &cfg->driftSpeed, 0.0f,
                         0.5f);
  ModEngineRegisterParam("diamondWeave.glowIntensity", &cfg->glowIntensity,
                         0.0f, 2.0f);
  ModEngineRegisterParam("diamondWeave.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("diamondWeave.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("diamondWeave.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("diamondWeave.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("diamondWeave.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("diamondWeave.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

DiamondWeaveEffect *GetDiamondWeaveEffect(PostEffect *pe) {
  return (DiamondWeaveEffect *)pe->effectStates[TRANSFORM_DIAMOND_WEAVE_BLEND];
}

void SetupDiamondWeave(PostEffect *pe) {
  DiamondWeaveEffectSetup(GetDiamondWeaveEffect(pe), &pe->effects.diamondWeave,
                          pe->currentDeltaTime, pe->fftTexture);
}

void SetupDiamondWeaveBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.diamondWeave.blendIntensity,
                       pe->effects.diamondWeave.blendMode);
}

// === UI ===

static void DrawDiamondWeaveParams(EffectConfig *e,
                                   const ModSources *modSources,
                                   ImU32 categoryGlow) {
  (void)categoryGlow;
  DiamondWeaveConfig *cfg = &e->diamondWeave;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##diamondWeave", &cfg->baseFreq,
                    "diamondWeave.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##diamondWeave", &cfg->maxFreq,
                    "diamondWeave.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##diamondWeave", &cfg->gain, "diamondWeave.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##diamondWeave", &cfg->curve, "diamondWeave.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##diamondWeave", &cfg->baseBright,
                    "diamondWeave.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Cell Size##diamondWeave", &cfg->cellSize,
                    "diamondWeave.cellSize", "%.1f", modSources);
  ModulatableSliderAngleDeg("Angle##diamondWeave", &cfg->baseAngle,
                            "diamondWeave.baseAngle", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Phase Speed##diamondWeave", &cfg->phaseSpeed,
                    "diamondWeave.phaseSpeed", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Rotation Speed##diamondWeave", &cfg->rotationSpeed,
                            "diamondWeave.rotationSpeed", modSources);
  ModulatableSliderSpeedDeg("Twist Speed##diamondWeave", &cfg->twistSpeed,
                            "diamondWeave.twistSpeed", modSources);
  ModulatableSlider("Drift Speed##diamondWeave", &cfg->driftSpeed,
                    "diamondWeave.driftSpeed", "%.2f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##diamondWeave", &cfg->glowIntensity,
                    "diamondWeave.glowIntensity", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(diamondWeave)
REGISTER_GENERATOR(TRANSFORM_DIAMOND_WEAVE_BLEND, DiamondWeave, diamondWeave,
                   "Diamond Weave", SetupDiamondWeaveBlend, SetupDiamondWeave,
                   12, DrawDiamondWeaveParams, DrawOutput_diamondWeave)
// clang-format on
