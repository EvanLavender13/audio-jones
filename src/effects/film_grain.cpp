// Film Grain effect - Gaussian noise with signal-to-noise ratio falloff for
// photographic grain

#include "film_grain.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool FilmGrainEffectInit(FilmGrainEffect *e) {
  e->shader = LoadShader(NULL, "shaders/film_grain.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->varianceLoc = GetShaderLocation(e->shader, "variance");
  e->snrLoc = GetShaderLocation(e->shader, "snr");
  e->colorAmountLoc = GetShaderLocation(e->shader, "colorAmount");
  e->time = 0.0f;

  return true;
}

void FilmGrainEffectSetup(FilmGrainEffect *e, const FilmGrainConfig *cfg,
                          float deltaTime) {
  e->time += deltaTime;
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->varianceLoc, &cfg->variance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->snrLoc, &cfg->snr, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorAmountLoc, &cfg->colorAmount,
                 SHADER_UNIFORM_FLOAT);
}

void FilmGrainEffectUninit(const FilmGrainEffect *e) {
  UnloadShader(e->shader);
}

void FilmGrainRegisterParams(FilmGrainConfig *cfg) {
  ModEngineRegisterParam("filmGrain.intensity", &cfg->intensity, 0.0f, 1.0f);
  ModEngineRegisterParam("filmGrain.variance", &cfg->variance, 0.0f, 1.0f);
  ModEngineRegisterParam("filmGrain.snr", &cfg->snr, 0.0f, 16.0f);
  ModEngineRegisterParam("filmGrain.colorAmount", &cfg->colorAmount, 0.0f,
                         1.0f);
}

// === UI ===

static void DrawFilmGrainParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  FilmGrainConfig *fg = &e->filmGrain;

  ModulatableSlider("Intensity##filmGrain", &fg->intensity,
                    "filmGrain.intensity", "%.2f", ms);
  ModulatableSlider("Variance##filmGrain", &fg->variance, "filmGrain.variance",
                    "%.2f", ms);
  ModulatableSlider("SNR##filmGrain", &fg->snr, "filmGrain.snr", "%.1f", ms);
  ModulatableSlider("Color Amount##filmGrain", &fg->colorAmount,
                    "filmGrain.colorAmount", "%.2f", ms);
}

void SetupFilmGrain(PostEffect *pe) {
  FilmGrainEffectSetup(&pe->filmGrain, &pe->effects.filmGrain,
                       pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_FILM_GRAIN, FilmGrain, filmGrain, "Film Grain",
                "RET", 6, EFFECT_FLAG_NONE, SetupFilmGrain, NULL,
                DrawFilmGrainParams)
// clang-format on
