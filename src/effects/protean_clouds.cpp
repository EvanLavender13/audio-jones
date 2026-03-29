// Protean Clouds effect module implementation
// Raymarched volumetric clouds with morphing noise and FFT-reactive density

#include "protean_clouds.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/dual_lissajous_config.h"
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

bool ProteanCloudsEffectInit(ProteanCloudsEffect *e,
                             const ProteanCloudsConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/protean_clouds.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->flyPhaseLoc = GetShaderLocation(e->shader, "flyPhase");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->morphLoc = GetShaderLocation(e->shader, "morph");
  e->colorBlendLoc = GetShaderLocation(e->shader, "colorBlend");
  e->fogIntensityLoc = GetShaderLocation(e->shader, "fogIntensity");
  e->turbulenceLoc = GetShaderLocation(e->shader, "turbulence");
  e->densityThresholdLoc = GetShaderLocation(e->shader, "densityThreshold");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->rollAngleLoc = GetShaderLocation(e->shader, "rollAngle");
  e->driftAmplitudeLoc = GetShaderLocation(e->shader, "driftAmplitude");
  e->driftFreqX1Loc = GetShaderLocation(e->shader, "driftFreqX1");
  e->driftFreqY1Loc = GetShaderLocation(e->shader, "driftFreqY1");
  e->driftFreqX2Loc = GetShaderLocation(e->shader, "driftFreqX2");
  e->driftFreqY2Loc = GetShaderLocation(e->shader, "driftFreqY2");
  e->driftOffsetX2Loc = GetShaderLocation(e->shader, "driftOffsetX2");
  e->driftOffsetY2Loc = GetShaderLocation(e->shader, "driftOffsetY2");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;
  e->flyPhase = 0.0f;
  e->rollAngle = 0.0f;

  return true;
}

static void BindUniforms(ProteanCloudsEffect *e,
                         const ProteanCloudsConfig *cfg) {
  SetShaderValue(e->shader, e->morphLoc, &cfg->morph, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->turbulenceLoc, &cfg->turbulence,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->densityThresholdLoc, &cfg->densityThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->colorBlendLoc, &cfg->colorBlend,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fogIntensityLoc, &cfg->fogIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->rollAngleLoc, &e->rollAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftAmplitudeLoc, &cfg->drift.amplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftFreqX1Loc, &cfg->drift.freqX1,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftFreqY1Loc, &cfg->drift.freqY1,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftFreqX2Loc, &cfg->drift.freqX2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftFreqY2Loc, &cfg->drift.freqY2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftOffsetX2Loc, &cfg->drift.offsetX2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftOffsetY2Loc, &cfg->drift.offsetY2,
                 SHADER_UNIFORM_FLOAT);
}

void ProteanCloudsEffectSetup(ProteanCloudsEffect *e,
                              const ProteanCloudsConfig *cfg, float deltaTime,
                              const Texture2D &fftTexture) {
  e->time += deltaTime;
  e->flyPhase += cfg->speed * deltaTime;
  e->rollAngle += cfg->rollSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flyPhaseLoc, &e->flyPhase, SHADER_UNIFORM_FLOAT);

  BindUniforms(e, cfg);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void ProteanCloudsEffectUninit(ProteanCloudsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void ProteanCloudsRegisterParams(ProteanCloudsConfig *cfg) {
  ModEngineRegisterParam("proteanClouds.speed", &cfg->speed, 0.5f, 6.0f);
  ModEngineRegisterParam("proteanClouds.morph", &cfg->morph, 0.0f, 1.0f);
  ModEngineRegisterParam("proteanClouds.turbulence", &cfg->turbulence, 0.05f,
                         0.5f);
  ModEngineRegisterParam("proteanClouds.densityThreshold",
                         &cfg->densityThreshold, 0.0f, 1.0f);
  ModEngineRegisterParam("proteanClouds.colorBlend", &cfg->colorBlend, 0.0f,
                         1.0f);
  ModEngineRegisterParam("proteanClouds.fogIntensity", &cfg->fogIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("proteanClouds.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("proteanClouds.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("proteanClouds.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("proteanClouds.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("proteanClouds.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("proteanClouds.rollSpeed", &cfg->rollSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("proteanClouds.drift.amplitude", &cfg->drift.amplitude,
                         0.0f, 4.0f);
  ModEngineRegisterParam("proteanClouds.drift.motionSpeed",
                         &cfg->drift.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("proteanClouds.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupProteanClouds(PostEffect *pe) {
  ProteanCloudsEffectSetup(&pe->proteanClouds, &pe->effects.proteanClouds,
                           pe->currentDeltaTime, pe->fftTexture);
}

void SetupProteanCloudsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.proteanClouds.blendIntensity,
                       pe->effects.proteanClouds.blendMode);
}

// === UI ===

static void DrawProteanCloudsParams(EffectConfig *e,
                                    const ModSources *modSources,
                                    ImU32 categoryGlow) {
  (void)categoryGlow;
  ProteanCloudsConfig *pc = &e->proteanClouds;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##proteanClouds", &pc->baseFreq,
                    "proteanClouds.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##proteanClouds", &pc->maxFreq,
                    "proteanClouds.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##proteanClouds", &pc->gain, "proteanClouds.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##proteanClouds", &pc->curve,
                    "proteanClouds.curve", "%.2f", modSources);
  ModulatableSlider("Base Bright##proteanClouds", &pc->baseBright,
                    "proteanClouds.baseBright", "%.2f", modSources);

  // Volume
  ImGui::SeparatorText("Volume");
  ImGui::SliderInt("Octaves##proteanClouds", &pc->octaves, 1, 8);
  ImGui::SliderInt("March Steps##proteanClouds", &pc->marchSteps, 40, 130);
  ModulatableSlider("Morph##proteanClouds", &pc->morph, "proteanClouds.morph",
                    "%.2f", modSources);
  ModulatableSlider("Turbulence##proteanClouds", &pc->turbulence,
                    "proteanClouds.turbulence", "%.2f", modSources);
  ModulatableSlider("Density##proteanClouds", &pc->densityThreshold,
                    "proteanClouds.densityThreshold", "%.2f", modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  DrawLissajousControls(&pc->drift, "pc_drift", "proteanClouds.drift",
                        modSources, 1.0f);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Speed##proteanClouds", &pc->speed, "proteanClouds.speed",
                    "%.1f", modSources);
  ModulatableSliderSpeedDeg("Roll##proteanClouds", &pc->rollSpeed,
                            "proteanClouds.rollSpeed", modSources);

  // Color
  ImGui::SeparatorText("Color");
  ModulatableSlider("Color Blend##proteanClouds", &pc->colorBlend,
                    "proteanClouds.colorBlend", "%.2f", modSources);
  ModulatableSlider("Fog##proteanClouds", &pc->fogIntensity,
                    "proteanClouds.fogIntensity", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(proteanClouds)
REGISTER_GENERATOR(TRANSFORM_PROTEAN_CLOUDS_BLEND, ProteanClouds, proteanClouds,
                   "Protean Clouds", SetupProteanCloudsBlend, SetupProteanClouds,
                   13, DrawProteanCloudsParams, DrawOutput_proteanClouds)
// clang-format on
