// Impressionist effect module implementation

#include "impressionist.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool ImpressionistEffectInit(ImpressionistEffect *e) {
  e->shader = LoadShader(NULL, "shaders/impressionist.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->splatCountLoc = GetShaderLocation(e->shader, "splatCount");
  e->splatSizeMinLoc = GetShaderLocation(e->shader, "splatSizeMin");
  e->splatSizeMaxLoc = GetShaderLocation(e->shader, "splatSizeMax");
  e->strokeFreqLoc = GetShaderLocation(e->shader, "strokeFreq");
  e->strokeOpacityLoc = GetShaderLocation(e->shader, "strokeOpacity");
  e->outlineStrengthLoc = GetShaderLocation(e->shader, "outlineStrength");
  e->edgeStrengthLoc = GetShaderLocation(e->shader, "edgeStrength");
  e->edgeMaxDarkenLoc = GetShaderLocation(e->shader, "edgeMaxDarken");
  e->grainScaleLoc = GetShaderLocation(e->shader, "grainScale");
  e->grainAmountLoc = GetShaderLocation(e->shader, "grainAmount");
  e->exposureLoc = GetShaderLocation(e->shader, "exposure");

  return true;
}

void ImpressionistEffectSetup(const ImpressionistEffect *e,
                              const ImpressionistConfig *cfg) {
  SetShaderValue(e->shader, e->splatCountLoc, &cfg->splatCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->splatSizeMinLoc, &cfg->splatSizeMin,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->splatSizeMaxLoc, &cfg->splatSizeMax,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strokeFreqLoc, &cfg->strokeFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strokeOpacityLoc, &cfg->strokeOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->outlineStrengthLoc, &cfg->outlineStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeStrengthLoc, &cfg->edgeStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeMaxDarkenLoc, &cfg->edgeMaxDarken,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainScaleLoc, &cfg->grainScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainAmountLoc, &cfg->grainAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->exposureLoc, &cfg->exposure,
                 SHADER_UNIFORM_FLOAT);
}

void ImpressionistEffectUninit(const ImpressionistEffect *e) {
  UnloadShader(e->shader);
}

void ImpressionistRegisterParams(ImpressionistConfig *cfg) {
  ModEngineRegisterParam("impressionist.splatSizeMax", &cfg->splatSizeMax,
                         0.05f, 0.25f);
  ModEngineRegisterParam("impressionist.strokeFreq", &cfg->strokeFreq, 400.0f,
                         2000.0f);
  ModEngineRegisterParam("impressionist.edgeStrength", &cfg->edgeStrength, 0.0f,
                         8.0f);
  ModEngineRegisterParam("impressionist.strokeOpacity", &cfg->strokeOpacity,
                         0.0f, 1.0f);
}

ImpressionistEffect *GetImpressionistEffect(PostEffect *pe) {
  return (ImpressionistEffect *)pe->effectStates[TRANSFORM_IMPRESSIONIST];
}

void SetupImpressionist(PostEffect *pe) {
  ImpressionistEffectSetup(GetImpressionistEffect(pe),
                           &pe->effects.impressionist);
}

// === UI ===

static void DrawImpressionistParams(EffectConfig *e, const ModSources *ms,
                                    ImU32 glow) {
  (void)glow;
  ImpressionistConfig *imp = &e->impressionist;

  ImGui::SeparatorText("Brush");
  ImGui::SliderInt("Splat Count##impressionist", &imp->splatCount, 4, 16);
  ImGui::SliderFloat("Splat Size Min##impressionist", &imp->splatSizeMin, 0.01f,
                     0.1f, "%.3f");
  ModulatableSlider("Splat Size Max##impressionist", &imp->splatSizeMax,
                    "impressionist.splatSizeMax", "%.3f", ms);
  ModulatableSlider("Stroke Freq##impressionist", &imp->strokeFreq,
                    "impressionist.strokeFreq", "%.0f", ms);
  ModulatableSlider("Stroke Opacity##impressionist", &imp->strokeOpacity,
                    "impressionist.strokeOpacity", "%.2f", ms);
  ImGui::SeparatorText("Edge");
  ModulatableSlider("Edge Strength##impressionist", &imp->edgeStrength,
                    "impressionist.edgeStrength", "%.2f", ms);
  ImGui::SliderFloat("Outline Strength##impressionist", &imp->outlineStrength,
                     0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat("Edge Max Darken##impressionist", &imp->edgeMaxDarken,
                     0.0f, 0.3f, "%.3f");
  ImGui::SeparatorText("Texture");
  ImGui::SliderFloat("Grain Scale##impressionist", &imp->grainScale, 100.0f,
                     800.0f, "%.0f");
  ImGui::SliderFloat("Grain Amount##impressionist", &imp->grainAmount, 0.0f,
                     0.2f, "%.3f");
  ImGui::SeparatorText("Glow");
  ImGui::SliderFloat("Exposure##impressionist", &imp->exposure, 0.5f, 2.0f,
                     "%.2f");
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_IMPRESSIONIST, Impressionist, impressionist,
                "Impressionist", "ART", 4, EFFECT_FLAG_HALF_RES,
                SetupImpressionist, NULL, DrawImpressionistParams)
// clang-format on
