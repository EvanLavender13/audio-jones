// Shard Crush effect module implementation

#include "shard_crush.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <math.h>
#include <stddef.h>

bool ShardCrushEffectInit(ShardCrushEffect *e) {
  e->shader = LoadShader(NULL, "shaders/shard_crush.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->zoomLoc = GetShaderLocation(e->shader, "zoom");
  e->aberrationSpreadLoc = GetShaderLocation(e->shader, "aberrationSpread");
  e->noiseScaleLoc = GetShaderLocation(e->shader, "noiseScale");
  e->rotationLevelsLoc = GetShaderLocation(e->shader, "rotationLevels");
  e->softnessLoc = GetShaderLocation(e->shader, "softness");
  e->mixLoc = GetShaderLocation(e->shader, "mixAmount");
  e->time = 0.0f;

  return true;
}

void ShardCrushEffectSetup(ShardCrushEffect *e, const ShardCrushConfig *cfg,
                           float deltaTime) {
  e->time += cfg->speed * deltaTime;
  e->time = fmodf(e->time, 1000.0f);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->zoomLoc, &cfg->zoom, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->aberrationSpreadLoc, &cfg->aberrationSpread,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseScaleLoc, &cfg->noiseScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLevelsLoc, &cfg->rotationLevels,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->softnessLoc, &cfg->softness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->mixLoc, &cfg->mix, SHADER_UNIFORM_FLOAT);
}

void ShardCrushEffectUninit(const ShardCrushEffect *e) {
  UnloadShader(e->shader);
}

void ShardCrushRegisterParams(ShardCrushConfig *cfg) {
  ModEngineRegisterParam("shardCrush.zoom", &cfg->zoom, 0.1f, 2.0f);
  ModEngineRegisterParam("shardCrush.aberrationSpread", &cfg->aberrationSpread,
                         0.001f, 0.05f);
  ModEngineRegisterParam("shardCrush.noiseScale", &cfg->noiseScale, 16.0f,
                         256.0f);
  ModEngineRegisterParam("shardCrush.rotationLevels", &cfg->rotationLevels,
                         2.0f, 16.0f);
  ModEngineRegisterParam("shardCrush.softness", &cfg->softness, 0.0f, 1.0f);
  ModEngineRegisterParam("shardCrush.speed", &cfg->speed, 0.1f, 5.0f);
  ModEngineRegisterParam("shardCrush.mix", &cfg->mix, 0.0f, 1.0f);
}

// === UI ===

static void DrawShardCrushParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  (void)glow;
  ShardCrushConfig *sc = &e->shardCrush;

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##shardCrush", &sc->iterations, 20, 100);
  ModulatableSlider("Zoom##shardCrush", &sc->zoom, "shardCrush.zoom", "%.2f",
                    ms);
  ModulatableSlider("Noise Scale##shardCrush", &sc->noiseScale,
                    "shardCrush.noiseScale", "%.1f", ms);
  ModulatableSlider("Rotation Levels##shardCrush", &sc->rotationLevels,
                    "shardCrush.rotationLevels", "%.1f", ms);
  ModulatableSlider("Softness##shardCrush", &sc->softness,
                    "shardCrush.softness", "%.2f", ms);

  ImGui::SeparatorText("Animation");
  ModulatableSlider("Speed##shardCrush", &sc->speed, "shardCrush.speed", "%.2f",
                    ms);

  ImGui::SeparatorText("Color");
  ModulatableSlider("Aberration##shardCrush", &sc->aberrationSpread,
                    "shardCrush.aberrationSpread", "%.3f", ms);
  ModulatableSlider("Mix##shardCrush", &sc->mix, "shardCrush.mix", "%.2f", ms);
}

ShardCrushEffect *GetShardCrushEffect(PostEffect *pe) {
  return (ShardCrushEffect *)pe->effectStates[TRANSFORM_SHARD_CRUSH];
}

void SetupShardCrush(PostEffect *pe) {
  ShardCrushEffectSetup(GetShardCrushEffect(pe), &pe->effects.shardCrush,
                        pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_SHARD_CRUSH, ShardCrush, shardCrush,
                "Shard Crush", "RET", 6, EFFECT_FLAG_NONE,
                SetupShardCrush, NULL, DrawShardCrushParams)
// clang-format on
