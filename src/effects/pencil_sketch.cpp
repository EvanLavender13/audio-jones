// Pencil sketch effect module implementation

#include "pencil_sketch.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/noise_texture.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool PencilSketchEffectInit(PencilSketchEffect *e) {
  e->shader = LoadShader(NULL, "shaders/pencil_sketch.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->angleCountLoc = GetShaderLocation(e->shader, "angleCount");
  e->sampleCountLoc = GetShaderLocation(e->shader, "sampleCount");
  e->gradientEpsLoc = GetShaderLocation(e->shader, "gradientEps");
  e->desaturationLoc = GetShaderLocation(e->shader, "desaturation");
  e->toneCapLoc = GetShaderLocation(e->shader, "toneCap");
  e->noiseInfluenceLoc = GetShaderLocation(e->shader, "noiseInfluence");
  e->colorStrengthLoc = GetShaderLocation(e->shader, "colorStrength");
  e->paperStrengthLoc = GetShaderLocation(e->shader, "paperStrength");
  e->vignetteStrengthLoc = GetShaderLocation(e->shader, "vignetteStrength");
  e->wobbleTimeLoc = GetShaderLocation(e->shader, "wobbleTime");
  e->wobbleAmountLoc = GetShaderLocation(e->shader, "wobbleAmount");
  e->noiseTexLoc = GetShaderLocation(e->shader, "texture1");

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
  SetShaderValue(e->shader, e->gradientEpsLoc, &cfg->gradientEps,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->desaturationLoc, &cfg->desaturation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->toneCapLoc, &cfg->toneCap, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseInfluenceLoc, &cfg->noiseInfluence,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorStrengthLoc, &cfg->colorStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->paperStrengthLoc, &cfg->paperStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->vignetteStrengthLoc, &cfg->vignetteStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wobbleTimeLoc, &e->wobbleTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wobbleAmountLoc, &cfg->wobbleAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->noiseTexLoc, NoiseTextureGet());
}

void PencilSketchEffectUninit(PencilSketchEffect *e) {
  UnloadShader(e->shader);
}

PencilSketchConfig PencilSketchConfigDefault(void) {
  return PencilSketchConfig{};
}

void PencilSketchRegisterParams(PencilSketchConfig *cfg) {
  ModEngineRegisterParam("pencilSketch.gradientEps", &cfg->gradientEps, 0.2f,
                         1.0f);
  ModEngineRegisterParam("pencilSketch.desaturation", &cfg->desaturation, 0.0f,
                         3.0f);
  ModEngineRegisterParam("pencilSketch.toneCap", &cfg->toneCap, 0.3f, 1.0f);
  ModEngineRegisterParam("pencilSketch.noiseInfluence", &cfg->noiseInfluence,
                         0.0f, 1.0f);
  ModEngineRegisterParam("pencilSketch.colorStrength", &cfg->colorStrength,
                         0.0f, 1.0f);
  ModEngineRegisterParam("pencilSketch.paperStrength", &cfg->paperStrength,
                         0.0f, 1.0f);
  ModEngineRegisterParam("pencilSketch.vignetteStrength",
                         &cfg->vignetteStrength, 0.0f, 1.0f);
  ModEngineRegisterParam("pencilSketch.wobbleAmount", &cfg->wobbleAmount, 0.0f,
                         8.0f);
}

void SetupPencilSketch(PostEffect *pe) {
  PencilSketchEffectSetup(&pe->pencilSketch, &pe->effects.pencilSketch,
                          pe->currentDeltaTime);
}

// === UI ===

static void DrawPencilSketchParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  PencilSketchConfig *ps = &e->pencilSketch;

  ImGui::SliderInt("Angle Count##pencilsketch", &ps->angleCount, 2, 6);
  ImGui::SliderInt("Sample Count##pencilsketch", &ps->sampleCount, 8, 24);
  ModulatableSlider("Gradient Eps##pencilsketch", &ps->gradientEps,
                    "pencilSketch.gradientEps", "%.2f", ms);
  ModulatableSlider("Desaturation##pencilsketch", &ps->desaturation,
                    "pencilSketch.desaturation", "%.2f", ms);
  ModulatableSlider("Tone Cap##pencilsketch", &ps->toneCap,
                    "pencilSketch.toneCap", "%.2f", ms);
  ModulatableSlider("Noise Influence##pencilsketch", &ps->noiseInfluence,
                    "pencilSketch.noiseInfluence", "%.2f", ms);
  ModulatableSlider("Color Strength##pencilsketch", &ps->colorStrength,
                    "pencilSketch.colorStrength", "%.2f", ms);
  ModulatableSlider("Paper Strength##pencilsketch", &ps->paperStrength,
                    "pencilSketch.paperStrength", "%.2f", ms);
  ModulatableSlider("Vignette##pencilsketch", &ps->vignetteStrength,
                    "pencilSketch.vignetteStrength", "%.2f", ms);

  if (TreeNodeAccented("Animation##pencilsketch", glow)) {
    ImGui::SliderFloat("Wobble Speed##pencilsketch", &ps->wobbleSpeed, 0.0f,
                       2.0f, "%.2f");
    ModulatableSlider("Wobble Amount##pencilsketch", &ps->wobbleAmount,
                      "pencilSketch.wobbleAmount", "%.1f px", ms);
    TreeNodeAccentedPop();
  }
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_PENCIL_SKETCH, PencilSketch, pencilSketch,
                "Pencil Sketch", "ART", 4, EFFECT_FLAG_NONE,
                SetupPencilSketch, NULL, DrawPencilSketchParams)
// clang-format on
