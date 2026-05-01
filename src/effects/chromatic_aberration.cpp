// Chromatic aberration effect module implementation

#include "chromatic_aberration.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"

#include "imgui.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"

#include <stddef.h>

bool ChromaticAberrationEffectInit(ChromaticAberrationEffect *e) {
  e->shader = LoadShader(NULL, "shaders/chromatic_aberration.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->offsetLoc = GetShaderLocation(e->shader, "offset");
  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->falloffLoc = GetShaderLocation(e->shader, "falloff");

  return true;
}

void ChromaticAberrationEffectSetup(const ChromaticAberrationEffect *e,
                                    const ChromaticAberrationConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->offsetLoc, &cfg->offset, SHADER_UNIFORM_FLOAT);
  int samples = (int)cfg->samples;
  SetShaderValue(e->shader, e->samplesLoc, &samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->falloffLoc, &cfg->falloff, SHADER_UNIFORM_FLOAT);
}

void ChromaticAberrationEffectUninit(const ChromaticAberrationEffect *e) {
  UnloadShader(e->shader);
}

void ChromaticAberrationRegisterParams(ChromaticAberrationConfig *cfg) {
  ModEngineRegisterParam("chromaticAberration.offset", &cfg->offset, 0.0f,
                         50.0f);
  ModEngineRegisterParam("chromaticAberration.samples", &cfg->samples, 3.0f,
                         24.0f);
  ModEngineRegisterParam("chromaticAberration.falloff", &cfg->falloff, 0.5f,
                         3.0f);
}

ChromaticAberrationEffect *GetChromaticAberrationEffect(PostEffect *pe) {
  return (ChromaticAberrationEffect *)
      pe->effectStates[TRANSFORM_CHROMATIC_ABERRATION];
}

void SetupChromaticAberration(PostEffect *pe) {
  ChromaticAberrationEffectSetup(GetChromaticAberrationEffect(pe),
                                 &pe->effects.chromaticAberration);
}

// === UI ===

static void DrawChromaticAberrationParams(EffectConfig *e, const ModSources *ms,
                                          ImU32 glow) {
  (void)glow;
  ChromaticAberrationConfig *c = &e->chromaticAberration;

  ModulatableSlider("Offset##chromaticAberration", &c->offset,
                    "chromaticAberration.offset", "%.0f px", ms);
  ModulatableSliderInt("Samples##chromaticAberration", &c->samples,
                       "chromaticAberration.samples", ms);
  ModulatableSlider("Falloff##chromaticAberration", &c->falloff,
                    "chromaticAberration.falloff", "%.2f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_CHROMATIC_ABERRATION, ChromaticAberration, chromaticAberration,
                "Chromatic Aberration", "OPT", 7, EFFECT_FLAG_NONE,
                SetupChromaticAberration, NULL, DrawChromaticAberrationParams)
// clang-format on
