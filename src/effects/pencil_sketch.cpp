// Pencil sketch effect module implementation

#include "pencil_sketch.h"
#include "automation/modulation_engine.h"
#include <stddef.h>

bool PencilSketchEffectInit(PencilSketchEffect *e) {
  e->shader = LoadShader(NULL, "shaders/pencil_sketch.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->angleCountLoc = GetShaderLocation(e->shader, "angleCount");
  e->sampleCountLoc = GetShaderLocation(e->shader, "sampleCount");
  e->strokeFalloffLoc = GetShaderLocation(e->shader, "strokeFalloff");
  e->gradientEpsLoc = GetShaderLocation(e->shader, "gradientEps");
  e->paperStrengthLoc = GetShaderLocation(e->shader, "paperStrength");
  e->vignetteStrengthLoc = GetShaderLocation(e->shader, "vignetteStrength");
  e->wobbleTimeLoc = GetShaderLocation(e->shader, "wobbleTime");
  e->wobbleAmountLoc = GetShaderLocation(e->shader, "wobbleAmount");

  e->wobbleTime = 0.0f;

  return true;
}

void PencilSketchEffectSetup(PencilSketchEffect *e,
                             const PencilSketchConfig *cfg, float deltaTime) {
  e->wobbleTime += cfg->wobbleSpeed * deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->angleCountLoc, &cfg->angleCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->sampleCountLoc, &cfg->sampleCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->strokeFalloffLoc, &cfg->strokeFalloff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gradientEpsLoc, &cfg->gradientEps,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->paperStrengthLoc, &cfg->paperStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->vignetteStrengthLoc, &cfg->vignetteStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wobbleTimeLoc, &e->wobbleTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wobbleAmountLoc, &cfg->wobbleAmount,
                 SHADER_UNIFORM_FLOAT);
}

void PencilSketchEffectUninit(PencilSketchEffect *e) {
  UnloadShader(e->shader);
}

PencilSketchConfig PencilSketchConfigDefault(void) {
  return PencilSketchConfig{};
}

void PencilSketchRegisterParams(PencilSketchConfig *cfg) {
  ModEngineRegisterParam("pencilSketch.strokeFalloff", &cfg->strokeFalloff,
                         0.0f, 1.0f);
  ModEngineRegisterParam("pencilSketch.paperStrength", &cfg->paperStrength,
                         0.0f, 1.0f);
  ModEngineRegisterParam("pencilSketch.vignetteStrength",
                         &cfg->vignetteStrength, 0.0f, 1.0f);
  ModEngineRegisterParam("pencilSketch.wobbleAmount", &cfg->wobbleAmount, 0.0f,
                         8.0f);
}
