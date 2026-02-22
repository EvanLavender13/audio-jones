#include "radial_streak.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool RadialStreakEffectInit(RadialStreakEffect *e) {
  e->shader = LoadShader(NULL, "shaders/radial_streak.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->streakLengthLoc = GetShaderLocation(e->shader, "streakLength");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");

  return true;
}

void RadialStreakEffectSetup(RadialStreakEffect *e,
                             const RadialStreakConfig *cfg, float deltaTime) {
  (void)deltaTime;

  SetShaderValue(e->shader, e->samplesLoc, &cfg->samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->streakLengthLoc, &cfg->streakLength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
}

void RadialStreakEffectUninit(RadialStreakEffect *e) {
  UnloadShader(e->shader);
}

RadialStreakConfig RadialStreakConfigDefault(void) {
  return RadialStreakConfig{};
}

void RadialStreakRegisterParams(RadialStreakConfig *cfg) {
  ModEngineRegisterParam("radialStreak.streakLength", &cfg->streakLength, 0.0f,
                         1.0f);
  ModEngineRegisterParam("radialStreak.intensity", &cfg->intensity, 0.0f, 1.0f);
}

// === UI ===

static void DrawRadialStreakParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  ImGui::SliderInt("Samples##streak", &e->radialStreak.samples, 8, 32);
  ModulatableSlider("Streak Length##streak", &e->radialStreak.streakLength,
                    "radialStreak.streakLength", "%.2f", ms);
  ModulatableSlider("Intensity##streak", &e->radialStreak.intensity,
                    "radialStreak.intensity", "%.2f", ms);
}

void SetupRadialStreak(PostEffect *pe) {
  RadialStreakEffectSetup(&pe->radialStreak, &pe->effects.radialStreak,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_RADIAL_STREAK, RadialStreak, radialStreak,
                "Radial Blur", "MOT", 3, EFFECT_FLAG_HALF_RES,
                SetupRadialStreak, NULL, DrawRadialStreakParams)
// clang-format on
