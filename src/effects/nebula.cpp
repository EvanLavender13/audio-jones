// Nebula effect module implementation
// FFT-driven procedural nebula clouds with fractal layers, semitone-reactive
// stars, sinusoidal drift, and gradient coloring

#include "nebula.h"
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
#include <stddef.h>

bool NebulaEffectInit(NebulaEffect *e, const NebulaConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/nebula.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->starBinsLoc = GetShaderLocation(e->shader, "starBins");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");

  e->frontScaleLoc = GetShaderLocation(e->shader, "frontScale");
  e->midScaleLoc = GetShaderLocation(e->shader, "midScale");
  e->backScaleLoc = GetShaderLocation(e->shader, "backScale");
  e->frontIterLoc = GetShaderLocation(e->shader, "frontIter");
  e->midIterLoc = GetShaderLocation(e->shader, "midIter");
  e->backIterLoc = GetShaderLocation(e->shader, "backIter");
  e->starDensityLoc = GetShaderLocation(e->shader, "starDensity");
  e->starSharpnessLoc = GetShaderLocation(e->shader, "starSharpness");
  e->glowWidthLoc = GetShaderLocation(e->shader, "glowWidth");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->noiseTypeLoc = GetShaderLocation(e->shader, "noiseType");
  e->dustScaleLoc = GetShaderLocation(e->shader, "dustScale");
  e->dustStrengthLoc = GetShaderLocation(e->shader, "dustStrength");
  e->dustEdgeLoc = GetShaderLocation(e->shader, "dustEdge");
  e->spikeIntensityLoc = GetShaderLocation(e->shader, "spikeIntensity");
  e->spikeSharpnessLoc = GetShaderLocation(e->shader, "spikeSharpness");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

void NebulaEffectSetup(NebulaEffect *e, const NebulaConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture) {
  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  e->time += cfg->driftSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starBinsLoc, &cfg->starBins, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->frontScaleLoc, &cfg->frontScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->midScaleLoc, &cfg->midScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->backScaleLoc, &cfg->backScale,
                 SHADER_UNIFORM_FLOAT);
  const int *fi = cfg->noiseType == 1 ? &cfg->fbmFrontOct : &cfg->frontIter;
  const int *mi = cfg->noiseType == 1 ? &cfg->fbmMidOct : &cfg->midIter;
  const int *bi = cfg->noiseType == 1 ? &cfg->fbmBackOct : &cfg->backIter;
  SetShaderValue(e->shader, e->frontIterLoc, fi, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->midIterLoc, mi, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->backIterLoc, bi, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->starDensityLoc, &cfg->starDensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starSharpnessLoc, &cfg->starSharpness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowWidthLoc, &cfg->glowWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseTypeLoc, &cfg->noiseType,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->dustScaleLoc, &cfg->dustScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dustStrengthLoc, &cfg->dustStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dustEdgeLoc, &cfg->dustEdge,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spikeIntensityLoc, &cfg->spikeIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spikeSharpnessLoc, &cfg->spikeSharpness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void NebulaEffectUninit(NebulaEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void NebulaRegisterParams(NebulaConfig *cfg) {
  ModEngineRegisterParam("nebula.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("nebula.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("nebula.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("nebula.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("nebula.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("nebula.driftSpeed", &cfg->driftSpeed, 0.01f, 5.0f);
  ModEngineRegisterParam("nebula.frontScale", &cfg->frontScale, 1.0f, 8.0f);
  ModEngineRegisterParam("nebula.midScale", &cfg->midScale, 1.0f, 10.0f);
  ModEngineRegisterParam("nebula.backScale", &cfg->backScale, 2.0f, 12.0f);
  ModEngineRegisterParam("nebula.starDensity", &cfg->starDensity, 100.0f,
                         800.0f);
  ModEngineRegisterParam("nebula.starSharpness", &cfg->starSharpness, 10.0f,
                         60.0f);
  ModEngineRegisterParam("nebula.glowWidth", &cfg->glowWidth, 0.05f, 0.3f);
  ModEngineRegisterParam("nebula.glowIntensity", &cfg->glowIntensity, 0.5f,
                         10.0f);
  ModEngineRegisterParam("nebula.dustScale", &cfg->dustScale, 1.0f, 8.0f);
  ModEngineRegisterParam("nebula.dustStrength", &cfg->dustStrength, 0.0f, 1.0f);
  ModEngineRegisterParam("nebula.dustEdge", &cfg->dustEdge, 0.05f, 0.3f);
  ModEngineRegisterParam("nebula.spikeIntensity", &cfg->spikeIntensity, 0.0f,
                         2.0f);
  ModEngineRegisterParam("nebula.spikeSharpness", &cfg->spikeSharpness, 5.0f,
                         40.0f);
  ModEngineRegisterParam("nebula.brightness", &cfg->brightness, 0.5f, 3.0f);
  ModEngineRegisterParam("nebula.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupNebula(PostEffect *pe) {
  NebulaEffectSetup(&pe->nebula, &pe->effects.nebula, pe->currentDeltaTime,
                    pe->fftTexture);
}

void SetupNebulaBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.nebula.blendIntensity,
                       pe->effects.nebula.blendMode);
}

// === UI ===

static void DrawNebulaParams(EffectConfig *e, const ModSources *modSources,
                             ImU32 categoryGlow) {
  (void)categoryGlow;
  NebulaConfig *n = &e->nebula;

  // FFT
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##nebula", &n->baseFreq, "nebula.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##nebula", &n->maxFreq, "nebula.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##nebula", &n->gain, "nebula.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##nebula", &n->curve, "nebula.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##nebula", &n->baseBright, "nebula.baseBright",
                    "%.2f", modSources);

  // Layers
  ImGui::SeparatorText("Layers");
  ImGui::Combo("Noise Type##nebula", &n->noiseType, "Kaliset\0FBM\0");
  ModulatableSlider("Front Scale##nebula", &n->frontScale, "nebula.frontScale",
                    "%.1f", modSources);
  ModulatableSlider("Mid Scale##nebula", &n->midScale, "nebula.midScale",
                    "%.1f", modSources);
  ModulatableSlider("Back Scale##nebula", &n->backScale, "nebula.backScale",
                    "%.1f", modSources);
  if (n->noiseType == 1) {
    ImGui::SliderInt("Front Octaves##nebula", &n->fbmFrontOct, 2, 8);
    ImGui::SliderInt("Mid Octaves##nebula", &n->fbmMidOct, 2, 8);
    ImGui::SliderInt("Back Octaves##nebula", &n->fbmBackOct, 2, 8);
  } else {
    ImGui::SliderInt("Front Iterations##nebula", &n->frontIter, 6, 40);
    ImGui::SliderInt("Mid Iterations##nebula", &n->midIter, 6, 40);
    ImGui::SliderInt("Back Iterations##nebula", &n->backIter, 6, 40);
  }

  // Dust
  ImGui::SeparatorText("Dust");
  ModulatableSlider("Dust Scale##nebula", &n->dustScale, "nebula.dustScale",
                    "%.1f", modSources);
  ModulatableSlider("Dust Strength##nebula", &n->dustStrength,
                    "nebula.dustStrength", "%.2f", modSources);
  ModulatableSlider("Dust Edge##nebula", &n->dustEdge, "nebula.dustEdge",
                    "%.2f", modSources);

  // Stars
  ImGui::SeparatorText("Stars");
  ImGui::SliderInt("Star Bins##nebula", &n->starBins, 12, 120);
  ModulatableSlider("Star Density##nebula", &n->starDensity,
                    "nebula.starDensity", "%.0f", modSources);
  ModulatableSlider("Star Rarity##nebula", &n->starSharpness,
                    "nebula.starSharpness", "%.1f", modSources);
  ModulatableSlider("Glow Width##nebula", &n->glowWidth, "nebula.glowWidth",
                    "%.2f", modSources);
  ModulatableSlider("Glow Intensity##nebula", &n->glowIntensity,
                    "nebula.glowIntensity", "%.1f", modSources);

  // Spikes
  ImGui::SeparatorText("Spikes");
  ModulatableSlider("Spike Intensity##nebula", &n->spikeIntensity,
                    "nebula.spikeIntensity", "%.2f", modSources);
  ModulatableSlider("Spike Sharpness##nebula", &n->spikeSharpness,
                    "nebula.spikeSharpness", "%.1f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Drift Speed##nebula", &n->driftSpeed, "nebula.driftSpeed",
                    "%.3f", modSources);

  // Brightness (effect-specific, not part of standard output)
  ModulatableSlider("Brightness##nebula", &n->brightness, "nebula.brightness",
                    "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(nebula)
REGISTER_GENERATOR(TRANSFORM_NEBULA_BLEND, Nebula, nebula, "Nebula",
                   SetupNebulaBlend, SetupNebula, 13, DrawNebulaParams, DrawOutput_nebula)
// clang-format on
