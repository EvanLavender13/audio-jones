// Apollonian Tunnel effect module implementation
// Raymarched Apollonian-gasket fractal carved into a sinusoidal tunnel with
// FFT-driven volumetric glow

#include "apollonian_tunnel.h"
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

bool ApollonianTunnelEffectInit(ApollonianTunnelEffect *e,
                                const ApollonianTunnelConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/apollonian_tunnel.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->flyPhaseLoc = GetShaderLocation(e->shader, "flyPhase");
  e->rollPhaseLoc = GetShaderLocation(e->shader, "rollPhase");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->fractalItersLoc = GetShaderLocation(e->shader, "fractalIters");
  e->preScaleLoc = GetShaderLocation(e->shader, "preScale");
  e->verticalOffsetLoc = GetShaderLocation(e->shader, "verticalOffset");
  e->tunnelRadiusLoc = GetShaderLocation(e->shader, "tunnelRadius");
  e->pathAmplitudeLoc = GetShaderLocation(e->shader, "pathAmplitude");
  e->pathFreqLoc = GetShaderLocation(e->shader, "pathFreq");
  e->rollAmountLoc = GetShaderLocation(e->shader, "rollAmount");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->depthCycleLoc = GetShaderLocation(e->shader, "depthCycle");
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

  e->flyPhase = 0.0f;
  e->rollPhase = 0.0f;

  return true;
}

static void BindUniforms(const ApollonianTunnelEffect *e,
                         const ApollonianTunnelConfig *cfg) {
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->fractalItersLoc, &cfg->fractalIters,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->preScaleLoc, &cfg->preScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->verticalOffsetLoc, &cfg->verticalOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tunnelRadiusLoc, &cfg->tunnelRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pathAmplitudeLoc, &cfg->pathAmplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pathFreqLoc, &cfg->pathFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rollAmountLoc, &cfg->rollAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->depthCycleLoc, &cfg->depthCycle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void ApollonianTunnelEffectSetup(ApollonianTunnelEffect *e,
                                 ApollonianTunnelConfig *cfg, float deltaTime,
                                 const Texture2D &fftTexture) {
  e->flyPhase += cfg->flySpeed * deltaTime;
  e->rollPhase += cfg->rollSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flyPhaseLoc, &e->flyPhase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rollPhaseLoc, &e->rollPhase,
                 SHADER_UNIFORM_FLOAT);

  BindUniforms(e, cfg);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void ApollonianTunnelEffectUninit(ApollonianTunnelEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void ApollonianTunnelRegisterParams(ApollonianTunnelConfig *cfg) {
  ModEngineRegisterParam("apollonianTunnel.preScale", &cfg->preScale, 4.0f,
                         32.0f);
  ModEngineRegisterParam("apollonianTunnel.verticalOffset",
                         &cfg->verticalOffset, -4.0f, 4.0f);
  ModEngineRegisterParam("apollonianTunnel.tunnelRadius", &cfg->tunnelRadius,
                         0.5f, 4.0f);
  ModEngineRegisterParam("apollonianTunnel.pathAmplitude", &cfg->pathAmplitude,
                         4.0f, 32.0f);
  ModEngineRegisterParam("apollonianTunnel.pathFreq", &cfg->pathFreq, 0.01f,
                         0.2f);
  ModEngineRegisterParam("apollonianTunnel.flySpeed", &cfg->flySpeed, 0.0f,
                         5.0f);
  ModEngineRegisterParam("apollonianTunnel.rollSpeed", &cfg->rollSpeed, -2.0f,
                         2.0f);
  ModEngineRegisterParam("apollonianTunnel.rollAmount", &cfg->rollAmount, 0.0f,
                         1.0f);
  ModEngineRegisterParam("apollonianTunnel.glowIntensity", &cfg->glowIntensity,
                         0.0f, 5.0f);
  ModEngineRegisterParam("apollonianTunnel.depthCycle", &cfg->depthCycle, 2.0f,
                         50.0f);
  ModEngineRegisterParam("apollonianTunnel.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("apollonianTunnel.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("apollonianTunnel.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("apollonianTunnel.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("apollonianTunnel.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("apollonianTunnel.blendIntensity",
                         &cfg->blendIntensity, 0.0f, 5.0f);
}

ApollonianTunnelEffect *GetApollonianTunnelEffect(PostEffect *pe) {
  return (ApollonianTunnelEffect *)
      pe->effectStates[TRANSFORM_APOLLONIAN_TUNNEL_BLEND];
}

void SetupApollonianTunnel(PostEffect *pe) {
  ApollonianTunnelEffectSetup(GetApollonianTunnelEffect(pe),
                              &pe->effects.apollonianTunnel,
                              pe->currentDeltaTime, pe->fftTexture);
}

void SetupApollonianTunnelBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.apollonianTunnel.blendIntensity,
                       pe->effects.apollonianTunnel.blendMode);
}

// === UI ===

static void DrawApollonianTunnelParams(EffectConfig *e,
                                       const ModSources *modSources,
                                       ImU32 categoryGlow) {
  (void)categoryGlow;
  ApollonianTunnelConfig *at = &e->apollonianTunnel;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##apollonianTunnel", &at->baseFreq,
                    "apollonianTunnel.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##apollonianTunnel", &at->maxFreq,
                    "apollonianTunnel.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##apollonianTunnel", &at->gain,
                    "apollonianTunnel.gain", "%.1f", modSources);
  ModulatableSlider("Contrast##apollonianTunnel", &at->curve,
                    "apollonianTunnel.curve", "%.2f", modSources);
  ModulatableSlider("Base Bright##apollonianTunnel", &at->baseBright,
                    "apollonianTunnel.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("March Steps##apollonianTunnel", &at->marchSteps, 48, 128);
  ImGui::SliderInt("Iterations##apollonianTunnel", &at->fractalIters, 3, 10);
  ModulatableSlider("Pre Scale##apollonianTunnel", &at->preScale,
                    "apollonianTunnel.preScale", "%.2f", modSources);
  ModulatableSlider("Vertical Offset##apollonianTunnel", &at->verticalOffset,
                    "apollonianTunnel.verticalOffset", "%.2f", modSources);
  ModulatableSlider("Tunnel Radius##apollonianTunnel", &at->tunnelRadius,
                    "apollonianTunnel.tunnelRadius", "%.2f", modSources);

  // Tunnel
  ImGui::SeparatorText("Tunnel");
  ModulatableSlider("Path Amp##apollonianTunnel", &at->pathAmplitude,
                    "apollonianTunnel.pathAmplitude", "%.2f", modSources);
  ModulatableSlider("Path Freq##apollonianTunnel", &at->pathFreq,
                    "apollonianTunnel.pathFreq", "%.3f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Fly Speed##apollonianTunnel", &at->flySpeed,
                    "apollonianTunnel.flySpeed", "%.2f", modSources);
  ModulatableSlider("Roll Speed##apollonianTunnel", &at->rollSpeed,
                    "apollonianTunnel.rollSpeed", "%.2f", modSources);
  ModulatableSlider("Roll Amount##apollonianTunnel", &at->rollAmount,
                    "apollonianTunnel.rollAmount", "%.2f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##apollonianTunnel", &at->glowIntensity,
                    "apollonianTunnel.glowIntensity", "%.2f", modSources);
  ModulatableSlider("Depth Cycle##apollonianTunnel", &at->depthCycle,
                    "apollonianTunnel.depthCycle", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(apollonianTunnel)
REGISTER_GENERATOR(TRANSFORM_APOLLONIAN_TUNNEL_BLEND, ApollonianTunnel, apollonianTunnel, "Apollonian Tunnel",
                   SetupApollonianTunnelBlend, SetupApollonianTunnel, 13, DrawApollonianTunnelParams, DrawOutput_apollonianTunnel)
// clang-format on
