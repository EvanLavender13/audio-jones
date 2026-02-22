// Plasma effect module implementation
// Generates animated lightning bolts via FBM noise with glow and drift

#include "plasma.h"
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

bool PlasmaEffectInit(PlasmaEffect *e, const PlasmaConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/plasma.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->boltCountLoc = GetShaderLocation(e->shader, "boltCount");
  e->layerCountLoc = GetShaderLocation(e->shader, "layerCount");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->falloffTypeLoc = GetShaderLocation(e->shader, "falloffType");
  e->driftAmountLoc = GetShaderLocation(e->shader, "driftAmount");
  e->displacementLoc = GetShaderLocation(e->shader, "displacement");
  e->glowRadiusLoc = GetShaderLocation(e->shader, "glowRadius");
  e->coreBrightnessLoc = GetShaderLocation(e->shader, "coreBrightness");
  e->flickerAmountLoc = GetShaderLocation(e->shader, "flickerAmount");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->driftPhaseLoc = GetShaderLocation(e->shader, "driftPhase");
  e->flickerTimeLoc = GetShaderLocation(e->shader, "flickerTime");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->animPhase = 0.0f;
  e->driftPhase = 0.0f;
  e->flickerTime = 0.0f;

  return true;
}

void PlasmaEffectSetup(PlasmaEffect *e, const PlasmaConfig *cfg,
                       float deltaTime) {
  e->animPhase += cfg->animSpeed * deltaTime;
  e->driftPhase += cfg->driftSpeed * deltaTime;
  e->flickerTime += deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->boltCountLoc, &cfg->boltCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->layerCountLoc, &cfg->layerCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->falloffTypeLoc, &cfg->falloffType,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->driftAmountLoc, &cfg->driftAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->displacementLoc, &cfg->displacement,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowRadiusLoc, &cfg->glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->coreBrightnessLoc, &cfg->coreBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flickerAmountLoc, &cfg->flickerAmount,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->animPhaseLoc, &e->animPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftPhaseLoc, &e->driftPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flickerTimeLoc, &e->flickerTime,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void PlasmaEffectUninit(PlasmaEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

PlasmaConfig PlasmaConfigDefault(void) { return PlasmaConfig{}; }

void PlasmaRegisterParams(PlasmaConfig *cfg) {
  ModEngineRegisterParam("plasma.animSpeed", &cfg->animSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("plasma.coreBrightness", &cfg->coreBrightness, 0.5f,
                         3.0f);
  ModEngineRegisterParam("plasma.displacement", &cfg->displacement, 0.0f, 2.0f);
  ModEngineRegisterParam("plasma.driftAmount", &cfg->driftAmount, 0.0f, 1.0f);
  ModEngineRegisterParam("plasma.driftSpeed", &cfg->driftSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("plasma.flickerAmount", &cfg->flickerAmount, 0.0f,
                         1.0f);
  ModEngineRegisterParam("plasma.glowRadius", &cfg->glowRadius, 0.01f, 0.3f);
  ModEngineRegisterParam("plasma.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupPlasma(PostEffect *pe) {
  PlasmaEffectSetup(&pe->plasma, &pe->effects.plasma, pe->currentDeltaTime);
}

void SetupPlasmaBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.plasma.blendIntensity,
                       pe->effects.plasma.blendMode);
}

// === UI ===

static void DrawPlasmaParams(EffectConfig *e, const ModSources *modSources,
                             ImU32 categoryGlow) {
  PlasmaConfig *p = &e->plasma;

  // Bolt configuration
  ImGui::SliderInt("Bolt Count##plasma", &p->boltCount, 1, 8);
  ImGui::SliderInt("Layers##plasma", &p->layerCount, 1, 3);
  ImGui::SliderInt("Octaves##plasma", &p->octaves, 1, 10);
  ImGui::Combo("Falloff##plasma", &p->falloffType, "Sharp\0Linear\0Soft\0");

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Animation
  ModulatableSlider("Drift Speed##plasma", &p->driftSpeed, "plasma.driftSpeed",
                    "%.2f", modSources);
  ModulatableSlider("Drift Amount##plasma", &p->driftAmount,
                    "plasma.driftAmount", "%.2f", modSources);
  ModulatableSlider("Anim Speed##plasma", &p->animSpeed, "plasma.animSpeed",
                    "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Appearance
  ModulatableSlider("Displacement##plasma", &p->displacement,
                    "plasma.displacement", "%.2f", modSources);
  ModulatableSlider("Glow Radius##plasma", &p->glowRadius, "plasma.glowRadius",
                    "%.3f", modSources);
  ModulatableSlider("Brightness##plasma", &p->coreBrightness,
                    "plasma.coreBrightness", "%.2f", modSources);
  ModulatableSlider("Flicker##plasma", &p->flickerAmount,
                    "plasma.flickerAmount", "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(plasma)
REGISTER_GENERATOR(TRANSFORM_PLASMA_BLEND, Plasma, plasma, "Plasma",
                   SetupPlasmaBlend, SetupPlasma, 12, DrawPlasmaParams, DrawOutput_plasma)
// clang-format on
