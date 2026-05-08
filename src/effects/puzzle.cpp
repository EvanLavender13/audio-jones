// Puzzle effect module implementation

#include "puzzle.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool PuzzleEffectInit(PuzzleEffect *e) {
  e->shader = LoadShader(NULL, "shaders/puzzle.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->pieceCountLoc = GetShaderLocation(e->shader, "pieceCount");
  e->seedLoc = GetShaderLocation(e->shader, "seed");
  e->fillModeLoc = GetShaderLocation(e->shader, "fillMode");
  e->edgeLightLoc = GetShaderLocation(e->shader, "edgeLight");

  return true;
}

void PuzzleEffectSetup(const PuzzleEffect *e, const PuzzleConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->pieceCountLoc, &cfg->pieceCount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->seedLoc, &cfg->seed, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fillModeLoc, &cfg->fillMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->edgeLightLoc, &cfg->edgeLight,
                 SHADER_UNIFORM_FLOAT);
}

void PuzzleEffectUninit(const PuzzleEffect *e) { UnloadShader(e->shader); }

void PuzzleRegisterParams(PuzzleConfig *cfg) {
  ModEngineRegisterParam("puzzle.pieceCount", &cfg->pieceCount, 4.0f, 40.0f);
  ModEngineRegisterParam("puzzle.seed", &cfg->seed, 0.0f, 100.0f);
  ModEngineRegisterParam("puzzle.edgeLight", &cfg->edgeLight, 0.0f, 1.0f);
}

PuzzleEffect *GetPuzzleEffect(PostEffect *pe) {
  return (PuzzleEffect *)pe->effectStates[TRANSFORM_PUZZLE];
}

void SetupPuzzle(PostEffect *pe) {
  PuzzleEffectSetup(GetPuzzleEffect(pe), &pe->effects.puzzle);
}

// === UI ===

static void DrawPuzzleParams(EffectConfig *e, const ModSources *ms,
                             ImU32 glow) {
  (void)glow;
  ImGui::SeparatorText("Geometry");
  ModulatableSliderInt("Pieces##puzzle", &e->puzzle.pieceCount,
                       "puzzle.pieceCount", ms);
  ModulatableSlider("Seed##puzzle", &e->puzzle.seed, "puzzle.seed", "%.2f", ms);
  ImGui::SeparatorText("Color");
  ImGui::Combo("Fill Mode##puzzle", &e->puzzle.fillMode, "Texture\0Solid\0");
  ImGui::SeparatorText("Lighting");
  ModulatableSlider("Edge Light##puzzle", &e->puzzle.edgeLight,
                    "puzzle.edgeLight", "%.2f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_PUZZLE, Puzzle, puzzle, "Puzzle", "NOV", 14,
                EFFECT_FLAG_NONE, SetupPuzzle, NULL, DrawPuzzleParams)
// clang-format on
