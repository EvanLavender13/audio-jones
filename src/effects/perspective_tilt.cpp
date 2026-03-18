// Perspective Tilt effect module implementation

#include "perspective_tilt.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"

#include "imgui.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

static const float PITCH_YAW_CLAMP = 1.4835f; // ~85 degrees safety clamp
static const float HALF_PI = PI_F / 2.0f;

bool PerspectiveTiltEffectInit(PerspectiveTiltEffect *e) {
  e->shader = LoadShader(NULL, "shaders/perspective_tilt.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->pitchLoc = GetShaderLocation(e->shader, "pitch");
  e->yawLoc = GetShaderLocation(e->shader, "yaw");
  e->rollLoc = GetShaderLocation(e->shader, "roll");
  e->fovLoc = GetShaderLocation(e->shader, "fov");
  e->autoZoomLoc = GetShaderLocation(e->shader, "autoZoom");

  return true;
}

void PerspectiveTiltEffectSetup(const PerspectiveTiltEffect *e,
                                const PerspectiveTiltConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  // Clamp pitch and yaw to +/-85 degrees for numerical safety
  float pitch = fmaxf(-PITCH_YAW_CLAMP, fminf(cfg->pitch, PITCH_YAW_CLAMP));
  float yaw = fmaxf(-PITCH_YAW_CLAMP, fminf(cfg->yaw, PITCH_YAW_CLAMP));

  SetShaderValue(e->shader, e->pitchLoc, &pitch, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->yawLoc, &yaw, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rollLoc, &cfg->roll, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fovLoc, &cfg->fov, SHADER_UNIFORM_FLOAT);

  int autoZoom = cfg->autoZoom ? 1 : 0;
  SetShaderValue(e->shader, e->autoZoomLoc, &autoZoom, SHADER_UNIFORM_INT);
}

void PerspectiveTiltEffectUninit(const PerspectiveTiltEffect *e) {
  UnloadShader(e->shader);
}

void PerspectiveTiltRegisterParams(PerspectiveTiltConfig *cfg) {
  ModEngineRegisterParam("perspectiveTilt.pitch", &cfg->pitch, -HALF_PI,
                         HALF_PI);
  ModEngineRegisterParam("perspectiveTilt.yaw", &cfg->yaw, -HALF_PI, HALF_PI);
  ModEngineRegisterParam("perspectiveTilt.roll", &cfg->roll,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("perspectiveTilt.fov", &cfg->fov, 20.0f, 120.0f);
}

void SetupPerspectiveTilt(PostEffect *pe) {
  PerspectiveTiltEffectSetup(&pe->perspectiveTilt,
                             &pe->effects.perspectiveTilt);
}

// === UI ===

static void DrawPerspectiveTiltParams(EffectConfig *e, const ModSources *ms,
                                      ImU32 glow) {
  (void)glow;
  PerspectiveTiltConfig *pt = &e->perspectiveTilt;

  ModulatableSliderAngleDeg("Pitch##perspTilt", &pt->pitch,
                            "perspectiveTilt.pitch", ms);
  ModulatableSliderAngleDeg("Yaw##perspTilt", &pt->yaw, "perspectiveTilt.yaw",
                            ms);
  ModulatableSliderAngleDeg("Roll##perspTilt", &pt->roll,
                            "perspectiveTilt.roll", ms);
  ModulatableSlider("FOV##perspTilt", &pt->fov, "perspectiveTilt.fov", "%.0f",
                    ms);
  ImGui::Checkbox("Auto Zoom##perspTilt", &pt->autoZoom);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_PERSPECTIVE_TILT, PerspectiveTilt,
                perspectiveTilt, "Perspective Tilt", "OPT", 7,
                EFFECT_FLAG_NONE, SetupPerspectiveTilt, NULL,
                DrawPerspectiveTiltParams)
// clang-format on
