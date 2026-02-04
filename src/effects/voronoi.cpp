// Voronoi multi-effect module implementation

#include "voronoi.h"
#include <stdlib.h>

bool VoronoiEffectInit(VoronoiEffect *e) {
  if (e == NULL) {
    return false;
  }

  e->shader = LoadShader(NULL, "shaders/voronoi.fs");
  if (!IsShaderValid(e->shader)) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->edgeFalloffLoc = GetShaderLocation(e->shader, "edgeFalloff");
  e->isoFrequencyLoc = GetShaderLocation(e->shader, "isoFrequency");
  e->smoothModeLoc = GetShaderLocation(e->shader, "smoothMode");
  e->uvDistortIntensityLoc = GetShaderLocation(e->shader, "uvDistortIntensity");
  e->edgeIsoIntensityLoc = GetShaderLocation(e->shader, "edgeIsoIntensity");
  e->centerIsoIntensityLoc = GetShaderLocation(e->shader, "centerIsoIntensity");
  e->flatFillIntensityLoc = GetShaderLocation(e->shader, "flatFillIntensity");
  e->organicFlowIntensityLoc =
      GetShaderLocation(e->shader, "organicFlowIntensity");
  e->edgeGlowIntensityLoc = GetShaderLocation(e->shader, "edgeGlowIntensity");
  e->determinantIntensityLoc =
      GetShaderLocation(e->shader, "determinantIntensity");
  e->ratioIntensityLoc = GetShaderLocation(e->shader, "ratioIntensity");
  e->edgeDetectIntensityLoc =
      GetShaderLocation(e->shader, "edgeDetectIntensity");

  e->time = 0.0f;

  return true;
}

void VoronoiEffectSetup(VoronoiEffect *e, const VoronoiConfig *cfg,
                        float deltaTime) {
  if (e == NULL || cfg == NULL) {
    return;
  }

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

  SetShaderValue(e->shader, e->uvDistortIntensityLoc, &cfg->uvDistortIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeIsoIntensityLoc, &cfg->edgeIsoIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->centerIsoIntensityLoc, &cfg->centerIsoIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flatFillIntensityLoc, &cfg->flatFillIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->organicFlowIntensityLoc,
                 &cfg->organicFlowIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeGlowIntensityLoc, &cfg->edgeGlowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->determinantIntensityLoc,
                 &cfg->determinantIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ratioIntensityLoc, &cfg->ratioIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeDetectIntensityLoc,
                 &cfg->edgeDetectIntensity, SHADER_UNIFORM_FLOAT);
}

void VoronoiEffectUninit(VoronoiEffect *e) {
  if (e == NULL) {
    return;
  }

  UnloadShader(e->shader);
}

VoronoiConfig VoronoiConfigDefault(void) {
  VoronoiConfig cfg = {};
  cfg.enabled = false;
  cfg.smoothMode = false;
  cfg.scale = 15.0f;
  cfg.speed = 0.5f;
  cfg.edgeFalloff = 0.3f;
  cfg.isoFrequency = 10.0f;
  cfg.uvDistortIntensity = 0.0f;
  cfg.edgeIsoIntensity = 0.0f;
  cfg.centerIsoIntensity = 0.0f;
  cfg.flatFillIntensity = 0.0f;
  cfg.organicFlowIntensity = 0.0f;
  cfg.edgeGlowIntensity = 0.0f;
  cfg.determinantIntensity = 0.0f;
  cfg.ratioIntensity = 0.0f;
  cfg.edgeDetectIntensity = 0.0f;
  return cfg;
}
