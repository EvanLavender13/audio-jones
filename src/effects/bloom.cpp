// Bloom effect module implementation

#include "bloom.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include "ui/modulatable_slider.h"
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

static void UnloadMips(const BloomEffect *e) {
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

void BloomEffectSetup(const BloomEffect *e, const BloomConfig *cfg) {
  SetShaderValue(e->compositeShader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->compositeShader, e->bloomTexLoc, e->mips[0].texture);
}

void BloomEffectResize(BloomEffect *e, int width, int height) {
  UnloadMips(e);
  InitMips(e, width, height);
}

void BloomEffectUninit(const BloomEffect *e) {
  UnloadShader(e->prefilterShader);
  UnloadShader(e->downsampleShader);
  UnloadShader(e->upsampleShader);
  UnloadShader(e->compositeShader);
  UnloadMips(e);
}

static void BloomRenderPass(const RenderTexture2D *source,
                            const RenderTexture2D *dest, Shader shader) {
  BeginTextureMode(*dest);
  BeginShaderMode(shader);
  DrawTexturePro(
      source->texture,
      {0, 0, (float)source->texture.width, (float)-source->texture.height},
      {0, 0, (float)dest->texture.width, (float)dest->texture.height}, {0, 0},
      0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();
}

void ApplyBloomPasses(PostEffect *pe, const RenderTexture2D *source,
                      int * /* writeIdx */) {
  const BloomConfig *b = &pe->effects.bloom;
  int iterations = b->iterations;
  if (iterations < 1) {
    iterations = 1;
  }
  if (iterations > BLOOM_MIP_COUNT) {
    iterations = BLOOM_MIP_COUNT;
  }

  // Prefilter: extract bright pixels from source to mip[0]
  SetShaderValue(pe->bloom.prefilterShader, pe->bloom.thresholdLoc,
                 &b->threshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->bloom.prefilterShader, pe->bloom.kneeLoc, &b->knee,
                 SHADER_UNIFORM_FLOAT);
  BloomRenderPass(source, &pe->bloom.mips[0], pe->bloom.prefilterShader);

  // Downsample: mip[0] -> mip[1] -> ... -> mip[iterations-1]
  for (int i = 1; i < iterations; i++) {
    const float halfpixel[2] = {
        0.5f / (float)pe->bloom.mips[i - 1].texture.width,
        0.5f / (float)pe->bloom.mips[i - 1].texture.height};
    SetShaderValue(pe->bloom.downsampleShader, pe->bloom.downsampleHalfpixelLoc,
                   halfpixel, SHADER_UNIFORM_VEC2);
    BloomRenderPass(&pe->bloom.mips[i - 1], &pe->bloom.mips[i],
                    pe->bloom.downsampleShader);
  }

  // Upsample: mip[iterations-1] -> ... -> mip[0] (additive blend at each level)
  for (int i = iterations - 1; i > 0; i--) {
    const float halfpixel[2] = {0.5f / (float)pe->bloom.mips[i].texture.width,
                                0.5f / (float)pe->bloom.mips[i].texture.height};
    SetShaderValue(pe->bloom.upsampleShader, pe->bloom.upsampleHalfpixelLoc,
                   halfpixel, SHADER_UNIFORM_VEC2);

    // Upsample mip[i] and add to mip[i-1]
    BeginTextureMode(pe->bloom.mips[i - 1]);
    BeginBlendMode(BLEND_ADDITIVE);
    BeginShaderMode(pe->bloom.upsampleShader);
    DrawTexturePro(pe->bloom.mips[i].texture,
                   {0, 0, (float)pe->bloom.mips[i].texture.width,
                    (float)-pe->bloom.mips[i].texture.height},
                   {0, 0, (float)pe->bloom.mips[i - 1].texture.width,
                    (float)pe->bloom.mips[i - 1].texture.height},
                   {0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndBlendMode();
    EndTextureMode();
  }

  // Final composite uses SetupBloom to bind uniforms, called by render_pipeline
}

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

void SetupBloom(PostEffect *pe) {
  BloomEffectSetup(&pe->bloom, &pe->effects.bloom);
}

// === UI ===

static void DrawBloomParams(EffectConfig *e, const ModSources *ms, ImU32 glow) {
  (void)glow;
  BloomConfig *b = &e->bloom;

  ModulatableSlider("Threshold##bloom", &b->threshold, "bloom.threshold",
                    "%.2f", ms);
  ImGui::SliderFloat("Knee##bloom", &b->knee, 0.0f, 1.0f, "%.2f");
  ModulatableSlider("Intensity##bloom", &b->intensity, "bloom.intensity",
                    "%.2f", ms);
  ImGui::SliderInt("Iterations##bloom", &b->iterations, 3, 5);
}

// clang-format off
static bool reg_bloom = EffectDescriptorRegister(
    TRANSFORM_BLOOM,
    EffectDescriptor{TRANSFORM_BLOOM, "Bloom", "OPT", 7,
     offsetof(EffectConfig, bloom.enabled), "bloom.",
     (uint8_t)(EFFECT_FLAG_NEEDS_RESIZE),
     Init_bloom, Uninit_bloom, Resize_bloom, Register_bloom,
     GetShader_bloom, SetupBloom,
     nullptr, nullptr, nullptr,
     DrawBloomParams, nullptr});
// clang-format on
