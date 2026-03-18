// Pixelation effect module implementation

#include "pixelation.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool PixelationEffectInit(PixelationEffect *e) {
  e->shader = LoadShader(NULL, "shaders/pixelation.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->cellCountLoc = GetShaderLocation(e->shader, "cellCount");
  e->ditherScaleLoc = GetShaderLocation(e->shader, "ditherScale");
  e->posterizeLevelsLoc = GetShaderLocation(e->shader, "posterizeLevels");

  return true;
}

void PixelationEffectSetup(const PixelationEffect *e,
                           const PixelationConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->cellCountLoc, &cfg->cellCount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ditherScaleLoc, &cfg->ditherScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->posterizeLevelsLoc, &cfg->posterizeLevels,
                 SHADER_UNIFORM_INT);
}

void PixelationEffectUninit(const PixelationEffect *e) {
  UnloadShader(e->shader);
}

void PixelationRegisterParams(PixelationConfig *cfg) {
  ModEngineRegisterParam("pixelation.cellCount", &cfg->cellCount, 4.0f, 256.0f);
  ModEngineRegisterParam("pixelation.ditherScale", &cfg->ditherScale, 1.0f,
                         8.0f);
}

// === UI ===

static void DrawPixelationParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  (void)glow;
  ModulatableSlider("Cell Count##pixel", &e->pixelation.cellCount,
                    "pixelation.cellCount", "%.0f", ms);
  ImGui::SliderInt("Posterize##pixel", &e->pixelation.posterizeLevels, 0, 16);
  if (e->pixelation.posterizeLevels > 0) {
    ModulatableSliderInt("Dither Scale##pixel", &e->pixelation.ditherScale,
                         "pixelation.ditherScale", ms);
  }
}

void SetupPixelation(PostEffect *pe) {
  PixelationEffectSetup(&pe->pixelation, &pe->effects.pixelation);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_PIXELATION, Pixelation, pixelation, "Pixelation",
                "RET", 6, EFFECT_FLAG_NONE, SetupPixelation, NULL,
                DrawPixelationParams)
// clang-format on
