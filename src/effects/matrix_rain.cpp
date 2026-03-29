// Matrix rain effect module implementation

#include "matrix_rain.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
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
  // CPU time accumulation - avoids position jumps when rainSpeed changes
  e->time += cfg->rainSpeed * deltaTime;

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
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

void MatrixRainEffectUninit(const MatrixRainEffect *e) {
  UnloadShader(e->shader);
}

void MatrixRainRegisterParams(MatrixRainConfig *cfg) {
  ModEngineRegisterParam("matrixRain.rainSpeed", &cfg->rainSpeed, 0.1f, 5.0f);
  ModEngineRegisterParam("matrixRain.overlayIntensity", &cfg->overlayIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("matrixRain.trailLength", &cfg->trailLength, 5.0f,
                         40.0f);
  ModEngineRegisterParam("matrixRain.leadBrightness", &cfg->leadBrightness,
                         0.5f, 3.0f);
}

// === UI ===

static void DrawMatrixRainParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  (void)glow;
  MatrixRainConfig *mr = &e->matrixRain;

  ImGui::SeparatorText("Geometry");
  ImGui::SliderFloat("Cell Size##matrixrain", &mr->cellSize, 4.0f, 32.0f,
                     "%.0f px");
  ImGui::SliderInt("Faller Count##matrixrain", &mr->fallerCount, 1, 20);
  ImGui::SeparatorText("Trail");
  ModulatableSlider("Trail Length##matrixrain", &mr->trailLength,
                    "matrixRain.trailLength", "%.0f", ms);
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Rain Speed##matrixrain", &mr->rainSpeed,
                    "matrixRain.rainSpeed", "%.2f", ms);
  ImGui::SliderFloat("Refresh Rate##matrixrain", &mr->refreshRate, 0.1f, 5.0f,
                     "%.2f");
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Lead Brightness##matrixrain", &mr->leadBrightness,
                    "matrixRain.leadBrightness", "%.2f", ms);
  ImGui::SeparatorText("Color");
  ModulatableSlider("Overlay Intensity##matrixrain", &mr->overlayIntensity,
                    "matrixRain.overlayIntensity", "%.2f", ms);
  ImGui::Checkbox("Sample##matrixrain", &mr->sampleMode);
}

void SetupMatrixRain(PostEffect *pe) {
  MatrixRainEffectSetup(&pe->matrixRain, &pe->effects.matrixRain,
                        pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_MATRIX_RAIN, MatrixRain, matrixRain, "Matrix Rain",
                "RET", 6, EFFECT_FLAG_NONE, SetupMatrixRain, NULL,
                DrawMatrixRainParams)
// clang-format on
