// Prism Shatter effect module implementation
// Raymarched cross-product crystal geometry with gradient coloring

#include "prism_shatter.h"
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

bool PrismShatterEffectInit(PrismShatterEffect *e,
                            const PrismShatterConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/prism_shatter.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->cameraTimeLoc = GetShaderLocation(e->shader, "cameraTime");
  e->structureTimeLoc = GetShaderLocation(e->shader, "structureTime");
  e->displacementScaleLoc = GetShaderLocation(e->shader, "displacementScale");
  e->stepSizeLoc = GetShaderLocation(e->shader, "stepSize");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->displacementModeLoc = GetShaderLocation(e->shader, "displacementMode");
  e->orbitRadiusLoc = GetShaderLocation(e->shader, "orbitRadius");
  e->fovLoc = GetShaderLocation(e->shader, "fov");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->saturationPowerLoc = GetShaderLocation(e->shader, "saturationPower");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->cameraTime = 0.0f;
  e->structureTime = 0.0f;

  return true;
}

void PrismShatterEffectSetup(PrismShatterEffect *e,
                             const PrismShatterConfig *cfg, float deltaTime) {
  e->cameraTime += cfg->cameraSpeed * deltaTime;
  e->structureTime += cfg->structureSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->cameraTimeLoc, &e->cameraTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->structureTimeLoc, &e->structureTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->displacementScaleLoc, &cfg->displacementScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stepSizeLoc, &cfg->stepSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->displacementModeLoc, &cfg->displacementMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->orbitRadiusLoc, &cfg->orbitRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fovLoc, &cfg->fov, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->saturationPowerLoc, &cfg->saturationPower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void PrismShatterEffectUninit(PrismShatterEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void PrismShatterRegisterParams(PrismShatterConfig *cfg) {
  ModEngineRegisterParam("prismShatter.displacementScale",
                         &cfg->displacementScale, 1.0f, 10.0f);
  ModEngineRegisterParam("prismShatter.stepSize", &cfg->stepSize, 0.5f, 5.0f);
  ModEngineRegisterParam("prismShatter.cameraSpeed", &cfg->cameraSpeed, -1.0f,
                         1.0f);
  ModEngineRegisterParam("prismShatter.orbitRadius", &cfg->orbitRadius, 4.0f,
                         32.0f);
  ModEngineRegisterParam("prismShatter.fov", &cfg->fov, 0.5f, 3.0f);
  ModEngineRegisterParam("prismShatter.structureSpeed", &cfg->structureSpeed,
                         -1.0f, 1.0f);
  ModEngineRegisterParam("prismShatter.brightness", &cfg->brightness, 0.5f,
                         3.0f);
  ModEngineRegisterParam("prismShatter.saturationPower", &cfg->saturationPower,
                         1.0f, 4.0f);
  ModEngineRegisterParam("prismShatter.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupPrismShatter(PostEffect *pe) {
  PrismShatterEffectSetup(&pe->prismShatter, &pe->effects.prismShatter,
                          pe->currentDeltaTime);
}

void SetupPrismShatterBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.prismShatter.blendIntensity,
                       pe->effects.prismShatter.blendMode);
}

// === UI ===

static void DrawPrismShatterParams(EffectConfig *e,
                                   const ModSources *modSources,
                                   ImU32 categoryGlow) {
  (void)categoryGlow;
  PrismShatterConfig *cfg = &e->prismShatter;

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##prismShatter", &cfg->iterations, 64, 256);
  ImGui::Combo("Displacement Mode##prismShatter", &cfg->displacementMode,
               "Triple Product\0Absolute Fold\0Mandelbox\0Sierpinski\0Menger\0"
               "Burning Ship\0");
  ModulatableSlider("Displacement##prismShatter", &cfg->displacementScale,
                    "prismShatter.displacementScale", "%.1f", modSources);
  ModulatableSlider("Step Size##prismShatter", &cfg->stepSize,
                    "prismShatter.stepSize", "%.1f", modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  ModulatableSlider("Camera Speed##prismShatter", &cfg->cameraSpeed,
                    "prismShatter.cameraSpeed", "%.3f", modSources);
  ModulatableSlider("Orbit Radius##prismShatter", &cfg->orbitRadius,
                    "prismShatter.orbitRadius", "%.1f", modSources);
  ModulatableSlider("FOV##prismShatter", &cfg->fov, "prismShatter.fov", "%.2f",
                    modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Structure Speed##prismShatter", &cfg->structureSpeed,
                    "prismShatter.structureSpeed", "%.3f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Brightness##prismShatter", &cfg->brightness,
                    "prismShatter.brightness", "%.2f", modSources);
  ModulatableSlider("Edge Contrast##prismShatter", &cfg->saturationPower,
                    "prismShatter.saturationPower", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(prismShatter)
REGISTER_GENERATOR(TRANSFORM_PRISM_SHATTER_BLEND, PrismShatter, prismShatter,
                   "Prism Shatter", SetupPrismShatterBlend,
                   SetupPrismShatter, 10, DrawPrismShatterParams, DrawOutput_prismShatter)
// clang-format on
