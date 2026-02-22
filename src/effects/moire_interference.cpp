#include "moire_interference.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool MoireInterferenceEffectInit(MoireInterferenceEffect *e) {
  e->shader = LoadShader(NULL, "shaders/moire_interference.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->rotationAngleLoc = GetShaderLocation(e->shader, "rotationAngle");
  e->scaleDiffLoc = GetShaderLocation(e->shader, "scaleDiff");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->blendModeLoc = GetShaderLocation(e->shader, "blendMode");
  e->centerXLoc = GetShaderLocation(e->shader, "centerX");
  e->centerYLoc = GetShaderLocation(e->shader, "centerY");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");

  e->rotationAccum = 0.0f;

  return true;
}

void MoireInterferenceEffectSetup(MoireInterferenceEffect *e,
                                  const MoireInterferenceConfig *cfg,
                                  float deltaTime) {
  e->rotationAccum += cfg->animationSpeed * deltaTime;

  SetShaderValue(e->shader, e->rotationAngleLoc, &cfg->rotationAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleDiffLoc, &cfg->scaleDiff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->blendModeLoc, &cfg->blendMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->centerXLoc, &cfg->centerX, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->centerYLoc, &cfg->centerY, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

void MoireInterferenceEffectUninit(MoireInterferenceEffect *e) {
  UnloadShader(e->shader);
}

MoireInterferenceConfig MoireInterferenceConfigDefault(void) {
  return MoireInterferenceConfig{};
}

void MoireInterferenceRegisterParams(MoireInterferenceConfig *cfg) {
  ModEngineRegisterParam("moireInterference.rotationAngle", &cfg->rotationAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("moireInterference.scaleDiff", &cfg->scaleDiff, 0.5f,
                         2.0f);
  ModEngineRegisterParam("moireInterference.animationSpeed",
                         &cfg->animationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
}

// === UI ===

static void DrawMoireInterferenceParams(EffectConfig *e, const ModSources *ms,
                                        ImU32 glow) {
  MoireInterferenceConfig *mi = &e->moireInterference;

  ModulatableSliderAngleDeg("Rotation##moire", &mi->rotationAngle,
                            "moireInterference.rotationAngle", ms, "%.1f °");
  ModulatableSlider("Scale Diff##moire", &mi->scaleDiff,
                    "moireInterference.scaleDiff", "%.3f", ms);
  ImGui::SliderInt("Layers##moire", &mi->layers, 2, 4);
  ImGui::Combo("Blend Mode##moire", &mi->blendMode,
               "Multiply\0Min\0Average\0Difference\0");
  ModulatableSliderSpeedDeg("Spin##moire", &mi->animationSpeed,
                            "moireInterference.animationSpeed", ms);
  if (TreeNodeAccented("Center##moire", glow)) {
    ImGui::SliderFloat("X##moirecenter", &mi->centerX, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Y##moirecenter", &mi->centerY, 0.0f, 1.0f, "%.2f");
    TreeNodeAccentedPop();
  }
}

void SetupMoireInterference(PostEffect *pe) {
  MoireInterferenceEffectSetup(&pe->moireInterference,
                               &pe->effects.moireInterference,
                               pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(
    TRANSFORM_MOIRE_INTERFERENCE, MoireInterference, moireInterference,
    "Moire Interference", "SYM", 0, EFFECT_FLAG_NONE,
    SetupMoireInterference, NULL, DrawMoireInterferenceParams)
// clang-format on
