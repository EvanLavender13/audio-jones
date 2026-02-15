#include "circuit_board.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

void CircuitBoardEffectUninit(CircuitBoardEffect *e) {
  UnloadShader(e->shader);
}

CircuitBoardConfig CircuitBoardConfigDefault(void) {
  return CircuitBoardConfig{};
}

void CircuitBoardRegisterParams(CircuitBoardConfig *cfg) {
  ModEngineRegisterParam("circuitBoard.tileScale", &cfg->tileScale, 2.0f,
                         16.0f);
  ModEngineRegisterParam("circuitBoard.strength", &cfg->strength, 0.0f, 1.0f);
  ModEngineRegisterParam("circuitBoard.baseSize", &cfg->baseSize, 0.3f, 0.9f);
  ModEngineRegisterParam("circuitBoard.breathe", &cfg->breathe, 0.0f, 0.4f);
  ModEngineRegisterParam("circuitBoard.breatheSpeed", &cfg->breatheSpeed, 0.1f,
                         4.0f);
  ModEngineRegisterParam("circuitBoard.layerOffset", &cfg->layerOffset, 5.0f,
                         80.0f);
  ModEngineRegisterParam("circuitBoard.contourFreq", &cfg->contourFreq, 0.0f,
                         80.0f);
}

void SetupCircuitBoard(PostEffect *pe) {
  CircuitBoardEffectSetup(&pe->circuitBoard, &pe->effects.circuitBoard,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_CIRCUIT_BOARD, CircuitBoard, circuitBoard,
                "Circuit Board", "WARP", 1, EFFECT_FLAG_NONE,
                SetupCircuitBoard, NULL)
// clang-format on
