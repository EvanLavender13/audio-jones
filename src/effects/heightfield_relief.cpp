// Heightfield Relief effect module implementation

#include "heightfield_relief.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool HeightfieldReliefEffectInit(HeightfieldReliefEffect *e) {
  e->shader = LoadShader(NULL, "shaders/heightfield_relief.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->reliefScaleLoc = GetShaderLocation(e->shader, "reliefScale");
  e->lightAngleLoc = GetShaderLocation(e->shader, "lightAngle");
  e->lightHeightLoc = GetShaderLocation(e->shader, "lightHeight");
  e->shininessLoc = GetShaderLocation(e->shader, "shininess");

  return true;
}

void HeightfieldReliefEffectSetup(HeightfieldReliefEffect *e,
                                  const HeightfieldReliefConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->reliefScaleLoc, &cfg->reliefScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lightAngleLoc, &cfg->lightAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lightHeightLoc, &cfg->lightHeight,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shininessLoc, &cfg->shininess,
                 SHADER_UNIFORM_FLOAT);
}

void HeightfieldReliefEffectUninit(HeightfieldReliefEffect *e) {
  UnloadShader(e->shader);
}

HeightfieldReliefConfig HeightfieldReliefConfigDefault(void) {
  return HeightfieldReliefConfig{};
}

void HeightfieldReliefRegisterParams(HeightfieldReliefConfig *cfg) {
  ModEngineRegisterParam("heightfieldRelief.lightAngle", &cfg->lightAngle, 0.0f,
                         6.28f);
  ModEngineRegisterParam("heightfieldRelief.intensity", &cfg->intensity, 0.0f,
                         1.0f);
}

void SetupHeightfieldRelief(PostEffect *pe) {
  HeightfieldReliefEffectSetup(&pe->heightfieldRelief,
                               &pe->effects.heightfieldRelief);
}

// === UI ===

static void DrawHeightfieldReliefParams(EffectConfig *e, const ModSources *ms,
                                        ImU32 glow) {
  (void)glow;
  HeightfieldReliefConfig *h = &e->heightfieldRelief;

  ModulatableSlider("Intensity##relief", &h->intensity,
                    "heightfieldRelief.intensity", "%.2f", ms);
  ImGui::SliderFloat("Relief Scale##relief", &h->reliefScale, 0.02f, 1.0f,
                     "%.2f");
  ModulatableSliderAngleDeg("Light Angle##relief", &h->lightAngle,
                            "heightfieldRelief.lightAngle", ms);
  ImGui::SliderFloat("Light Height##relief", &h->lightHeight, 0.1f, 2.0f,
                     "%.2f");
  ImGui::SliderFloat("Shininess##relief", &h->shininess, 1.0f, 128.0f, "%.0f");
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_HEIGHTFIELD_RELIEF, HeightfieldRelief,
                heightfieldRelief, "Heightfield Relief", "OPT", 7,
                EFFECT_FLAG_NONE, SetupHeightfieldRelief, NULL,
                DrawHeightfieldReliefParams)
// clang-format on
