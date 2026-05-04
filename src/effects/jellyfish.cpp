// Jellyfish effect module implementation
// FFT-driven swarm of drifting bell silhouettes with pulsing rims, wavy
// tentacles, marine snow, and caustic backdrop

#include "jellyfish.h"
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

bool JellyfishEffectInit(JellyfishEffect *e, const JellyfishConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/jellyfish.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->pulsePhaseLoc = GetShaderLocation(e->shader, "pulsePhase");
  e->driftPhaseLoc = GetShaderLocation(e->shader, "driftPhase");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");

  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");

  e->jellyCountLoc = GetShaderLocation(e->shader, "jellyCount");
  e->sizeBaseLoc = GetShaderLocation(e->shader, "sizeBase");
  e->sizeVarianceLoc = GetShaderLocation(e->shader, "sizeVariance");
  e->pulseDepthLoc = GetShaderLocation(e->shader, "pulseDepth");
  e->driftAmpLoc = GetShaderLocation(e->shader, "driftAmp");
  e->tentacleWaveLoc = GetShaderLocation(e->shader, "tentacleWave");
  e->bellGlowLoc = GetShaderLocation(e->shader, "bellGlow");
  e->rimGlowLoc = GetShaderLocation(e->shader, "rimGlow");
  e->tentacleGlowLoc = GetShaderLocation(e->shader, "tentacleGlow");
  e->snowDensityLoc = GetShaderLocation(e->shader, "snowDensity");
  e->snowBrightnessLoc = GetShaderLocation(e->shader, "snowBrightness");
  e->causticIntensityLoc = GetShaderLocation(e->shader, "causticIntensity");
  e->backdropDepthLoc = GetShaderLocation(e->shader, "backdropDepth");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;
  e->pulsePhase = 0.0f;
  e->driftPhase = 0.0f;

  return true;
}

void JellyfishEffectSetup(JellyfishEffect *e, const JellyfishConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture) {
  e->time += deltaTime;
  e->pulsePhase += cfg->pulseSpeed * deltaTime;
  e->driftPhase += cfg->driftSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  float sampleRate = (float)AUDIO_SAMPLE_RATE;

  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pulsePhaseLoc, &e->pulsePhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftPhaseLoc, &e->driftPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->jellyCountLoc, &cfg->jellyCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->sizeBaseLoc, &cfg->sizeBase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sizeVarianceLoc, &cfg->sizeVariance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pulseDepthLoc, &cfg->pulseDepth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftAmpLoc, &cfg->driftAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tentacleWaveLoc, &cfg->tentacleWave,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bellGlowLoc, &cfg->bellGlow,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rimGlowLoc, &cfg->rimGlow, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tentacleGlowLoc, &cfg->tentacleGlow,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->snowDensityLoc, &cfg->snowDensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->snowBrightnessLoc, &cfg->snowBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->causticIntensityLoc, &cfg->causticIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->backdropDepthLoc, &cfg->backdropDepth,
                 SHADER_UNIFORM_FLOAT);
}

void JellyfishEffectUninit(JellyfishEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void JellyfishRegisterParams(JellyfishConfig *cfg) {
  ModEngineRegisterParam("jellyfish.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("jellyfish.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("jellyfish.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("jellyfish.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("jellyfish.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("jellyfish.sizeBase", &cfg->sizeBase, 0.02f, 0.10f);
  ModEngineRegisterParam("jellyfish.sizeVariance", &cfg->sizeVariance, 0.0f,
                         0.06f);
  ModEngineRegisterParam("jellyfish.pulseDepth", &cfg->pulseDepth, 0.0f, 0.4f);
  ModEngineRegisterParam("jellyfish.pulseSpeed", &cfg->pulseSpeed, 0.1f, 3.0f);
  ModEngineRegisterParam("jellyfish.driftAmp", &cfg->driftAmp, 0.0f, 3.0f);
  ModEngineRegisterParam("jellyfish.driftSpeed", &cfg->driftSpeed, 0.0f, 3.0f);
  ModEngineRegisterParam("jellyfish.tentacleWave", &cfg->tentacleWave, 0.0f,
                         0.025f);
  ModEngineRegisterParam("jellyfish.bellGlow", &cfg->bellGlow, 0.0f, 1.0f);
  ModEngineRegisterParam("jellyfish.rimGlow", &cfg->rimGlow, 0.0f, 1.0f);
  ModEngineRegisterParam("jellyfish.tentacleGlow", &cfg->tentacleGlow, 0.0f,
                         1.0f);
  ModEngineRegisterParam("jellyfish.snowDensity", &cfg->snowDensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("jellyfish.snowBrightness", &cfg->snowBrightness, 0.0f,
                         0.4f);
  ModEngineRegisterParam("jellyfish.causticIntensity", &cfg->causticIntensity,
                         0.0f, 0.6f);
  ModEngineRegisterParam("jellyfish.backdropDepth", &cfg->backdropDepth, 0.0f,
                         2.0f);
  ModEngineRegisterParam("jellyfish.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

JellyfishEffect *GetJellyfishEffect(PostEffect *pe) {
  return (JellyfishEffect *)pe->effectStates[TRANSFORM_JELLYFISH_BLEND];
}

void SetupJellyfish(PostEffect *pe) {
  JellyfishEffectSetup(GetJellyfishEffect(pe), &pe->effects.jellyfish,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupJellyfishBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.jellyfish.blendIntensity,
                       pe->effects.jellyfish.blendMode);
}

// === UI ===

static void DrawJellyfishParams(EffectConfig *e, const ModSources *modSources,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  JellyfishConfig *n = &e->jellyfish;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##jellyfish", &n->baseFreq,
                    "jellyfish.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##jellyfish", &n->maxFreq,
                    "jellyfish.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##jellyfish", &n->gain, "jellyfish.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##jellyfish", &n->curve, "jellyfish.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##jellyfish", &n->baseBright,
                    "jellyfish.baseBright", "%.2f", modSources);

  ImGui::SeparatorText("Swarm");
  ImGui::SliderInt("Jelly Count##jellyfish", &n->jellyCount, 1, 16);

  ImGui::SeparatorText("Bell");
  ModulatableSlider("Size Base##jellyfish", &n->sizeBase, "jellyfish.sizeBase",
                    "%.3f", modSources);
  ModulatableSlider("Size Variance##jellyfish", &n->sizeVariance,
                    "jellyfish.sizeVariance", "%.3f", modSources);
  ModulatableSlider("Pulse Depth##jellyfish", &n->pulseDepth,
                    "jellyfish.pulseDepth", "%.2f", modSources);
  ModulatableSlider("Pulse Speed##jellyfish", &n->pulseSpeed,
                    "jellyfish.pulseSpeed", "%.2f", modSources);

  ImGui::SeparatorText("Drift");
  ModulatableSlider("Drift Amp##jellyfish", &n->driftAmp, "jellyfish.driftAmp",
                    "%.2f", modSources);
  ModulatableSlider("Drift Speed##jellyfish", &n->driftSpeed,
                    "jellyfish.driftSpeed", "%.2f", modSources);

  ImGui::SeparatorText("Tentacles");
  ModulatableSlider("Tentacle Wave##jellyfish", &n->tentacleWave,
                    "jellyfish.tentacleWave", "%.4f", modSources);

  ImGui::SeparatorText("Glow");
  ModulatableSlider("Bell Glow##jellyfish", &n->bellGlow, "jellyfish.bellGlow",
                    "%.2f", modSources);
  ModulatableSlider("Rim Glow##jellyfish", &n->rimGlow, "jellyfish.rimGlow",
                    "%.2f", modSources);
  ModulatableSlider("Tentacle Glow##jellyfish", &n->tentacleGlow,
                    "jellyfish.tentacleGlow", "%.2f", modSources);

  ImGui::SeparatorText("Atmosphere");
  ModulatableSlider("Snow Density##jellyfish", &n->snowDensity,
                    "jellyfish.snowDensity", "%.2f", modSources);
  ModulatableSlider("Snow Bright##jellyfish", &n->snowBrightness,
                    "jellyfish.snowBrightness", "%.2f", modSources);
  ModulatableSlider("Caustics##jellyfish", &n->causticIntensity,
                    "jellyfish.causticIntensity", "%.2f", modSources);
  ModulatableSlider("Backdrop##jellyfish", &n->backdropDepth,
                    "jellyfish.backdropDepth", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(jellyfish)
REGISTER_GENERATOR(TRANSFORM_JELLYFISH_BLEND, Jellyfish, jellyfish, "Jellyfish",
                   SetupJellyfishBlend, SetupJellyfish, 15, DrawJellyfishParams, DrawOutput_jellyfish)
// clang-format on
