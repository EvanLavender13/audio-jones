#include "circuit_board.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool CircuitBoardEffectInit(CircuitBoardEffect *e) {
  e->shader = LoadShader(NULL, "shaders/circuit_board.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->tileScaleLoc = GetShaderLocation(e->shader, "tileScale");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->baseSizeLoc = GetShaderLocation(e->shader, "baseSize");
  e->breatheLoc = GetShaderLocation(e->shader, "breathe");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->dualLayerLoc = GetShaderLocation(e->shader, "dualLayer");
  e->layerOffsetLoc = GetShaderLocation(e->shader, "layerOffset");
  e->contourFreqLoc = GetShaderLocation(e->shader, "contourFreq");

  e->time = 0.0f;

  return true;
}

void CircuitBoardEffectSetup(CircuitBoardEffect *e,
                             const CircuitBoardConfig *cfg, float deltaTime) {
  e->time += cfg->breatheSpeed * deltaTime;

  SetShaderValue(e->shader, e->tileScaleLoc, &cfg->tileScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseSizeLoc, &cfg->baseSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->breatheLoc, &cfg->breathe, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  int dualLayer = cfg->dualLayer ? 1 : 0;
  SetShaderValue(e->shader, e->dualLayerLoc, &dualLayer, SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->layerOffsetLoc, &cfg->layerOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->contourFreqLoc, &cfg->contourFreq,
                 SHADER_UNIFORM_FLOAT);
}

void CircuitBoardEffectUninit(const CircuitBoardEffect *e) {
  UnloadShader(e->shader);
}

void CircuitBoardRegisterParams(CircuitBoardConfig *cfg) {
  ModEngineRegisterParam("circuitBoard.tileScale", &cfg->tileScale, 2.0f,
                         16.0f);
  ModEngineRegisterParam("circuitBoard.strength", &cfg->strength, 0.0f, 1.0f);
  ModEngineRegisterParam("circuitBoard.baseSize", &cfg->baseSize, 0.3f, 0.9f);
  ModEngineRegisterParam("circuitBoard.breathe", &cfg->breathe, 0.0f, 0.4f);
  ModEngineRegisterParam("circuitBoard.breatheSpeed", &cfg->breatheSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("circuitBoard.layerOffset", &cfg->layerOffset, 5.0f,
                         80.0f);
  ModEngineRegisterParam("circuitBoard.contourFreq", &cfg->contourFreq, 0.0f,
                         80.0f);
}

// === UI ===

static void DrawCircuitBoardParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Tile Scale##circuitboard", &e->circuitBoard.tileScale,
                    "circuitBoard.tileScale", "%.1f", ms);
  ModulatableSlider("Strength##circuitboard", &e->circuitBoard.strength,
                    "circuitBoard.strength", "%.2f", ms);
  ModulatableSlider("Base Size##circuitboard", &e->circuitBoard.baseSize,
                    "circuitBoard.baseSize", "%.2f", ms);
  ImGui::SeparatorText("Pattern");
  ModulatableSlider("Contour Freq##circuitboard", &e->circuitBoard.contourFreq,
                    "circuitBoard.contourFreq", "%.1f", ms);
  ImGui::Checkbox("Dual Layer##circuitboard", &e->circuitBoard.dualLayer);
  if (e->circuitBoard.dualLayer) {
    ModulatableSlider("Layer Offset##circuitboard",
                      &e->circuitBoard.layerOffset, "circuitBoard.layerOffset",
                      "%.1f", ms);
  }
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Breathe##circuitboard", &e->circuitBoard.breathe,
                    "circuitBoard.breathe", "%.2f", ms);
  ModulatableSliderSpeedDeg("Breathe Speed##circuitboard",
                            &e->circuitBoard.breatheSpeed,
                            "circuitBoard.breatheSpeed", ms);
}

CircuitBoardEffect *GetCircuitBoardEffect(PostEffect *pe) {
  return (CircuitBoardEffect *)pe->effectStates[TRANSFORM_CIRCUIT_BOARD];
}

void SetupCircuitBoard(PostEffect *pe) {
  CircuitBoardEffectSetup(GetCircuitBoardEffect(pe), &pe->effects.circuitBoard,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_CIRCUIT_BOARD, CircuitBoard, circuitBoard,
                "Circuit Board", "WARP", 1, EFFECT_FLAG_NONE,
                SetupCircuitBoard, NULL, DrawCircuitBoardParams)
// clang-format on
