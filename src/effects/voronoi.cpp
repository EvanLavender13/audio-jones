// Voronoi multi-effect module implementation

#include "voronoi.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool VoronoiEffectInit(VoronoiEffect *e) {
  e->shader = LoadShader(NULL, "shaders/voronoi.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->edgeFalloffLoc = GetShaderLocation(e->shader, "edgeFalloff");
  e->isoFrequencyLoc = GetShaderLocation(e->shader, "isoFrequency");
  e->smoothModeLoc = GetShaderLocation(e->shader, "smoothMode");
  e->modeLoc = GetShaderLocation(e->shader, "mode");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");

  e->time = 0.0f;

  return true;
}

void VoronoiEffectSetup(VoronoiEffect *e, const VoronoiConfig *cfg,
                        float deltaTime) {
  e->time += cfg->speed * deltaTime;

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeFalloffLoc, &cfg->edgeFalloff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->isoFrequencyLoc, &cfg->isoFrequency,
                 SHADER_UNIFORM_FLOAT);

  int smoothModeInt = cfg->smoothMode ? 1 : 0;
  SetShaderValue(e->shader, e->smoothModeLoc, &smoothModeInt,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
}

void VoronoiEffectUninit(const VoronoiEffect *e) { UnloadShader(e->shader); }

void VoronoiRegisterParams(VoronoiConfig *cfg) {
  ModEngineRegisterParam("voronoi.scale", &cfg->scale, 5.0f, 50.0f);
  ModEngineRegisterParam("voronoi.speed", &cfg->speed, 0.1f, 2.0f);
  ModEngineRegisterParam("voronoi.edgeFalloff", &cfg->edgeFalloff, 0.1f, 1.0f);
  ModEngineRegisterParam("voronoi.isoFrequency", &cfg->isoFrequency, 1.0f,
                         50.0f);
  ModEngineRegisterParam("voronoi.intensity", &cfg->intensity, 0.0f, 1.0f);
}

// === UI ===

static void DrawVoronoiParams(EffectConfig *e, const ModSources *ms,
                              ImU32 glow) {
  (void)glow;
  VoronoiConfig *v = &e->voronoi;

  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Scale##vor", &v->scale, "voronoi.scale", "%.1f", ms);
  ImGui::Checkbox("Smooth##vor", &v->smoothMode);
  ImGui::SeparatorText("Cell Mode");
  ImGui::Combo("Mode##vor", &v->mode,
               "Distort\0Organic Flow\0Edge Iso\0Center Iso\0Flat "
               "Fill\0Edge Glow\0Ratio\0Determinant\0Edge Detect\0");
  ModulatableSlider("Intensity##vor", &v->intensity, "voronoi.intensity",
                    "%.2f", ms);

  if (v->mode == 2 || v->mode == 3) {
    ModulatableSlider("Iso Frequency##vor", &v->isoFrequency,
                      "voronoi.isoFrequency", "%.1f", ms);
  }
  if (v->mode == 1 || v->mode == 4 || v->mode == 5 || v->mode == 8) {
    ModulatableSlider("Edge Falloff##vor", &v->edgeFalloff,
                      "voronoi.edgeFalloff", "%.2f", ms);
  }
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Speed##vor", &v->speed, "voronoi.speed", "%.2f", ms);
}

VoronoiEffect *GetVoronoiEffect(PostEffect *pe) {
  return (VoronoiEffect *)pe->effectStates[TRANSFORM_VORONOI];
}

void SetupVoronoi(PostEffect *pe) {
  VoronoiEffectSetup(GetVoronoiEffect(pe), &pe->effects.voronoi,
                     pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_VORONOI, Voronoi, voronoi, "Voronoi", "CELL", 2,
                EFFECT_FLAG_NONE, SetupVoronoi, NULL, DrawVoronoiParams)
// clang-format on
