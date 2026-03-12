// Flip Book effect module implementation
// Holds frames at a configurable FPS with per-frame hash-based UV jitter

#include "flip_book.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

static void CacheLocations(FlipBookEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->jitterSeedLoc = GetShaderLocation(e->shader, "jitterSeed");
  e->jitterAmountLoc = GetShaderLocation(e->shader, "jitterAmount");
}

bool FlipBookEffectInit(FlipBookEffect *e, int width, int height) {
  e->shader = LoadShader(NULL, "shaders/flip_book.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);
  RenderUtilsInitTextureHDR(&e->heldFrame, width, height, "FLIP_BOOK");
  e->frameTimer = 0.0f;
  e->frameIndex = 0;
  e->lastRenderedIndex = -1;

  return true;
}

void FlipBookEffectSetup(FlipBookEffect *e, const FlipBookConfig *cfg,
                         float deltaTime) {
  e->frameTimer += deltaTime;
  float interval = 1.0f / cfg->fps;
  if (e->frameTimer >= interval) {
    e->frameTimer = 0.0f;
    e->frameIndex++;
  }

  float seed = (float)e->frameIndex;
  SetShaderValue(e->shader, e->jitterSeedLoc, &seed, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->jitterAmountLoc, &cfg->jitter,
                 SHADER_UNIFORM_FLOAT);
}

void FlipBookEffectRender(FlipBookEffect *e, PostEffect *pe) {
  // Capture current scene into held frame when frameIndex advances
  if (e->frameIndex != e->lastRenderedIndex) {
    BeginTextureMode(e->heldFrame);
    RenderUtilsDrawFullscreenQuad(pe->currentSceneTexture, pe->screenWidth,
                                  pe->screenHeight);
    EndTextureMode();
    e->lastRenderedIndex = e->frameIndex;
  }

  // Render held frame through jitter shader to pipeline output
  float resolution[2] = {(float)pe->screenWidth, (float)pe->screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  BeginTextureMode(*pe->currentRenderDest);
  BeginShaderMode(e->shader);
  RenderUtilsDrawFullscreenQuad(e->heldFrame.texture, pe->screenWidth,
                                pe->screenHeight);
  EndShaderMode();
  EndTextureMode();
}

void FlipBookEffectResize(FlipBookEffect *e, int width, int height) {
  UnloadRenderTexture(e->heldFrame);
  RenderUtilsInitTextureHDR(&e->heldFrame, width, height, "FLIP_BOOK");
  e->lastRenderedIndex = -1;
}

void FlipBookEffectUninit(FlipBookEffect *e) {
  UnloadShader(e->shader);
  UnloadRenderTexture(e->heldFrame);
}

FlipBookConfig FlipBookConfigDefault(void) { return FlipBookConfig{}; }

void FlipBookRegisterParams(FlipBookConfig *cfg) {
  ModEngineRegisterParam("flipBook.fps", &cfg->fps, 2.0f, 60.0f);
  ModEngineRegisterParam("flipBook.jitter", &cfg->jitter, 0.0f, 8.0f);
}

// === UI ===

static void DrawFlipBookParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  ModulatableSlider("FPS##flipBook", &e->flipBook.fps, "flipBook.fps", "%.1f",
                    ms);
  ModulatableSlider("Jitter##flipBook", &e->flipBook.jitter, "flipBook.jitter",
                    "%.1f", ms);
}

// Bridge functions for EffectDescriptor dispatch
void SetupFlipBook(PostEffect *pe) {
  FlipBookEffectSetup(&pe->flipBook, &pe->effects.flipBook,
                      pe->currentDeltaTime);
}

void RenderFlipBook(PostEffect *pe) { FlipBookEffectRender(&pe->flipBook, pe); }

// Manual registration — RET badge, section 6, needs resize
static bool Init_flipBook(PostEffect *pe, int w, int h) {
  return FlipBookEffectInit(&pe->flipBook, w, h);
}

static void Uninit_flipBook(PostEffect *pe) {
  FlipBookEffectUninit(&pe->flipBook);
}

static void Resize_flipBook(PostEffect *pe, int w, int h) {
  FlipBookEffectResize(&pe->flipBook, w, h);
}

static void Register_flipBook(EffectConfig *cfg) {
  FlipBookRegisterParams(&cfg->flipBook);
}

static Shader *GetShader_flipBook(PostEffect *pe) {
  return &pe->flipBook.shader;
}

// clang-format off
static bool reg_flipBook = EffectDescriptorRegister(
    TRANSFORM_FLIP_BOOK,
    EffectDescriptor{TRANSFORM_FLIP_BOOK,
        "Flip Book", "RET", 6,
        offsetof(EffectConfig, flipBook.enabled), "flipBook.",
        EFFECT_FLAG_NEEDS_RESIZE,
        Init_flipBook, Uninit_flipBook,
        Resize_flipBook, Register_flipBook,
        GetShader_flipBook, nullptr,
        nullptr, SetupFlipBook,
        RenderFlipBook,
        DrawFlipBookParams, nullptr});
// clang-format on
