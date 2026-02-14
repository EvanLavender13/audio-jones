#include "texture_warp.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

void TextureWarpEffectSetup(TextureWarpEffect *e, const TextureWarpConfig *cfg,
                            float deltaTime) {
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

void TextureWarpEffectUninit(TextureWarpEffect *e) { UnloadShader(e->shader); }

TextureWarpConfig TextureWarpConfigDefault(void) { return TextureWarpConfig{}; }

void TextureWarpRegisterParams(TextureWarpConfig *cfg) {
  ModEngineRegisterParam("textureWarp.strength", &cfg->strength, 0.0f, 0.3f);
  ModEngineRegisterParam("textureWarp.ridgeAngle", &cfg->ridgeAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("textureWarp.anisotropy", &cfg->anisotropy, 0.0f,
                         1.0f);
  ModEngineRegisterParam("textureWarp.noiseAmount", &cfg->noiseAmount, 0.0f,
                         1.0f);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_TEXTURE_WARP, TextureWarp, textureWarp,
                "Texture Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupTextureWarp, NULL)
// clang-format on
