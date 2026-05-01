#include "escher_droste.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/dual_lissajous_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool EscherDrosteEffectInit(EscherDrosteEffect *e) {
  e->shader = LoadShader(NULL, "shaders/escher_droste.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->zoomPhaseLoc = GetShaderLocation(e->shader, "zoomPhase");
  e->spiralStrengthLoc = GetShaderLocation(e->shader, "spiralStrength");
  e->rotationOffsetLoc = GetShaderLocation(e->shader, "rotationOffset");
  e->innerRadiusLoc = GetShaderLocation(e->shader, "innerRadius");

  e->zoomPhase = 0.0f;

  return true;
}

void EscherDrosteEffectSetup(EscherDrosteEffect *e, EscherDrosteConfig *cfg,
                             float deltaTime) {
  e->zoomPhase += cfg->zoomSpeed * deltaTime;

  const float w = (float)GetScreenWidth();
  const float h = (float)GetScreenHeight();

  float cx = 0.0f;
  float cy = 0.0f;
  DualLissajousUpdate(&cfg->center, deltaTime, 0.0f, &cx, &cy);

  const float centerPix[2] = {cx * h, cy * h};
  const float resolution[2] = {w, h};

  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->centerLoc, centerPix, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomPhaseLoc, &e->zoomPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralStrengthLoc, &cfg->spiralStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationOffsetLoc, &cfg->rotationOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->innerRadiusLoc, &cfg->innerRadius,
                 SHADER_UNIFORM_FLOAT);
}

void EscherDrosteEffectUninit(const EscherDrosteEffect *e) {
  UnloadShader(e->shader);
}

void EscherDrosteRegisterParams(EscherDrosteConfig *cfg) {
  ModEngineRegisterParam("escherDroste.scale", &cfg->scale, 4.0f, 1024.0f);
  ModEngineRegisterParam("escherDroste.zoomSpeed", &cfg->zoomSpeed, -2.0f,
                         2.0f);
  ModEngineRegisterParam("escherDroste.spiralStrength", &cfg->spiralStrength,
                         -2.0f, 2.0f);
  ModEngineRegisterParam("escherDroste.rotationOffset", &cfg->rotationOffset,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("escherDroste.center.amplitude",
                         &cfg->center.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("escherDroste.center.motionSpeed",
                         &cfg->center.motionSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("escherDroste.innerRadius", &cfg->innerRadius, 0.0f,
                         0.5f);
}

// === UI ===

static void DrawEscherDrosteParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  EscherDrosteConfig *cfg = &e->escherDroste;

  ImGui::SeparatorText("Geometry");
  ModulatableSliderLog("Scale##escherDroste", &cfg->scale, "escherDroste.scale",
                       "%.0f", ms);

  ImGui::SeparatorText("Warp");
  ModulatableSlider("Spiral Strength##escherDroste", &cfg->spiralStrength,
                    "escherDroste.spiralStrength", "%.2f", ms);
  ModulatableSliderAngleDeg("Rotation##escherDroste", &cfg->rotationOffset,
                            "escherDroste.rotationOffset", ms);

  ImGui::SeparatorText("Animation");
  ModulatableSlider("Zoom Speed##escherDroste", &cfg->zoomSpeed,
                    "escherDroste.zoomSpeed", "%.2f", ms);

  ImGui::SeparatorText("Center");
  DrawLissajousControls(&cfg->center, "escherDroste_center",
                        "escherDroste.center", ms, 5.0f);

  ImGui::SeparatorText("Masking");
  ModulatableSlider("Inner Radius##escherDroste", &cfg->innerRadius,
                    "escherDroste.innerRadius", "%.2f", ms);
}

EscherDrosteEffect *GetEscherDrosteEffect(PostEffect *pe) {
  return (EscherDrosteEffect *)pe->effectStates[TRANSFORM_ESCHER_DROSTE];
}

void SetupEscherDroste(PostEffect *pe) {
  EscherDrosteEffectSetup(GetEscherDrosteEffect(pe), &pe->effects.escherDroste,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_ESCHER_DROSTE, EscherDroste, escherDroste,
                "Escher Droste", "MOT", 3, EFFECT_FLAG_NONE,
                SetupEscherDroste, NULL, DrawEscherDrosteParams)
// clang-format on
