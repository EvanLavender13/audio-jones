// Matrix rain effect module implementation

#include "matrix_rain.h"
#include <stddef.h>

bool MatrixRainEffectInit(MatrixRainEffect *e) {
  e->shader = LoadShader(NULL, "shaders/matrix_rain.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->cellSizeLoc = GetShaderLocation(e->shader, "cellSize");
  e->trailLengthLoc = GetShaderLocation(e->shader, "trailLength");
  e->fallerCountLoc = GetShaderLocation(e->shader, "fallerCount");
  e->overlayIntensityLoc = GetShaderLocation(e->shader, "overlayIntensity");
  e->refreshRateLoc = GetShaderLocation(e->shader, "refreshRate");
  e->leadBrightnessLoc = GetShaderLocation(e->shader, "leadBrightness");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->sampleModeLoc = GetShaderLocation(e->shader, "sampleMode");

  e->time = 0.0f;

  return true;
}

void MatrixRainEffectSetup(MatrixRainEffect *e, const MatrixRainConfig *cfg,
                           float deltaTime) {
  // CPU time accumulation — avoids position jumps when rainSpeed changes
  e->time += cfg->rainSpeed * deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->cellSizeLoc, &cfg->cellSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->trailLengthLoc, &cfg->trailLength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fallerCountLoc, &cfg->fallerCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->overlayIntensityLoc, &cfg->overlayIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->refreshRateLoc, &cfg->refreshRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->leadBrightnessLoc, &cfg->leadBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  const int sampleModeInt = cfg->sampleMode ? 1 : 0;
  SetShaderValue(e->shader, e->sampleModeLoc, &sampleModeInt,
                 SHADER_UNIFORM_INT);
}

void MatrixRainEffectUninit(MatrixRainEffect *e) { UnloadShader(e->shader); }

MatrixRainConfig MatrixRainConfigDefault(void) { return MatrixRainConfig{}; }
