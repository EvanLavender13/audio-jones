#include "chladni_warp.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool ChladniWarpEffectInit(ChladniWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/chladni_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->nLoc = GetShaderLocation(e->shader, "n");
  e->mLoc = GetShaderLocation(e->shader, "m");
  e->plateSizeLoc = GetShaderLocation(e->shader, "plateSize");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->modeLoc = GetShaderLocation(e->shader, "warpMode");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->animRangeLoc = GetShaderLocation(e->shader, "animRange");
  e->preFoldLoc = GetShaderLocation(e->shader, "preFold");

  e->phase = 0.0f;

  return true;
}

void ChladniWarpEffectSetup(ChladniWarpEffect *e, const ChladniWarpConfig *cfg,
                            float deltaTime) {
  e->phase += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->nLoc, &cfg->n, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->mLoc, &cfg->m, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->plateSizeLoc, &cfg->plateSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->modeLoc, &cfg->warpMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->animPhaseLoc, &e->phase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->animRangeLoc, &cfg->animRange,
                 SHADER_UNIFORM_FLOAT);

  int preFold = cfg->preFold ? 1 : 0;
  SetShaderValue(e->shader, e->preFoldLoc, &preFold, SHADER_UNIFORM_INT);
}

void ChladniWarpEffectUninit(ChladniWarpEffect *e) { UnloadShader(e->shader); }

void ChladniWarpRegisterParams(ChladniWarpConfig *cfg) {
  ModEngineRegisterParam("chladniWarp.n", &cfg->n, 1.0f, 12.0f);
  ModEngineRegisterParam("chladniWarp.m", &cfg->m, 1.0f, 12.0f);
  ModEngineRegisterParam("chladniWarp.strength", &cfg->strength, 0.0f, 0.5f);
  ModEngineRegisterParam("chladniWarp.animRange", &cfg->animRange, 0.0f, 5.0f);
}

// === UI ===

static void DrawChladniWarpParams(EffectConfig *e, const ModSources *ms,
                                  ImU32 glow) {
  ChladniWarpConfig *cw = &e->chladniWarp;

  ModulatableSlider("N (X Mode)##chladni", &cw->n, "chladniWarp.n", "%.1f", ms);
  ModulatableSlider("M (Y Mode)##chladni", &cw->m, "chladniWarp.m", "%.1f", ms);
  ImGui::SliderFloat("Plate Size##chladni", &cw->plateSize, 0.5f, 2.0f, "%.2f");
  ModulatableSlider("Strength##chladni", &cw->strength, "chladniWarp.strength",
                    "%.3f", ms);

  const char *warpModeNames[] = {"Toward Nodes", "Along Nodes", "Intensity"};
  ImGui::Combo("Mode##chladni", &cw->warpMode, warpModeNames, 3);

  if (TreeNodeAccented("Animation##chladni", glow)) {
    ImGui::SliderFloat("Speed##chladni", &cw->speed, 0.0f, 2.0f, "%.2f rad/s");
    ModulatableSlider("Range##chladni", &cw->animRange, "chladniWarp.animRange",
                      "%.1f", ms);
    TreeNodeAccentedPop();
  }

  ImGui::Checkbox("Pre-Fold (Symmetry)##chladni", &cw->preFold);
}

void SetupChladniWarp(PostEffect *pe) {
  ChladniWarpEffectSetup(&pe->chladniWarp, &pe->effects.chladniWarp,
                         pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_CHLADNI_WARP, ChladniWarp, chladniWarp,
                "Chladni Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupChladniWarp, NULL, DrawChladniWarpParams)
// clang-format on
