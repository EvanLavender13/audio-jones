// Kuwahara painterly filter effect module implementation

#include "kuwahara.h"

#include "automation/modulation_engine.h"
#include <stddef.h>

bool KuwaharaEffectInit(KuwaharaEffect *e) {
  e->shader = LoadShader(NULL, "shaders/kuwahara.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->radiusLoc = GetShaderLocation(e->shader, "radius");

  return true;
}

void KuwaharaEffectSetup(KuwaharaEffect *e, const KuwaharaConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  int radius = (int)cfg->radius;
  SetShaderValue(e->shader, e->radiusLoc, &radius, SHADER_UNIFORM_INT);
}

void KuwaharaEffectUninit(KuwaharaEffect *e) { UnloadShader(e->shader); }

KuwaharaConfig KuwaharaConfigDefault(void) { return KuwaharaConfig{}; }

void KuwaharaRegisterParams(KuwaharaConfig *cfg) {
  ModEngineRegisterParam("kuwahara.radius", &cfg->radius, 2.0f, 12.0f);
}
