// Voronoi multi-effect module implementation

#include "voronoi.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
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

void VoronoiEffectUninit(VoronoiEffect *e) { UnloadShader(e->shader); }

VoronoiConfig VoronoiConfigDefault(void) { return VoronoiConfig{}; }

void VoronoiRegisterParams(VoronoiConfig *cfg) {
  ModEngineRegisterParam("voronoi.scale", &cfg->scale, 5.0f, 50.0f);
  ModEngineRegisterParam("voronoi.speed", &cfg->speed, 0.1f, 2.0f);
  ModEngineRegisterParam("voronoi.edgeFalloff", &cfg->edgeFalloff, 0.1f, 1.0f);
  ModEngineRegisterParam("voronoi.isoFrequency", &cfg->isoFrequency, 1.0f,
                         50.0f);
  ModEngineRegisterParam("voronoi.intensity", &cfg->intensity, 0.0f, 1.0f);
}

void SetupVoronoi(PostEffect *pe) {
  VoronoiEffectSetup(&pe->voronoi, &pe->effects.voronoi, pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_VORONOI, Voronoi, voronoi, "Voronoi", "CELL", 2,
                EFFECT_FLAG_NONE, SetupVoronoi, NULL)
// clang-format on
