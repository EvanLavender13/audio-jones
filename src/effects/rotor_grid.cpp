// Rotor Grid generator - radial cell-grid with density and randomness controls,
// per-cell discrete coloring driven by FFT energy

#include "rotor_grid.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool RotorGridEffectInit(RotorGridEffect *e, const RotorGridConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/rotor_grid.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->densityLoc = GetShaderLocation(e->shader, "density");
  e->randomnessLoc = GetShaderLocation(e->shader, "randomness");
  e->ringSpacingLoc = GetShaderLocation(e->shader, "ringSpacing");
  e->baseDivisionsLoc = GetShaderLocation(e->shader, "baseDivisions");
  e->ringFrequencyLoc = GetShaderLocation(e->shader, "ringFrequency");
  e->spinPhaseLoc = GetShaderLocation(e->shader, "spinPhase");
  e->twistPhaseLoc = GetShaderLocation(e->shader, "twistPhase");
  e->driftPhaseLoc = GetShaderLocation(e->shader, "driftPhase");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->spinPhase = 0.0f;
  e->twistPhase = 0.0f;
  e->driftPhase = 0.0f;

  return true;
}

void RotorGridEffectSetup(RotorGridEffect *e, const RotorGridConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture) {
  e->spinPhase += cfg->spinSpeed * deltaTime;
  e->twistPhase += cfg->spinSpeed * cfg->differentialTwist * deltaTime;
  e->driftPhase += cfg->driftRate * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

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
  SetShaderValue(e->shader, e->densityLoc, &cfg->density, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->randomnessLoc, &cfg->randomness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ringSpacingLoc, &cfg->ringSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseDivisionsLoc, &cfg->baseDivisions,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->ringFrequencyLoc, &cfg->ringFrequency,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spinPhaseLoc, &e->spinPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistPhaseLoc, &e->twistPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftPhaseLoc, &e->driftPhase,
                 SHADER_UNIFORM_FLOAT);
}

void RotorGridEffectUninit(RotorGridEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void RotorGridRegisterParams(RotorGridConfig *cfg) {
  ModEngineRegisterParam("rotorGrid.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("rotorGrid.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("rotorGrid.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("rotorGrid.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("rotorGrid.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("rotorGrid.density", &cfg->density, 0.0f, 1.0f);
  ModEngineRegisterParam("rotorGrid.randomness", &cfg->randomness, 0.0f, 1.0f);
  ModEngineRegisterParam("rotorGrid.ringSpacing", &cfg->ringSpacing, 0.05f,
                         0.5f);
  ModEngineRegisterParam("rotorGrid.ringFrequency", &cfg->ringFrequency, 1.0f,
                         6.283f);
  ModEngineRegisterParam("rotorGrid.spinSpeed", &cfg->spinSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("rotorGrid.differentialTwist", &cfg->differentialTwist,
                         -2.0f, 2.0f);
  ModEngineRegisterParam("rotorGrid.driftRate", &cfg->driftRate, 0.0f, 1.0f);
  ModEngineRegisterParam("rotorGrid.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

RotorGridEffect *GetRotorGridEffect(PostEffect *pe) {
  return (RotorGridEffect *)pe->effectStates[TRANSFORM_ROTOR_GRID_BLEND];
}

void SetupRotorGrid(PostEffect *pe) {
  RotorGridEffectSetup(GetRotorGridEffect(pe), &pe->effects.rotorGrid,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupRotorGridBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.rotorGrid.blendIntensity,
                       pe->effects.rotorGrid.blendMode);
}

// === UI ===

static void DrawRotorGridParams(EffectConfig *e, const ModSources *modSources,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  RotorGridConfig *cfg = &e->rotorGrid;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##rotorgrid", &cfg->baseFreq,
                    "rotorGrid.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##rotorgrid", &cfg->maxFreq,
                    "rotorGrid.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##rotorgrid", &cfg->gain, "rotorGrid.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##rotorgrid", &cfg->curve, "rotorGrid.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##rotorgrid", &cfg->baseBright,
                    "rotorGrid.baseBright", "%.2f", modSources);

  // Cells
  ImGui::SeparatorText("Cells");
  ModulatableSlider("Density##rotorgrid", &cfg->density, "rotorGrid.density",
                    "%.2f", modSources);
  ModulatableSlider("Randomness##rotorgrid", &cfg->randomness,
                    "rotorGrid.randomness", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Ring Spacing##rotorgrid", &cfg->ringSpacing,
                    "rotorGrid.ringSpacing", "%.3f", modSources);
  ImGui::SliderInt("Base Divisions##rotorgrid", &cfg->baseDivisions, 1, 8);
  ModulatableSlider("Ring Frequency##rotorgrid", &cfg->ringFrequency,
                    "rotorGrid.ringFrequency", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Spin Speed##rotorgrid", &cfg->spinSpeed,
                            "rotorGrid.spinSpeed", modSources);
  ModulatableSlider("Differential Twist##rotorgrid", &cfg->differentialTwist,
                    "rotorGrid.differentialTwist", "%.2f", modSources);
  ModulatableSlider("Drift Rate##rotorgrid", &cfg->driftRate,
                    "rotorGrid.driftRate", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(rotorGrid)
REGISTER_GENERATOR(TRANSFORM_ROTOR_GRID_BLEND, RotorGrid, rotorGrid,
                   "Rotor Grid", SetupRotorGridBlend, SetupRotorGrid, 10,
                   DrawRotorGridParams, DrawOutput_rotorGrid)
// clang-format on
