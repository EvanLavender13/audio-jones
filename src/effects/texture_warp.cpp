#include "texture_warp.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool TextureWarpEffectInit(TextureWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/texture_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->channelModeLoc = GetShaderLocation(e->shader, "channelMode");
  e->ridgeAngleLoc = GetShaderLocation(e->shader, "ridgeAngle");
  e->anisotropyLoc = GetShaderLocation(e->shader, "anisotropy");
  e->noiseAmountLoc = GetShaderLocation(e->shader, "noiseAmount");
  e->noiseScaleLoc = GetShaderLocation(e->shader, "noiseScale");

  return true;
}

void TextureWarpEffectSetup(const TextureWarpEffect *e,
                            const TextureWarpConfig *cfg, float deltaTime) {
  (void)deltaTime; // No time accumulation for this effect

  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);

  int channelMode = (int)cfg->channelMode;
  SetShaderValue(e->shader, e->channelModeLoc, &channelMode,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->ridgeAngleLoc, &cfg->ridgeAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->anisotropyLoc, &cfg->anisotropy,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseAmountLoc, &cfg->noiseAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseScaleLoc, &cfg->noiseScale,
                 SHADER_UNIFORM_FLOAT);
}

void TextureWarpEffectUninit(const TextureWarpEffect *e) {
  UnloadShader(e->shader);
}

void TextureWarpRegisterParams(TextureWarpConfig *cfg) {
  ModEngineRegisterParam("textureWarp.strength", &cfg->strength, 0.0f, 0.3f);
  ModEngineRegisterParam("textureWarp.ridgeAngle", &cfg->ridgeAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("textureWarp.anisotropy", &cfg->anisotropy, 0.0f,
                         1.0f);
  ModEngineRegisterParam("textureWarp.noiseAmount", &cfg->noiseAmount, 0.0f,
                         1.0f);
}

// === UI ===

static void DrawTextureWarpParams(EffectConfig *e, const ModSources *ms,
                                  ImU32 glow) {
  const char *channelModeNames[] = {
      "RG", "RB", "GB", "Luminance", "LuminanceSplit", "Chrominance", "Polar"};
  int channelMode = (int)e->textureWarp.channelMode;
  if (ImGui::Combo("Channel Mode##texwarp", &channelMode, channelModeNames,
                   7)) {
    e->textureWarp.channelMode = (TextureWarpChannelMode)channelMode;
  }
  ModulatableSlider("Strength##texwarp", &e->textureWarp.strength,
                    "textureWarp.strength", "%.3f", ms);
  ImGui::SliderInt("Iterations##texwarp", &e->textureWarp.iterations, 1, 8);

  if (TreeNodeAccented("Directional##texwarp", glow)) {
    ModulatableSliderAngleDeg("Ridge Angle##texwarp",
                              &e->textureWarp.ridgeAngle,
                              "textureWarp.ridgeAngle", ms);
    ModulatableSlider("Anisotropy##texwarp", &e->textureWarp.anisotropy,
                      "textureWarp.anisotropy", "%.2f", ms);
    TreeNodeAccentedPop();
  }

  if (TreeNodeAccented("Noise##texwarp", glow)) {
    ModulatableSlider("Noise Amount##texwarp", &e->textureWarp.noiseAmount,
                      "textureWarp.noiseAmount", "%.2f", ms);
    ImGui::SliderFloat("Noise Scale##texwarp", &e->textureWarp.noiseScale, 1.0f,
                       20.0f, "%.1f");
    TreeNodeAccentedPop();
  }
}

void SetupTextureWarp(PostEffect *pe) {
  TextureWarpEffectSetup(&pe->textureWarp, &pe->effects.textureWarp,
                         pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_TEXTURE_WARP, TextureWarp, textureWarp,
                "Texture Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupTextureWarp, NULL, DrawTextureWarpParams)
// clang-format on
