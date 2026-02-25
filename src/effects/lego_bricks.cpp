// LEGO bricks effect module implementation

#include "lego_bricks.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool LegoBricksEffectInit(LegoBricksEffect *e) {
  e->shader = LoadShader(NULL, "shaders/lego_bricks.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->brickScaleLoc = GetShaderLocation(e->shader, "brickScale");
  e->studHeightLoc = GetShaderLocation(e->shader, "studHeight");
  e->edgeShadowLoc = GetShaderLocation(e->shader, "edgeShadow");
  e->colorThresholdLoc = GetShaderLocation(e->shader, "colorThreshold");
  e->maxBrickSizeLoc = GetShaderLocation(e->shader, "maxBrickSize");
  e->lightAngleLoc = GetShaderLocation(e->shader, "lightAngle");

  return true;
}

void LegoBricksEffectSetup(LegoBricksEffect *e, const LegoBricksConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->brickScaleLoc, &cfg->brickScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->studHeightLoc, &cfg->studHeight,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeShadowLoc, &cfg->edgeShadow,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorThresholdLoc, &cfg->colorThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxBrickSizeLoc, &cfg->maxBrickSize,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lightAngleLoc, &cfg->lightAngle,
                 SHADER_UNIFORM_FLOAT);
}

void LegoBricksEffectUninit(LegoBricksEffect *e) { UnloadShader(e->shader); }

LegoBricksConfig LegoBricksConfigDefault(void) { return LegoBricksConfig{}; }

void LegoBricksRegisterParams(LegoBricksConfig *cfg) {
  ModEngineRegisterParam("legoBricks.brickScale", &cfg->brickScale, 0.01f,
                         0.2f);
  ModEngineRegisterParam("legoBricks.studHeight", &cfg->studHeight, 0.0f, 1.0f);
  ModEngineRegisterParam("legoBricks.lightAngle", &cfg->lightAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

void SetupLegoBricks(PostEffect *pe) {
  LegoBricksEffectSetup(&pe->legoBricks, &pe->effects.legoBricks);
}

// === UI ===

static void DrawLegoBricksParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  (void)glow;
  ModulatableSlider("Brick Scale##legobricks", &e->legoBricks.brickScale,
                    "legoBricks.brickScale", "%.3f", ms);
  ModulatableSlider("Stud Height##legobricks", &e->legoBricks.studHeight,
                    "legoBricks.studHeight", "%.2f", ms);
  ImGui::SliderFloat("Edge Shadow##legobricks", &e->legoBricks.edgeShadow, 0.0f,
                     1.0f, "%.2f");
  ImGui::SliderFloat("Color Threshold##legobricks",
                     &e->legoBricks.colorThreshold, 0.0f, 0.5f, "%.3f");
  ImGui::SliderInt("Max Brick Size##legobricks", &e->legoBricks.maxBrickSize, 1,
                   4);
  ModulatableSliderAngleDeg("Light Angle##legobricks",
                            &e->legoBricks.lightAngle, "legoBricks.lightAngle",
                            ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_LEGO_BRICKS, LegoBricks, legoBricks, "LEGO Bricks",
                "NOV", 14, EFFECT_FLAG_NONE, SetupLegoBricks, NULL,
                DrawLegoBricksParams)
// clang-format on
