// Spiral March effect module implementation
// Raymarched flight through a tumbling SDF lattice with ray-space rotation
// that bends straight rays into helical paths

#include "spiral_march.h"
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

bool SpiralMarchEffectInit(SpiralMarchEffect *e, const SpiralMarchConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/spiral_march.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->flyPhaseLoc = GetShaderLocation(e->shader, "flyPhase");
  e->cellPhaseLoc = GetShaderLocation(e->shader, "cellPhase");
  e->shapeTypeLoc = GetShaderLocation(e->shader, "shapeType");
  e->shapeSizeLoc = GetShaderLocation(e->shader, "shapeSize");
  e->cellPitchZLoc = GetShaderLocation(e->shader, "cellPitchZ");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->stepFactorLoc = GetShaderLocation(e->shader, "stepFactor");
  e->maxDistLoc = GetShaderLocation(e->shader, "maxDist");
  e->spiralRateLoc = GetShaderLocation(e->shader, "spiralRate");
  e->tColorDistScaleLoc = GetShaderLocation(e->shader, "tColorDistScale");
  e->tColorIterScaleLoc = GetShaderLocation(e->shader, "tColorIterScale");
  e->fovLoc = GetShaderLocation(e->shader, "fov");
  e->pitchAngleLoc = GetShaderLocation(e->shader, "pitchAngle");
  e->rollAngleLoc = GetShaderLocation(e->shader, "rollAngle");
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
  e->cellPhase = 0.0f;

  return true;
}

static void BindUniforms(const SpiralMarchEffect *e,
                         const SpiralMarchConfig *cfg) {
  SetShaderValue(e->shader, e->shapeTypeLoc, &cfg->shapeType,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->shapeSizeLoc, &cfg->shapeSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellPitchZLoc, &cfg->cellPitchZ,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->stepFactorLoc, &cfg->stepFactor,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxDistLoc, &cfg->maxDist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralRateLoc, &cfg->spiralRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tColorDistScaleLoc, &cfg->tColorDistScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tColorIterScaleLoc, &cfg->tColorIterScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fovLoc, &cfg->fov, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pitchAngleLoc, &cfg->pitchAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rollAngleLoc, &cfg->rollAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void SpiralMarchEffectSetup(SpiralMarchEffect *e, SpiralMarchConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture) {
  e->flyPhase += cfg->flySpeed * deltaTime;
  e->cellPhase += cfg->cellRotateSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flyPhaseLoc, &e->flyPhase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellPhaseLoc, &e->cellPhase,
                 SHADER_UNIFORM_FLOAT);

  BindUniforms(e, cfg);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void SpiralMarchEffectUninit(SpiralMarchEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void SpiralMarchRegisterParams(SpiralMarchConfig *cfg) {
  ModEngineRegisterParam("spiralMarch.shapeSize", &cfg->shapeSize, 0.05f,
                         0.45f);
  ModEngineRegisterParam("spiralMarch.cellPitchZ", &cfg->cellPitchZ, 0.05f,
                         1.0f);
  ModEngineRegisterParam("spiralMarch.stepFactor", &cfg->stepFactor, 0.5f,
                         1.0f);
  ModEngineRegisterParam("spiralMarch.maxDist", &cfg->maxDist, 20.0f, 200.0f);
  ModEngineRegisterParam("spiralMarch.spiralRate", &cfg->spiralRate, 0.0f,
                         3.0f);
  ModEngineRegisterParam("spiralMarch.tColorDistScale", &cfg->tColorDistScale,
                         0.0f, 0.1f);
  ModEngineRegisterParam("spiralMarch.tColorIterScale", &cfg->tColorIterScale,
                         0.0f, 0.05f);
  ModEngineRegisterParam("spiralMarch.flySpeed", &cfg->flySpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("spiralMarch.cellRotateSpeed", &cfg->cellRotateSpeed,
                         0.0f, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("spiralMarch.fov", &cfg->fov, 0.05f, 1.0f);
  ModEngineRegisterParam("spiralMarch.pitchAngle", &cfg->pitchAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("spiralMarch.rollAngle", &cfg->rollAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("spiralMarch.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("spiralMarch.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("spiralMarch.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("spiralMarch.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("spiralMarch.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("spiralMarch.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

SpiralMarchEffect *GetSpiralMarchEffect(PostEffect *pe) {
  return (SpiralMarchEffect *)pe->effectStates[TRANSFORM_SPIRAL_MARCH_BLEND];
}

void SetupSpiralMarch(PostEffect *pe) {
  SpiralMarchEffectSetup(GetSpiralMarchEffect(pe), &pe->effects.spiralMarch,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupSpiralMarchBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.spiralMarch.blendIntensity,
                       pe->effects.spiralMarch.blendMode);
}

// === UI ===

static void DrawSpiralMarchParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  SpiralMarchConfig *sm = &e->spiralMarch;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##spiralMarch", &sm->baseFreq,
                    "spiralMarch.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##spiralMarch", &sm->maxFreq,
                    "spiralMarch.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##spiralMarch", &sm->gain, "spiralMarch.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##spiralMarch", &sm->curve, "spiralMarch.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##spiralMarch", &sm->baseBright,
                    "spiralMarch.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::Combo("Shape##spiralMarch", &sm->shapeType,
               "Octahedron\0Box\0Sphere\0Torus\0");
  ModulatableSlider("Shape Size##spiralMarch", &sm->shapeSize,
                    "spiralMarch.shapeSize", "%.3f", modSources);
  ModulatableSlider("Cell Pitch Z##spiralMarch", &sm->cellPitchZ,
                    "spiralMarch.cellPitchZ", "%.3f", modSources);

  // Raymarching
  ImGui::SeparatorText("Raymarching");
  ImGui::SliderInt("March Steps##spiralMarch", &sm->marchSteps, 30, 120);
  ModulatableSlider("Step Factor##spiralMarch", &sm->stepFactor,
                    "spiralMarch.stepFactor", "%.2f", modSources);
  ModulatableSlider("Max Dist##spiralMarch", &sm->maxDist,
                    "spiralMarch.maxDist", "%.1f", modSources);
  ModulatableSlider("Spiral Rate##spiralMarch", &sm->spiralRate,
                    "spiralMarch.spiralRate", "%.2f", modSources);
  ModulatableSliderLog("Color Dist Scale##spiralMarch", &sm->tColorDistScale,
                       "spiralMarch.tColorDistScale", "%.4f", modSources);
  ModulatableSliderLog("Color Iter Scale##spiralMarch", &sm->tColorIterScale,
                       "spiralMarch.tColorIterScale", "%.4f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Fly Speed##spiralMarch", &sm->flySpeed,
                    "spiralMarch.flySpeed", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Cell Rotate Speed##spiralMarch",
                            &sm->cellRotateSpeed, "spiralMarch.cellRotateSpeed",
                            modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  ModulatableSlider("FOV##spiralMarch", &sm->fov, "spiralMarch.fov", "%.2f",
                    modSources);
  ModulatableSliderAngleDeg("Pitch##spiralMarch", &sm->pitchAngle,
                            "spiralMarch.pitchAngle", modSources);
  ModulatableSliderAngleDeg("Roll##spiralMarch", &sm->rollAngle,
                            "spiralMarch.rollAngle", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(spiralMarch)
REGISTER_GENERATOR(TRANSFORM_SPIRAL_MARCH_BLEND, SpiralMarch, spiralMarch, "Spiral March",
                   SetupSpiralMarchBlend, SetupSpiralMarch, 13, DrawSpiralMarchParams, DrawOutput_spiralMarch)
// clang-format on
