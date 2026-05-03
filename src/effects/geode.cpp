// Geode effect module implementation
// Raymarched cluster of cubes carved by a gyroid/noise cut field with
// FFT-reactive coloring

#include "geode.h"
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

bool GeodeEffectInit(GeodeEffect *e, const GeodeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/geode.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->frameLoc = GetShaderLocation(e->shader, "frame");

  e->cutModeLoc = GetShaderLocation(e->shader, "cutMode");
  e->cutScaleLoc = GetShaderLocation(e->shader, "cutScale");
  e->cutThresholdBaseLoc = GetShaderLocation(e->shader, "cutThresholdBase");
  e->cutThresholdPulseLoc = GetShaderLocation(e->shader, "cutThresholdPulse");
  e->cutPulsePhaseLoc = GetShaderLocation(e->shader, "cutPulsePhase");
  e->fieldOffsetLoc = GetShaderLocation(e->shader, "fieldOffset");
  e->clusterRadiusLoc = GetShaderLocation(e->shader, "clusterRadius");
  e->cubeSizeLoc = GetShaderLocation(e->shader, "cubeSize");
  e->colorRateLoc = GetShaderLocation(e->shader, "colorRate");

  e->orbitPhaseLoc = GetShaderLocation(e->shader, "orbitPhase");
  e->orbitPitchLoc = GetShaderLocation(e->shader, "orbitPitch");
  e->cameraDistanceLoc = GetShaderLocation(e->shader, "cameraDistance");

  e->ambientLoc = GetShaderLocation(e->shader, "ambient");
  e->specularPowerLoc = GetShaderLocation(e->shader, "specularPower");
  e->fogDistanceLoc = GetShaderLocation(e->shader, "fogDistance");

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

  e->cutPulsePhase = 0.0f;
  e->orbitPhase = 0.0f;
  e->fieldOffsetX = 0.0f;
  e->fieldOffsetY = 0.0f;
  e->fieldOffsetZ = 0.0f;
  e->frame = 0;

  return true;
}

static void BindUniforms(const GeodeEffect *e, const GeodeConfig *cfg) {
  SetShaderValue(e->shader, e->cutModeLoc, &cfg->cutMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->cutScaleLoc, &cfg->cutScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cutThresholdBaseLoc, &cfg->cutThresholdBase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cutThresholdPulseLoc, &cfg->cutThresholdPulse,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->clusterRadiusLoc, &cfg->clusterRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cubeSizeLoc, &cfg->cubeSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorRateLoc, &cfg->colorRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitPitchLoc, &cfg->orbitPitch,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraDistanceLoc, &cfg->cameraDistance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ambientLoc, &cfg->ambient, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->specularPowerLoc, &cfg->specularPower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fogDistanceLoc, &cfg->fogDistance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void GeodeEffectSetup(GeodeEffect *e, const GeodeConfig *cfg, float deltaTime,
                      const Texture2D &fftTexture) {
  e->cutPulsePhase += cfg->cutPulseSpeed * deltaTime;
  e->orbitPhase += cfg->orbitSpeed * deltaTime;
  e->fieldOffsetX += cfg->fieldDriftX * deltaTime;
  e->fieldOffsetY += cfg->fieldDriftY * deltaTime;
  e->fieldOffsetZ += cfg->fieldDriftZ * deltaTime;
  e->frame++;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->frameLoc, &e->frame, SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->cutPulsePhaseLoc, &e->cutPulsePhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitPhaseLoc, &e->orbitPhase,
                 SHADER_UNIFORM_FLOAT);
  const float fieldOffset[3] = {e->fieldOffsetX, e->fieldOffsetY,
                                e->fieldOffsetZ};
  SetShaderValue(e->shader, e->fieldOffsetLoc, fieldOffset,
                 SHADER_UNIFORM_VEC3);

  BindUniforms(e, cfg);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void GeodeEffectUninit(GeodeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void GeodeRegisterParams(GeodeConfig *cfg) {
  ModEngineRegisterParam("geode.clusterRadius", &cfg->clusterRadius, 5.0f,
                         60.0f);
  ModEngineRegisterParam("geode.cubeSize", &cfg->cubeSize, 0.3f, 0.7f);
  ModEngineRegisterParam("geode.colorRate", &cfg->colorRate, 0.02f, 0.5f);
  ModEngineRegisterParam("geode.cutScale", &cfg->cutScale, 0.02f, 0.25f);
  ModEngineRegisterParam("geode.cutThresholdBase", &cfg->cutThresholdBase,
                         -2.0f, 2.0f);
  ModEngineRegisterParam("geode.cutThresholdPulse", &cfg->cutThresholdPulse,
                         0.0f, 2.0f);
  ModEngineRegisterParam("geode.cutPulseSpeed", &cfg->cutPulseSpeed, 0.0f,
                         4.0f);
  ModEngineRegisterParam("geode.fieldDriftX", &cfg->fieldDriftX, -2.0f, 2.0f);
  ModEngineRegisterParam("geode.fieldDriftY", &cfg->fieldDriftY, -2.0f, 2.0f);
  ModEngineRegisterParam("geode.fieldDriftZ", &cfg->fieldDriftZ, -2.0f, 2.0f);
  ModEngineRegisterParam("geode.orbitSpeed", &cfg->orbitSpeed, -1.0f, 1.0f);
  ModEngineRegisterParam("geode.orbitPitch", &cfg->orbitPitch,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("geode.cameraDistance", &cfg->cameraDistance, 30.0f,
                         80.0f);
  ModEngineRegisterParam("geode.ambient", &cfg->ambient, 0.0f, 0.5f);
  ModEngineRegisterParam("geode.specularPower", &cfg->specularPower, 10.0f,
                         1000.0f);
  ModEngineRegisterParam("geode.fogDistance", &cfg->fogDistance, 10.0f, 100.0f);
  ModEngineRegisterParam("geode.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("geode.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("geode.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("geode.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("geode.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("geode.blendIntensity", &cfg->blendIntensity, 0.0f,
                         1.0f);
}

GeodeEffect *GetGeodeEffect(PostEffect *pe) {
  return (GeodeEffect *)pe->effectStates[TRANSFORM_GEODE_BLEND];
}

void SetupGeode(PostEffect *pe) {
  GeodeEffectSetup(GetGeodeEffect(pe), &pe->effects.geode, pe->currentDeltaTime,
                   pe->fftTexture);
}

void SetupGeodeBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.geode.blendIntensity,
                       pe->effects.geode.blendMode);
}

// === UI ===

static void DrawGeodeParams(EffectConfig *e, const ModSources *modSources,
                            ImU32 categoryGlow) {
  (void)categoryGlow;
  GeodeConfig *g = &e->geode;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##geode", &g->baseFreq, "geode.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##geode", &g->maxFreq, "geode.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##geode", &g->gain, "geode.gain", "%.1f", modSources);
  ModulatableSlider("Contrast##geode", &g->curve, "geode.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##geode", &g->baseBright, "geode.baseBright",
                    "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Cluster Radius##geode", &g->clusterRadius,
                    "geode.clusterRadius", "%.2f", modSources);
  ModulatableSlider("Cube Size##geode", &g->cubeSize, "geode.cubeSize", "%.3f",
                    modSources);
  ModulatableSlider("Color Rate##geode", &g->colorRate, "geode.colorRate",
                    "%.3f", modSources);

  // Cut Field
  ImGui::SeparatorText("Cut Field");
  ImGui::Combo("Cut Mode##geode", &g->cutMode, "Gyroid\0Noise\0");
  ModulatableSlider("Cut Scale##geode", &g->cutScale, "geode.cutScale", "%.3f",
                    modSources);
  ModulatableSlider("Threshold##geode", &g->cutThresholdBase,
                    "geode.cutThresholdBase", "%.2f", modSources);
  ModulatableSlider("Pulse Amount##geode", &g->cutThresholdPulse,
                    "geode.cutThresholdPulse", "%.2f", modSources);
  ModulatableSlider("Pulse Speed##geode", &g->cutPulseSpeed,
                    "geode.cutPulseSpeed", "%.2f", modSources);
  ModulatableSlider("Drift X##geode", &g->fieldDriftX, "geode.fieldDriftX",
                    "%.2f", modSources);
  ModulatableSlider("Drift Y##geode", &g->fieldDriftY, "geode.fieldDriftY",
                    "%.2f", modSources);
  ModulatableSlider("Drift Z##geode", &g->fieldDriftZ, "geode.fieldDriftZ",
                    "%.2f", modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  ModulatableSlider("Orbit Speed##geode", &g->orbitSpeed, "geode.orbitSpeed",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Pitch##geode", &g->orbitPitch, "geode.orbitPitch",
                            modSources);
  ModulatableSlider("Distance##geode", &g->cameraDistance,
                    "geode.cameraDistance", "%.1f", modSources);

  // Lighting
  ImGui::SeparatorText("Lighting");
  ModulatableSlider("Ambient##geode", &g->ambient, "geode.ambient", "%.2f",
                    modSources);
  ModulatableSlider("Specular Power##geode", &g->specularPower,
                    "geode.specularPower", "%.1f", modSources);
  ModulatableSlider("Fog Distance##geode", &g->fogDistance, "geode.fogDistance",
                    "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(geode)
REGISTER_GENERATOR(TRANSFORM_GEODE_BLEND, Geode, geode, "Geode",
                   SetupGeodeBlend, SetupGeode, 17,
                   DrawGeodeParams, DrawOutput_geode)
// clang-format on
