#include "mobius.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool MobiusEffectInit(MobiusEffect *e) {
  e->shader = LoadShader(NULL, "shaders/mobius.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->point1Loc = GetShaderLocation(e->shader, "point1");
  e->point2Loc = GetShaderLocation(e->shader, "point2");
  e->spiralTightnessLoc = GetShaderLocation(e->shader, "spiralTightness");
  e->zoomFactorLoc = GetShaderLocation(e->shader, "zoomFactor");
  e->armCountLoc = GetShaderLocation(e->shader, "armCount");
  e->spiralOffsetLoc = GetShaderLocation(e->shader, "spiralOffset");

  e->time = 0.0f;
  e->currentPoint1[0] = 0.0f;
  e->currentPoint1[1] = 0.0f;
  e->currentPoint2[0] = 0.0f;
  e->currentPoint2[1] = 0.0f;

  return true;
}

void MobiusEffectSetup(MobiusEffect *e, MobiusConfig *cfg, float deltaTime) {
  e->time += cfg->speed * deltaTime;

  // Compute point1 via Lissajous
  float offset1X;
  float offset1Y;
  DualLissajousUpdate(&cfg->point1Lissajous, deltaTime, 0.0f, &offset1X,
                      &offset1Y);
  e->currentPoint1[0] = cfg->point1X + offset1X;
  e->currentPoint1[1] = cfg->point1Y + offset1Y;

  // Compute point2 via Lissajous
  float offset2X;
  float offset2Y;
  DualLissajousUpdate(&cfg->point2Lissajous, deltaTime, 0.0f, &offset2X,
                      &offset2Y);
  e->currentPoint2[0] = cfg->point2X + offset2X;
  e->currentPoint2[1] = cfg->point2Y + offset2Y;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->point1Loc, e->currentPoint1,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->point2Loc, e->currentPoint2,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->spiralTightnessLoc, &cfg->spiralTightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomFactorLoc, &cfg->zoomFactor,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->armCountLoc, &cfg->armCount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralOffsetLoc, &cfg->spiralOffset,
                 SHADER_UNIFORM_FLOAT);
}

void MobiusEffectUninit(const MobiusEffect *e) { UnloadShader(e->shader); }

void MobiusRegisterParams(MobiusConfig *cfg) {
  ModEngineRegisterParam("mobius.spiralTightness", &cfg->spiralTightness, -2.0f,
                         2.0f);
  ModEngineRegisterParam("mobius.zoomFactor", &cfg->zoomFactor, -2.0f, 2.0f);
  ModEngineRegisterParam("mobius.armCount", &cfg->armCount, 1.0f, 12.0f);
  ModEngineRegisterParam("mobius.spiralOffset", &cfg->spiralOffset, -1.0f,
                         1.0f);
  ModEngineRegisterParam("mobius.speed", &cfg->speed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("mobius.point1X", &cfg->point1X, 0.0f, 1.0f);
  ModEngineRegisterParam("mobius.point1Y", &cfg->point1Y, 0.0f, 1.0f);
  ModEngineRegisterParam("mobius.point2X", &cfg->point2X, 0.0f, 1.0f);
  ModEngineRegisterParam("mobius.point2Y", &cfg->point2Y, 0.0f, 1.0f);
  ModEngineRegisterParam("mobius.point1Lissajous.amplitude",
                         &cfg->point1Lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("mobius.point1Lissajous.motionSpeed",
                         &cfg->point1Lissajous.motionSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("mobius.point2Lissajous.amplitude",
                         &cfg->point2Lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("mobius.point2Lissajous.motionSpeed",
                         &cfg->point2Lissajous.motionSpeed, 0.0f, 10.0f);
}

// === UI ===

static void DrawMobiusParams(EffectConfig *e, const ModSources *ms,
                             ImU32 glow) {
  (void)glow;
  ModulatableSlider("Spiral Tightness##mobius", &e->mobius.spiralTightness,
                    "mobius.spiralTightness", "%.2f", ms);
  ModulatableSlider("Zoom Factor##mobius", &e->mobius.zoomFactor,
                    "mobius.zoomFactor", "%.2f", ms);
  ModulatableSlider("Arm Count##mobius", &e->mobius.armCount, "mobius.armCount",
                    "%.1f", ms);
  ModulatableSlider("Spiral Offset##mobius", &e->mobius.spiralOffset,
                    "mobius.spiralOffset", "%.2f", ms);
  ModulatableSliderSpeedDeg("Speed##mobius", &e->mobius.speed, "mobius.speed",
                            ms);
  ImGui::SeparatorText("Fixed Points");
  ModulatableSlider("Point 1 X##mobius", &e->mobius.point1X, "mobius.point1X",
                    "%.2f", ms);
  ModulatableSlider("Point 1 Y##mobius", &e->mobius.point1Y, "mobius.point1Y",
                    "%.2f", ms);
  ModulatableSlider("Point 2 X##mobius", &e->mobius.point2X, "mobius.point2X",
                    "%.2f", ms);
  ModulatableSlider("Point 2 Y##mobius", &e->mobius.point2Y, "mobius.point2Y",
                    "%.2f", ms);
  ImGui::SeparatorText("Point 1 Motion");
  DrawLissajousControls(&e->mobius.point1Lissajous, "mobius_p1",
                        "mobius.point1Lissajous", ms, 5.0f);
  ImGui::SeparatorText("Point 2 Motion");
  DrawLissajousControls(&e->mobius.point2Lissajous, "mobius_p2",
                        "mobius.point2Lissajous", ms, 5.0f);
}

void SetupMobius(PostEffect *pe) {
  MobiusEffectSetup(&pe->mobius, &pe->effects.mobius, pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_MOBIUS, Mobius, mobius, "Mobius", "WARP", 1,
                EFFECT_FLAG_NONE, SetupMobius, NULL, DrawMobiusParams)
// clang-format on
