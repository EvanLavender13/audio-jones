// Synapse Tree effect module implementation
// Raymarched sphere-inversion fractal tree with synapse pulses

#include "synapse_tree.h"
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
#include "render/render_utils.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

bool SynapseTreeEffectInit(SynapseTreeEffect *e, const SynapseTreeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/synapse_tree.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->foldIterationsLoc = GetShaderLocation(e->shader, "foldIterations");
  e->fovLoc = GetShaderLocation(e->shader, "fov");
  e->foldOffsetLoc = GetShaderLocation(e->shader, "foldOffset");
  e->branchThicknessLoc = GetShaderLocation(e->shader, "branchThickness");
  e->orbitAngleLoc = GetShaderLocation(e->shader, "orbitAngle");
  e->synapseIntensityLoc = GetShaderLocation(e->shader, "synapseIntensity");
  e->synapseBounceFreqLoc = GetShaderLocation(e->shader, "synapseBounceFreq");
  e->synapsePulseFreqLoc = GetShaderLocation(e->shader, "synapsePulseFreq");
  e->colorPhaseLoc = GetShaderLocation(e->shader, "colorPhase");
  e->colorStretchLoc = GetShaderLocation(e->shader, "colorStretch");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->animPhase = 0.0f;
  e->orbitAngle = 0.0f;
  e->colorPhase = 0.0f;

  return true;
}

void SynapseTreeEffectSetup(SynapseTreeEffect *e, const SynapseTreeConfig *cfg,
                            float deltaTime) {
  e->animPhase += cfg->animSpeed * deltaTime;
  e->orbitAngle += cfg->orbitSpeed * deltaTime;
  e->colorPhase += cfg->colorSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->animPhaseLoc, &e->animPhase,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->foldIterationsLoc, &cfg->foldIterations,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->fovLoc, &cfg->fov, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->foldOffsetLoc, &cfg->foldOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->branchThicknessLoc, &cfg->branchThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitAngleLoc, &e->orbitAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->synapseIntensityLoc, &cfg->synapseIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->synapseBounceFreqLoc, &cfg->synapseBounceFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->synapsePulseFreqLoc, &cfg->synapsePulseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorPhaseLoc, &e->colorPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorStretchLoc, &cfg->colorStretch,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void SynapseTreeEffectUninit(SynapseTreeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

SynapseTreeConfig SynapseTreeConfigDefault(void) { return SynapseTreeConfig{}; }

void SynapseTreeRegisterParams(SynapseTreeConfig *cfg) {
  ModEngineRegisterParam("synapseTree.fov", &cfg->fov, 1.0f, 3.0f);
  ModEngineRegisterParam("synapseTree.foldOffset", &cfg->foldOffset, 0.2f,
                         0.6f);
  ModEngineRegisterParam("synapseTree.branchThickness", &cfg->branchThickness,
                         0.01f, 0.2f);
  ModEngineRegisterParam("synapseTree.animSpeed", &cfg->animSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("synapseTree.orbitSpeed", &cfg->orbitSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("synapseTree.synapseIntensity", &cfg->synapseIntensity,
                         0.0f, 2.0f);
  ModEngineRegisterParam("synapseTree.synapseBounceFreq",
                         &cfg->synapseBounceFreq, 1.0f, 20.0f);
  ModEngineRegisterParam("synapseTree.synapsePulseFreq", &cfg->synapsePulseFreq,
                         1.0f, 15.0f);
  ModEngineRegisterParam("synapseTree.colorSpeed", &cfg->colorSpeed, 0.0f,
                         2.0f);
  ModEngineRegisterParam("synapseTree.colorStretch", &cfg->colorStretch, 0.1f,
                         5.0f);
  ModEngineRegisterParam("synapseTree.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupSynapseTree(PostEffect *pe) {
  SynapseTreeEffectSetup(&pe->synapseTree, &pe->effects.synapseTree,
                         pe->currentDeltaTime);
}

void SetupSynapseTreeBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.synapseTree.blendIntensity,
                       pe->effects.synapseTree.blendMode);
}

// === UI ===

static void DrawSynapseTreeParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  SynapseTreeConfig *s = &e->synapseTree;

  // Raymarching
  ImGui::SeparatorText("Raymarching");
  ImGui::SliderInt("March Steps##synapseTree", &s->marchSteps, 40, 200);
  ImGui::SliderInt("Fold Depth##synapseTree", &s->foldIterations, 4, 12);
  ModulatableSlider("FOV##synapseTree", &s->fov, "synapseTree.fov", "%.1f",
                    modSources);

  // Shape
  ImGui::SeparatorText("Shape");
  ModulatableSlider("Branch Spread##synapseTree", &s->foldOffset,
                    "synapseTree.foldOffset", "%.2f", modSources);
  ModulatableSliderLog("Thickness##synapseTree", &s->branchThickness,
                       "synapseTree.branchThickness", "%.3f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Anim Speed##synapseTree", &s->animSpeed,
                    "synapseTree.animSpeed", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Orbit Speed##synapseTree", &s->orbitSpeed,
                            "synapseTree.orbitSpeed", modSources);

  // Synapse
  ImGui::SeparatorText("Synapse");
  ModulatableSlider("Intensity##synapseTree", &s->synapseIntensity,
                    "synapseTree.synapseIntensity", "%.2f", modSources);
  ModulatableSlider("Bounce Freq##synapseTree", &s->synapseBounceFreq,
                    "synapseTree.synapseBounceFreq", "%.1f", modSources);
  ModulatableSlider("Pulse Freq##synapseTree", &s->synapsePulseFreq,
                    "synapseTree.synapsePulseFreq", "%.1f", modSources);

  // Color
  ImGui::SeparatorText("Color");
  ModulatableSlider("Color Speed##synapseTree", &s->colorSpeed,
                    "synapseTree.colorSpeed", "%.2f", modSources);
  ModulatableSlider("Color Stretch##synapseTree", &s->colorStretch,
                    "synapseTree.colorStretch", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(synapseTree)
REGISTER_GENERATOR(TRANSFORM_SYNAPSE_TREE_BLEND, SynapseTree, synapseTree,
                   "Synapse Tree", SetupSynapseTreeBlend, SetupSynapseTree, 11,
                   DrawSynapseTreeParams, DrawOutput_synapseTree)
// clang-format on
