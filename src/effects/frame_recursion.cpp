// Frame Recursion effect module implementation
// Raymarched kaleidoscopic plane-fold IFS wireframe polyhedron tunnel with
// fract(log) self-similar zoom and FFT-driven volumetric glow

#include "frame_recursion.h"
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

bool FrameRecursionEffectInit(FrameRecursionEffect *e,
                              const FrameRecursionConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/frame_recursion.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->rotateAngleLoc = GetShaderLocation(e->shader, "rotateAngle");
  e->zoomPhaseLoc = GetShaderLocation(e->shader, "zoomPhase");
  e->shapeLoc = GetShaderLocation(e->shader, "shape");
  e->edgeRadiusLoc = GetShaderLocation(e->shader, "edgeRadius");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
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

  e->rotateAngleX = 0.0f;
  e->rotateAngleY = 0.0f;
  e->rotateAngleZ = 0.0f;
  e->zoomPhase = 0.0f;

  return true;
}

static void BindUniforms(const FrameRecursionEffect *e,
                         const FrameRecursionConfig *cfg) {
  SetShaderValue(e->shader, e->shapeLoc, &cfg->shape, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->edgeRadiusLoc, &cfg->edgeRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void FrameRecursionEffectSetup(FrameRecursionEffect *e,
                               FrameRecursionConfig *cfg, float deltaTime,
                               const Texture2D &fftTexture) {
  const float twoPi = 2.0f * PI_F;
  e->rotateAngleX =
      fmodf(e->rotateAngleX + cfg->rotateSpeedX * deltaTime, twoPi);
  e->rotateAngleY =
      fmodf(e->rotateAngleY + cfg->rotateSpeedY * deltaTime, twoPi);
  e->rotateAngleZ =
      fmodf(e->rotateAngleZ + cfg->rotateSpeedZ * deltaTime, twoPi);
  e->zoomPhase = fmodf(e->zoomPhase + cfg->zoomSpeed * deltaTime, 1.0f);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);

  const float rotateAngle[3] = {e->rotateAngleX, e->rotateAngleY,
                                e->rotateAngleZ};
  SetShaderValue(e->shader, e->rotateAngleLoc, rotateAngle,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(e->shader, e->zoomPhaseLoc, &e->zoomPhase,
                 SHADER_UNIFORM_FLOAT);

  BindUniforms(e, cfg);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void FrameRecursionEffectUninit(FrameRecursionEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void FrameRecursionRegisterParams(FrameRecursionConfig *cfg) {
  ModEngineRegisterParam("frameRecursion.rotateSpeedX", &cfg->rotateSpeedX,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("frameRecursion.rotateSpeedY", &cfg->rotateSpeedY,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("frameRecursion.rotateSpeedZ", &cfg->rotateSpeedZ,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("frameRecursion.zoomSpeed", &cfg->zoomSpeed, -2.0f,
                         2.0f);
  ModEngineRegisterParam("frameRecursion.edgeRadius", &cfg->edgeRadius, 0.001f,
                         0.05f);
  ModEngineRegisterParam("frameRecursion.glowIntensity", &cfg->glowIntensity,
                         0.0f, 2.0f);
  ModEngineRegisterParam("frameRecursion.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("frameRecursion.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("frameRecursion.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("frameRecursion.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("frameRecursion.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("frameRecursion.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

FrameRecursionEffect *GetFrameRecursionEffect(PostEffect *pe) {
  return (FrameRecursionEffect *)
      pe->effectStates[TRANSFORM_FRAME_RECURSION_BLEND];
}

void SetupFrameRecursion(PostEffect *pe) {
  FrameRecursionEffectSetup(GetFrameRecursionEffect(pe),
                            &pe->effects.frameRecursion, pe->currentDeltaTime,
                            pe->fftTexture);
}

void SetupFrameRecursionBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.frameRecursion.blendIntensity,
                       pe->effects.frameRecursion.blendMode);
}

// === UI ===

static void DrawFrameRecursionParams(EffectConfig *e,
                                     const ModSources *modSources,
                                     ImU32 categoryGlow) {
  (void)categoryGlow;
  FrameRecursionConfig *ft = &e->frameRecursion;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##frameRecursion", &ft->baseFreq,
                    "frameRecursion.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##frameRecursion", &ft->maxFreq,
                    "frameRecursion.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##frameRecursion", &ft->gain, "frameRecursion.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##frameRecursion", &ft->curve,
                    "frameRecursion.curve", "%.2f", modSources);
  ModulatableSlider("Base Bright##frameRecursion", &ft->baseBright,
                    "frameRecursion.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::Combo("Shape##frameRecursion", &ft->shape,
               "Octahedron\0Dodecahedron\0Icosahedron\0");
  ModulatableSliderLog("Edge Radius##frameRecursion", &ft->edgeRadius,
                       "frameRecursion.edgeRadius", "%.3f", modSources);
  ImGui::SliderInt("March Steps##frameRecursion", &ft->marchSteps, 32, 128);
  ImGui::Combo("Color Mode##frameRecursion", &ft->colorMode,
               "Banded\0Layered\0Depth\0");

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotate Speed X##frameRecursion", &ft->rotateSpeedX,
                            "frameRecursion.rotateSpeedX", modSources);
  ModulatableSliderSpeedDeg("Rotate Speed Y##frameRecursion", &ft->rotateSpeedY,
                            "frameRecursion.rotateSpeedY", modSources);
  ModulatableSliderSpeedDeg("Rotate Speed Z##frameRecursion", &ft->rotateSpeedZ,
                            "frameRecursion.rotateSpeedZ", modSources);
  ModulatableSlider("Zoom Speed##frameRecursion", &ft->zoomSpeed,
                    "frameRecursion.zoomSpeed", "%.2f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow##frameRecursion", &ft->glowIntensity,
                    "frameRecursion.glowIntensity", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(frameRecursion)
REGISTER_GENERATOR(TRANSFORM_FRAME_RECURSION_BLEND, FrameRecursion, frameRecursion,
                   "Frame Recursion", SetupFrameRecursionBlend, SetupFrameRecursion, 10,
                   DrawFrameRecursionParams, DrawOutput_frameRecursion)
// clang-format on
