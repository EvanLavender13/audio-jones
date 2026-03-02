// Oil paint effect module implementation

#include "oil_paint.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/noise_texture.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool OilPaintEffectInit(OilPaintEffect *e, int width, int height) {
  e->strokeShader = LoadShader(NULL, "shaders/oil_paint_stroke.fs");
  if (e->strokeShader.id == 0) {
    return false;
  }

  e->compositeShader = LoadShader(NULL, "shaders/oil_paint.fs");
  if (e->compositeShader.id == 0) {
    UnloadShader(e->strokeShader);
    return false;
  }

  // Stroke shader uniform locations
  e->strokeResolutionLoc = GetShaderLocation(e->strokeShader, "resolution");
  e->brushSizeLoc = GetShaderLocation(e->strokeShader, "brushSize");
  e->strokeBendLoc = GetShaderLocation(e->strokeShader, "strokeBend");
  e->brushDetailLoc = GetShaderLocation(e->strokeShader, "brushDetail");
  e->srcContrastLoc = GetShaderLocation(e->strokeShader, "srcContrast");
  e->srcBrightLoc = GetShaderLocation(e->strokeShader, "srcBright");
  e->layersLoc = GetShaderLocation(e->strokeShader, "layers");
  e->noiseTexLoc = GetShaderLocation(e->strokeShader, "texture1");

  // Composite shader uniform locations
  e->compositeResolutionLoc =
      GetShaderLocation(e->compositeShader, "resolution");
  e->specularLoc = GetShaderLocation(e->compositeShader, "specular");

  RenderUtilsInitTextureHDR(&e->intermediate, width, height, "OIL_PAINT");

  return true;
}

void OilPaintEffectSetup(OilPaintEffect *e, const OilPaintConfig *cfg,
                         float deltaTime) {
  (void)deltaTime;

  // Stroke shader uniforms
  SetShaderValue(e->strokeShader, e->brushSizeLoc, &cfg->brushSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->strokeShader, e->strokeBendLoc, &cfg->strokeBend,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->strokeShader, e->brushDetailLoc, &cfg->brushDetail,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->strokeShader, e->srcContrastLoc, &cfg->srcContrast,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->strokeShader, e->srcBrightLoc, &cfg->srcBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->strokeShader, e->layersLoc, &cfg->layers,
                 SHADER_UNIFORM_INT);
  SetShaderValueTexture(e->strokeShader, e->noiseTexLoc, NoiseTextureGet());

  // Composite shader uniforms
  SetShaderValue(e->compositeShader, e->specularLoc, &cfg->specular,
                 SHADER_UNIFORM_FLOAT);
}

void OilPaintEffectUninit(OilPaintEffect *e) {
  UnloadShader(e->strokeShader);
  UnloadShader(e->compositeShader);
  UnloadRenderTexture(e->intermediate);
}

void OilPaintEffectResize(OilPaintEffect *e, int width, int height) {
  UnloadRenderTexture(e->intermediate);
  RenderUtilsInitTextureHDR(&e->intermediate, width, height, "OIL_PAINT");
}

void ApplyHalfResOilPaint(PostEffect *pe, RenderTexture2D *source,
                          const int *writeIdx) {
  const int halfW = pe->screenWidth / 2;
  const int halfH = pe->screenHeight / 2;
  const Rectangle srcRect = {0, 0, (float)source->texture.width,
                             (float)-source->texture.height};
  const Rectangle halfRect = {0, 0, (float)halfW, (float)halfH};
  const Rectangle fullRect = {0, 0, (float)pe->screenWidth,
                              (float)pe->screenHeight};
  float halfRes[2] = {(float)halfW, (float)halfH};
  float fullRes[2] = {(float)pe->screenWidth, (float)pe->screenHeight};

  BeginTextureMode(pe->halfResA);
  DrawTexturePro(source->texture, srcRect, halfRect, {0, 0}, 0.0f, WHITE);
  EndTextureMode();

  SetShaderValue(pe->oilPaint.strokeShader, pe->oilPaint.strokeResolutionLoc,
                 halfRes, SHADER_UNIFORM_VEC2);

  BeginTextureMode(pe->halfResB);
  BeginShaderMode(pe->oilPaint.strokeShader);
  DrawTexturePro(pe->halfResA.texture, {0, 0, (float)halfW, (float)-halfH},
                 halfRect, {0, 0}, 0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();

  SetShaderValue(pe->oilPaint.compositeShader,
                 pe->oilPaint.compositeResolutionLoc, halfRes,
                 SHADER_UNIFORM_VEC2);

  BeginTextureMode(pe->halfResA);
  BeginShaderMode(pe->oilPaint.compositeShader);
  DrawTexturePro(pe->halfResB.texture, {0, 0, (float)halfW, (float)-halfH},
                 halfRect, {0, 0}, 0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();

  // Subsequent effects may share these shaders
  SetShaderValue(pe->oilPaint.strokeShader, pe->oilPaint.strokeResolutionLoc,
                 fullRes, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->oilPaint.compositeShader,
                 pe->oilPaint.compositeResolutionLoc, fullRes,
                 SHADER_UNIFORM_VEC2);

  BeginTextureMode(pe->pingPong[*writeIdx]);
  DrawTexturePro(pe->halfResA.texture, {0, 0, (float)halfW, (float)-halfH},
                 fullRect, {0, 0}, 0.0f, WHITE);
  EndTextureMode();
}

OilPaintConfig OilPaintConfigDefault(void) { return OilPaintConfig{}; }

void OilPaintRegisterParams(OilPaintConfig *cfg) {
  ModEngineRegisterParam("oilPaint.brushSize", &cfg->brushSize, 0.5f, 3.0f);
  ModEngineRegisterParam("oilPaint.strokeBend", &cfg->strokeBend, -2.0f, 2.0f);
  ModEngineRegisterParam("oilPaint.brushDetail", &cfg->brushDetail, 0.01f,
                         0.5f);
  ModEngineRegisterParam("oilPaint.srcContrast", &cfg->srcContrast, 0.5f, 3.0f);
  ModEngineRegisterParam("oilPaint.srcBright", &cfg->srcBright, 0.5f, 1.5f);
  ModEngineRegisterParam("oilPaint.specular", &cfg->specular, 0.0f, 1.0f);
}

// Manual registration: oil paint has a composite shader (not .shader)
static bool Init_oilPaint(PostEffect *pe, int w, int h) {
  return OilPaintEffectInit(&pe->oilPaint, w, h);
}
static void Uninit_oilPaint(PostEffect *pe) {
  OilPaintEffectUninit(&pe->oilPaint);
}
static void Resize_oilPaint(PostEffect *pe, int w, int h) {
  OilPaintEffectResize(&pe->oilPaint, w, h);
}
static void Register_oilPaint(EffectConfig *cfg) {
  OilPaintRegisterParams(&cfg->oilPaint);
}
static Shader *GetShader_oilPaint(PostEffect *pe) {
  return &pe->oilPaint.compositeShader;
}
void SetupOilPaint(PostEffect *pe) {
  OilPaintEffectSetup(&pe->oilPaint, &pe->effects.oilPaint,
                      pe->currentDeltaTime);
}

// === UI ===

static void DrawOilPaintParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  OilPaintConfig *op = &e->oilPaint;
  ModulatableSlider("Brush Size##oilpaint", &op->brushSize,
                    "oilPaint.brushSize", "%.2f", ms);
  ModulatableSlider("Stroke Bend##oilpaint", &op->strokeBend,
                    "oilPaint.strokeBend", "%.2f", ms);
  ModulatableSlider("Brush Detail##oilpaint", &op->brushDetail,
                    "oilPaint.brushDetail", "%.2f", ms);
  ModulatableSlider("Contrast##oilpaint", &op->srcContrast,
                    "oilPaint.srcContrast", "%.2f", ms);
  ModulatableSlider("Brightness##oilpaint", &op->srcBright,
                    "oilPaint.srcBright", "%.2f", ms);
  ModulatableSlider("Specular##oilpaint", &op->specular, "oilPaint.specular",
                    "%.2f", ms);
  ImGui::SliderInt("Layers##oilpaint", &op->layers, 3, 11);
}

// clang-format off
static bool reg_oilPaint = EffectDescriptorRegister(
    TRANSFORM_OIL_PAINT,
    EffectDescriptor{TRANSFORM_OIL_PAINT, "Oil Paint", "ART", 4,
     offsetof(EffectConfig, oilPaint.enabled), EFFECT_FLAG_NEEDS_RESIZE,
     Init_oilPaint, Uninit_oilPaint, Resize_oilPaint, Register_oilPaint,
     GetShader_oilPaint, SetupOilPaint, nullptr, nullptr, nullptr,
     DrawOilPaintParams, nullptr});
// clang-format on
