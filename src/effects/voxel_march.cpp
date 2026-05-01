// Voxel March effect module implementation
// Raymarched voxelized sphere shells with FFT-reactive highlights and
// gradient coloring

#include "voxel_march.h"
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

bool VoxelMarchEffectInit(VoxelMarchEffect *e, const VoxelMarchConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/voxel_march.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->flyPhaseLoc = GetShaderLocation(e->shader, "flyPhase");
  e->gridPhaseLoc = GetShaderLocation(e->shader, "gridPhase");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->stepSizeLoc = GetShaderLocation(e->shader, "stepSize");
  e->voxelScaleLoc = GetShaderLocation(e->shader, "voxelScale");
  e->voxelVariationLoc = GetShaderLocation(e->shader, "voxelVariation");
  e->cellSizeLoc = GetShaderLocation(e->shader, "cellSize");
  e->shellRadiusLoc = GetShaderLocation(e->shader, "shellRadius");
  e->surfaceShapeLoc = GetShaderLocation(e->shader, "surfaceShape");
  e->domainFoldLoc = GetShaderLocation(e->shader, "domainFold");
  e->boundaryWaveLoc = GetShaderLocation(e->shader, "boundaryWave");
  e->surfaceCountLoc = GetShaderLocation(e->shader, "surfaceCount");
  e->highlightIntensityLoc = GetShaderLocation(e->shader, "highlightIntensity");
  e->positionTintLoc = GetShaderLocation(e->shader, "positionTint");
  e->tonemapGainLoc = GetShaderLocation(e->shader, "tonemapGain");
  e->panLoc = GetShaderLocation(e->shader, "pan");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->colorFreqMapLoc = GetShaderLocation(e->shader, "colorFreqMap");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->flyPhase = 0.0f;
  e->gridPhase = 0.0f;

  return true;
}

static void BindUniforms(const VoxelMarchEffect *e,
                         const VoxelMarchConfig *cfg) {
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->stepSizeLoc, &cfg->stepSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->voxelScaleLoc, &cfg->voxelScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->voxelVariationLoc, &cfg->voxelVariation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellSizeLoc, &cfg->cellSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shellRadiusLoc, &cfg->shellRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->surfaceShapeLoc, &cfg->surfaceShape,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->domainFoldLoc, &cfg->domainFold,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->boundaryWaveLoc, &cfg->boundaryWave,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->surfaceCountLoc, &cfg->surfaceCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->highlightIntensityLoc, &cfg->highlightIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->positionTintLoc, &cfg->positionTint,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tonemapGainLoc, &cfg->tonemapGain,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  const int colorFreqMapInt = cfg->colorFreqMap ? 1 : 0;
  SetShaderValue(e->shader, e->colorFreqMapLoc, &colorFreqMapInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void VoxelMarchEffectSetup(VoxelMarchEffect *e, VoxelMarchConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture) {
  e->flyPhase += cfg->flySpeed * deltaTime;
  e->gridPhase += cfg->gridAnimSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flyPhaseLoc, &e->flyPhase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gridPhaseLoc, &e->gridPhase,
                 SHADER_UNIFORM_FLOAT);

  BindUniforms(e, cfg);

  float panX;
  float panY;
  DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &panX, &panY);
  const float pan[2] = {panX, panY};
  SetShaderValue(e->shader, e->panLoc, pan, SHADER_UNIFORM_VEC2);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void VoxelMarchEffectUninit(VoxelMarchEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void VoxelMarchRegisterParams(VoxelMarchConfig *cfg) {
  ModEngineRegisterParam("voxelMarch.stepSize", &cfg->stepSize, 0.1f, 0.5f);
  ModEngineRegisterParam("voxelMarch.voxelScale", &cfg->voxelScale, 4.0f,
                         32.0f);
  ModEngineRegisterParam("voxelMarch.voxelVariation", &cfg->voxelVariation,
                         0.0f, 1.0f);
  ModEngineRegisterParam("voxelMarch.cellSize", &cfg->cellSize, 1.0f, 8.0f);
  ModEngineRegisterParam("voxelMarch.shellRadius", &cfg->shellRadius, 0.5f,
                         4.0f);
  ModEngineRegisterParam("voxelMarch.highlightIntensity",
                         &cfg->highlightIntensity, 0.0f, 0.5f);
  ModEngineRegisterParam("voxelMarch.positionTint", &cfg->positionTint, 0.0f,
                         1.0f);
  ModEngineRegisterParam("voxelMarch.tonemapGain", &cfg->tonemapGain, 0.0001f,
                         0.002f);
  ModEngineRegisterParam("voxelMarch.flySpeed", &cfg->flySpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("voxelMarch.gridAnimSpeed", &cfg->gridAnimSpeed, 0.0f,
                         3.0f);
  ModEngineRegisterParam("voxelMarch.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("voxelMarch.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("voxelMarch.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("voxelMarch.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("voxelMarch.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("voxelMarch.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 2.0f);
  ModEngineRegisterParam("voxelMarch.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("voxelMarch.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

VoxelMarchEffect *GetVoxelMarchEffect(PostEffect *pe) {
  return (VoxelMarchEffect *)pe->effectStates[TRANSFORM_VOXEL_MARCH_BLEND];
}

void SetupVoxelMarch(PostEffect *pe) {
  VoxelMarchEffectSetup(GetVoxelMarchEffect(pe), &pe->effects.voxelMarch,
                        pe->currentDeltaTime, pe->fftTexture);
}

void SetupVoxelMarchBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.voxelMarch.blendIntensity,
                       pe->effects.voxelMarch.blendMode);
}

// === UI ===

static void DrawVoxelMarchParams(EffectConfig *e, const ModSources *modSources,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;
  VoxelMarchConfig *vm = &e->voxelMarch;

  // Audio
  ImGui::SeparatorText("Audio");
  ImGui::Checkbox("Color Freq Map##voxelMarch", &vm->colorFreqMap);
  ModulatableSlider("Base Freq (Hz)##voxelMarch", &vm->baseFreq,
                    "voxelMarch.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##voxelMarch", &vm->maxFreq,
                    "voxelMarch.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##voxelMarch", &vm->gain, "voxelMarch.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##voxelMarch", &vm->curve, "voxelMarch.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##voxelMarch", &vm->baseBright,
                    "voxelMarch.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("March Steps##voxelMarch", &vm->marchSteps, 30, 80);
  ModulatableSlider("Step Size##voxelMarch", &vm->stepSize,
                    "voxelMarch.stepSize", "%.2f", modSources);
  ModulatableSlider("Voxel Scale##voxelMarch", &vm->voxelScale,
                    "voxelMarch.voxelScale", "%.1f", modSources);
  ModulatableSlider("Voxel Variation##voxelMarch", &vm->voxelVariation,
                    "voxelMarch.voxelVariation", "%.2f", modSources);
  ModulatableSlider("Cell Size##voxelMarch", &vm->cellSize,
                    "voxelMarch.cellSize", "%.1f", modSources);
  ModulatableSlider("Shell Radius##voxelMarch", &vm->shellRadius,
                    "voxelMarch.shellRadius", "%.2f", modSources);
  ImGui::Combo("Surface Shape##voxelMarch", &vm->surfaceShape,
               "Sphere\0Gyroid\0");
  ImGui::Combo("Domain Fold##voxelMarch", &vm->domainFold, "Tile\0Mirror\0");
  ImGui::Combo("Boundary Wave##voxelMarch", &vm->boundaryWave,
               "Sine\0Triangle\0Sawtooth\0");
  ImGui::SliderInt("Surfaces##voxelMarch", &vm->surfaceCount, 1, 2);
  ModulatableSliderLog("Highlight##voxelMarch", &vm->highlightIntensity,
                       "voxelMarch.highlightIntensity", "%.3f", modSources);
  ModulatableSlider("Position Tint##voxelMarch", &vm->positionTint,
                    "voxelMarch.positionTint", "%.2f", modSources);
  ModulatableSliderLog("Tonemap Gain##voxelMarch", &vm->tonemapGain,
                       "voxelMarch.tonemapGain", "%.4f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Fly Speed##voxelMarch", &vm->flySpeed,
                    "voxelMarch.flySpeed", "%.2f", modSources);
  ModulatableSlider("Grid Speed##voxelMarch", &vm->gridAnimSpeed,
                    "voxelMarch.gridAnimSpeed", "%.2f", modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  DrawLissajousControls(&vm->lissajous, "vm_cam", "voxelMarch.lissajous",
                        modSources, 2.0f);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(voxelMarch)
REGISTER_GENERATOR(TRANSFORM_VOXEL_MARCH_BLEND, VoxelMarch, voxelMarch, "Voxel March",
                   SetupVoxelMarchBlend, SetupVoxelMarch, 13, DrawVoxelMarchParams, DrawOutput_voxelMarch)
// clang-format on
