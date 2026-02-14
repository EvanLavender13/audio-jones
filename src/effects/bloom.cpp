// Bloom effect module implementation

#include "bloom.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include <stddef.h>

static void InitMips(BloomEffect *e, int width, int height) {
  int w = width / 2;
  int h = height / 2;
  for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
    RenderUtilsInitTextureHDR(&e->mips[i], w, h, "BLOOM");
    w /= 2;
    h /= 2;
    if (w < 1) {
      w = 1;
    }
    if (h < 1) {
      h = 1;
    }
  }
}

static void UnloadMips(BloomEffect *e) {
  for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
    UnloadRenderTexture(e->mips[i]);
  }
}

bool BloomEffectInit(BloomEffect *e, int width, int height) {
  e->prefilterShader = LoadShader(NULL, "shaders/bloom_prefilter.fs");
  if (e->prefilterShader.id == 0) {
    return false;
  }

  e->downsampleShader = LoadShader(NULL, "shaders/bloom_downsample.fs");
  if (e->downsampleShader.id == 0) {
    UnloadShader(e->prefilterShader);
    return false;
  }

  e->upsampleShader = LoadShader(NULL, "shaders/bloom_upsample.fs");
  if (e->upsampleShader.id == 0) {
    UnloadShader(e->prefilterShader);
    UnloadShader(e->downsampleShader);
    return false;
  }

  e->compositeShader = LoadShader(NULL, "shaders/bloom_composite.fs");
  if (e->compositeShader.id == 0) {
    UnloadShader(e->prefilterShader);
    UnloadShader(e->downsampleShader);
    UnloadShader(e->upsampleShader);
    return false;
  }

  // Prefilter shader uniform locations
  e->thresholdLoc = GetShaderLocation(e->prefilterShader, "threshold");
  e->kneeLoc = GetShaderLocation(e->prefilterShader, "knee");

  // Downsample shader uniform locations
  e->downsampleHalfpixelLoc =
      GetShaderLocation(e->downsampleShader, "halfpixel");

  // Upsample shader uniform locations
  e->upsampleHalfpixelLoc = GetShaderLocation(e->upsampleShader, "halfpixel");

  // Composite shader uniform locations
  e->intensityLoc = GetShaderLocation(e->compositeShader, "intensity");
  e->bloomTexLoc = GetShaderLocation(e->compositeShader, "bloomTexture");

  InitMips(e, width, height);

  return true;
}

void BloomEffectSetup(BloomEffect *e, const BloomConfig *cfg) {
  SetShaderValue(e->compositeShader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->compositeShader, e->bloomTexLoc, e->mips[0].texture);
}

void BloomEffectResize(BloomEffect *e, int width, int height) {
  UnloadMips(e);
  InitMips(e, width, height);
}

void BloomEffectUninit(BloomEffect *e) {
  UnloadShader(e->prefilterShader);
  UnloadShader(e->downsampleShader);
  UnloadShader(e->upsampleShader);
  UnloadShader(e->compositeShader);
  UnloadMips(e);
}

BloomConfig BloomConfigDefault(void) { return BloomConfig{}; }

void BloomRegisterParams(BloomConfig *cfg) {
  ModEngineRegisterParam("bloom.threshold", &cfg->threshold, 0.0f, 2.0f);
  ModEngineRegisterParam("bloom.intensity", &cfg->intensity, 0.0f, 2.0f);
}

// Manual registration: custom GetShader (compositeShader) and Resize wrapper
static bool Init_bloom(PostEffect *pe, int w, int h) {
  return BloomEffectInit(&pe->bloom, w, h);
}
static void Uninit_bloom(PostEffect *pe) { BloomEffectUninit(&pe->bloom); }
static void Resize_bloom(PostEffect *pe, int w, int h) {
  BloomEffectResize(&pe->bloom, w, h);
}
static void Register_bloom(EffectConfig *cfg) {
  BloomRegisterParams(&cfg->bloom);
}
static Shader *GetShader_bloom(PostEffect *pe) {
  return &pe->bloom.compositeShader;
}

void SetupBloom(PostEffect *);
// clang-format off
static bool reg_bloom = EffectDescriptorRegister(
    TRANSFORM_BLOOM,
    EffectDescriptor{TRANSFORM_BLOOM, "Bloom", "OPT", 7,
     offsetof(EffectConfig, bloom.enabled),
     (uint8_t)(EFFECT_FLAG_NEEDS_RESIZE),
     Init_bloom, Uninit_bloom, Resize_bloom, Register_bloom,
     GetShader_bloom, SetupBloom});
// clang-format on
