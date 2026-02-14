#include "post_effect.h"
#include "analysis/fft.h"
#include "blend_compositor.h"
#include "config/effect_descriptor.h"
#include "render_utils.h"
#include "rlgl.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/curl_advection.h"
#include "simulation/curl_flow.h"
#include "simulation/cymatics.h"
#include "simulation/particle_life.h"
#include "simulation/physarum.h"
#include <stdlib.h>

static const char *LOG_PREFIX = "POST_EFFECT";

static const int WAVEFORM_TEXTURE_SIZE = 2048;

static void InitFFTTexture(Texture2D *tex) {
  tex->id =
      rlLoadTexture(NULL, FFT_BIN_COUNT, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
  tex->width = FFT_BIN_COUNT;
  tex->height = 1;
  tex->mipmaps = 1;
  tex->format = RL_PIXELFORMAT_UNCOMPRESSED_R32;

  SetTextureFilter(*tex, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(*tex, TEXTURE_WRAP_CLAMP);
}

static void InitWaveformTexture(Texture2D *tex) {
  tex->id = rlLoadTexture(NULL, WAVEFORM_TEXTURE_SIZE, 1,
                          RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
  tex->width = WAVEFORM_TEXTURE_SIZE;
  tex->height = 1;
  tex->mipmaps = 1;
  tex->format = RL_PIXELFORMAT_UNCOMPRESSED_R32;

  SetTextureFilter(*tex, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(*tex, TEXTURE_WRAP_CLAMP);
}

static bool LoadPostEffectShaders(PostEffect *pe) {
  pe->feedbackShader = LoadShader(0, "shaders/feedback.fs");
  pe->blurHShader = LoadShader(0, "shaders/blur_h.fs");
  pe->blurVShader = LoadShader(0, "shaders/blur_v.fs");
  pe->chromaticShader = LoadShader(0, "shaders/chromatic.fs");
  pe->fxaaShader = LoadShader(0, "shaders/fxaa.fs");
  pe->clarityShader = LoadShader(0, "shaders/clarity.fs");
  pe->gammaShader = LoadShader(0, "shaders/gamma.fs");
  pe->shapeTextureShader = LoadShader(0, "shaders/shape_texture.fs");

  return pe->feedbackShader.id != 0 && pe->blurHShader.id != 0 &&
         pe->blurVShader.id != 0 && pe->chromaticShader.id != 0 &&
         pe->fxaaShader.id != 0 && pe->clarityShader.id != 0 &&
         pe->gammaShader.id != 0 && pe->shapeTextureShader.id != 0;
}

// NOLINTNEXTLINE(readability-function-size) - caches all shader uniform
// locations
static void GetShaderUniformLocations(PostEffect *pe) {
  pe->blurHResolutionLoc = GetShaderLocation(pe->blurHShader, "resolution");
  pe->blurVResolutionLoc = GetShaderLocation(pe->blurVShader, "resolution");
  pe->blurHScaleLoc = GetShaderLocation(pe->blurHShader, "blurScale");
  pe->blurVScaleLoc = GetShaderLocation(pe->blurVShader, "blurScale");
  pe->halfLifeLoc = GetShaderLocation(pe->blurVShader, "halfLife");
  pe->deltaTimeLoc = GetShaderLocation(pe->blurVShader, "deltaTime");
  pe->chromaticResolutionLoc =
      GetShaderLocation(pe->chromaticShader, "resolution");
  pe->chromaticOffsetLoc =
      GetShaderLocation(pe->chromaticShader, "chromaticOffset");
  pe->feedbackResolutionLoc =
      GetShaderLocation(pe->feedbackShader, "resolution");
  pe->feedbackDesaturateLoc =
      GetShaderLocation(pe->feedbackShader, "desaturate");
  pe->feedbackZoomBaseLoc = GetShaderLocation(pe->feedbackShader, "zoomBase");
  pe->feedbackZoomRadialLoc =
      GetShaderLocation(pe->feedbackShader, "zoomRadial");
  pe->feedbackRotBaseLoc = GetShaderLocation(pe->feedbackShader, "rotBase");
  pe->feedbackRotRadialLoc = GetShaderLocation(pe->feedbackShader, "rotRadial");
  pe->feedbackDxBaseLoc = GetShaderLocation(pe->feedbackShader, "dxBase");
  pe->feedbackDxRadialLoc = GetShaderLocation(pe->feedbackShader, "dxRadial");
  pe->feedbackDyBaseLoc = GetShaderLocation(pe->feedbackShader, "dyBase");
  pe->feedbackDyRadialLoc = GetShaderLocation(pe->feedbackShader, "dyRadial");
  pe->feedbackFlowStrengthLoc =
      GetShaderLocation(pe->feedbackShader, "feedbackFlowStrength");
  pe->feedbackFlowAngleLoc =
      GetShaderLocation(pe->feedbackShader, "feedbackFlowAngle");
  pe->feedbackFlowScaleLoc =
      GetShaderLocation(pe->feedbackShader, "feedbackFlowScale");
  pe->feedbackFlowThresholdLoc =
      GetShaderLocation(pe->feedbackShader, "feedbackFlowThreshold");
  pe->feedbackCxLoc = GetShaderLocation(pe->feedbackShader, "cx");
  pe->feedbackCyLoc = GetShaderLocation(pe->feedbackShader, "cy");
  pe->feedbackSxLoc = GetShaderLocation(pe->feedbackShader, "sx");
  pe->feedbackSyLoc = GetShaderLocation(pe->feedbackShader, "sy");
  pe->feedbackZoomAngularLoc =
      GetShaderLocation(pe->feedbackShader, "zoomAngular");
  pe->feedbackZoomAngularFreqLoc =
      GetShaderLocation(pe->feedbackShader, "zoomAngularFreq");
  pe->feedbackRotAngularLoc =
      GetShaderLocation(pe->feedbackShader, "rotAngular");
  pe->feedbackRotAngularFreqLoc =
      GetShaderLocation(pe->feedbackShader, "rotAngularFreq");
  pe->feedbackDxAngularLoc = GetShaderLocation(pe->feedbackShader, "dxAngular");
  pe->feedbackDxAngularFreqLoc =
      GetShaderLocation(pe->feedbackShader, "dxAngularFreq");
  pe->feedbackDyAngularLoc = GetShaderLocation(pe->feedbackShader, "dyAngular");
  pe->feedbackDyAngularFreqLoc =
      GetShaderLocation(pe->feedbackShader, "dyAngularFreq");
  pe->feedbackWarpLoc = GetShaderLocation(pe->feedbackShader, "warp");
  pe->feedbackWarpTimeLoc = GetShaderLocation(pe->feedbackShader, "warpTime");
  pe->feedbackWarpScaleInverseLoc =
      GetShaderLocation(pe->feedbackShader, "warpScaleInverse");
  pe->fxaaResolutionLoc = GetShaderLocation(pe->fxaaShader, "resolution");
  pe->clarityResolutionLoc = GetShaderLocation(pe->clarityShader, "resolution");
  pe->clarityAmountLoc = GetShaderLocation(pe->clarityShader, "clarity");
  pe->gammaGammaLoc = GetShaderLocation(pe->gammaShader, "gamma");
  pe->shapeTexZoomLoc = GetShaderLocation(pe->shapeTextureShader, "texZoom");
  pe->shapeTexAngleLoc = GetShaderLocation(pe->shapeTextureShader, "texAngle");
  pe->shapeTexBrightnessLoc =
      GetShaderLocation(pe->shapeTextureShader, "texBrightness");
}

static void SetResolutionUniforms(PostEffect *pe, int width, int height) {
  float resolution[2] = {(float)width, (float)height};
  SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->feedbackShader, pe->feedbackResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->fxaaShader, pe->fxaaResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->clarityShader, pe->clarityResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
}

PostEffect *PostEffectInit(int screenWidth, int screenHeight) {
  PostEffect *pe = (PostEffect *)calloc(1, sizeof(PostEffect));
  if (pe == NULL) {
    return NULL;
  }

  pe->screenWidth = screenWidth;
  pe->screenHeight = screenHeight;
  pe->effects = EffectConfig{};

  if (!LoadPostEffectShaders(pe)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to load shaders");
    free(pe);
    return NULL;
  }

  GetShaderUniformLocations(pe);
  pe->warpTime = 0.0f;

  SetResolutionUniforms(pe, screenWidth, screenHeight);

  RenderUtilsInitTextureHDR(&pe->accumTexture, screenWidth, screenHeight,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->pingPong[0], screenWidth, screenHeight,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->pingPong[1], screenWidth, screenHeight,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->outputTexture, screenWidth, screenHeight,
                            LOG_PREFIX);

  if (pe->accumTexture.id == 0 || pe->pingPong[0].id == 0 ||
      pe->pingPong[1].id == 0 || pe->outputTexture.id == 0) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to create render textures");
    goto cleanup;
  }

  pe->physarum = PhysarumInit(screenWidth, screenHeight, NULL);
  pe->curlFlow = CurlFlowInit(screenWidth, screenHeight, NULL);
  pe->curlAdvection = CurlAdvectionInit(screenWidth, screenHeight, NULL);
  pe->attractorFlow = AttractorFlowInit(screenWidth, screenHeight, NULL);
  pe->particleLife = ParticleLifeInit(screenWidth, screenHeight, NULL);
  pe->boids = BoidsInit(screenWidth, screenHeight, NULL);
  pe->cymatics = CymaticsInit(screenWidth, screenHeight, NULL);
  pe->blendCompositor = BlendCompositorInit();

  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (EFFECT_DESCRIPTORS[i].init != NULL) {
      if (!EFFECT_DESCRIPTORS[i].init(pe, screenWidth, screenHeight)) {
        goto cleanup;
      }
    }
  }

  RenderUtilsInitTextureHDR(&pe->generatorScratch, screenWidth, screenHeight,
                            LOG_PREFIX);

  InitFFTTexture(&pe->fftTexture);
  pe->fftMaxMagnitude = 1.0f;
  TraceLog(LOG_INFO, "POST_EFFECT: FFT texture created (%dx%d)",
           pe->fftTexture.width, pe->fftTexture.height);

  InitWaveformTexture(&pe->waveformTexture);
  TraceLog(LOG_INFO, "POST_EFFECT: Waveform texture created (%dx%d)",
           pe->waveformTexture.width, pe->waveformTexture.height);

  RenderUtilsInitTextureHDR(&pe->halfResA, screenWidth / 2, screenHeight / 2,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->halfResB, screenWidth / 2, screenHeight / 2,
                            LOG_PREFIX);
  TraceLog(LOG_INFO, "POST_EFFECT: Half-res textures allocated (%dx%d)",
           pe->halfResA.texture.width, pe->halfResA.texture.height);

  return pe;

cleanup:
  PostEffectUninit(pe);
  return NULL;
}

void PostEffectRegisterParams(PostEffect *pe) {
  if (pe == NULL) {
    return;
  }

  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (EFFECT_DESCRIPTORS[i].registerParams != NULL) {
      EFFECT_DESCRIPTORS[i].registerParams(&pe->effects);
    }
  }
}

void PostEffectUninit(PostEffect *pe) {
  if (pe == NULL) {
    return;
  }

  PhysarumUninit(pe->physarum);
  CurlFlowUninit(pe->curlFlow);
  CurlAdvectionUninit(pe->curlAdvection);
  AttractorFlowUninit(pe->attractorFlow);
  ParticleLifeUninit(pe->particleLife);
  BoidsUninit(pe->boids);
  CymaticsUninit(pe->cymatics);
  BlendCompositorUninit(pe->blendCompositor);

  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (EFFECT_DESCRIPTORS[i].uninit != NULL) {
      EFFECT_DESCRIPTORS[i].uninit(pe);
    }
  }

  UnloadTexture(pe->fftTexture);
  UnloadTexture(pe->waveformTexture);
  UnloadRenderTexture(pe->accumTexture);
  UnloadRenderTexture(pe->pingPong[0]);
  UnloadRenderTexture(pe->pingPong[1]);
  UnloadRenderTexture(pe->outputTexture);
  UnloadShader(pe->feedbackShader);
  UnloadShader(pe->blurHShader);
  UnloadShader(pe->blurVShader);
  UnloadShader(pe->chromaticShader);
  UnloadShader(pe->fxaaShader);
  UnloadShader(pe->clarityShader);
  UnloadShader(pe->gammaShader);
  UnloadShader(pe->shapeTextureShader);
  UnloadRenderTexture(pe->generatorScratch);
  UnloadRenderTexture(pe->halfResA);
  UnloadRenderTexture(pe->halfResB);
  free(pe);
}

void PostEffectResize(PostEffect *pe, int width, int height) {
  if (pe == NULL || (width == pe->screenWidth && height == pe->screenHeight)) {
    return;
  }

  pe->screenWidth = width;
  pe->screenHeight = height;

  UnloadRenderTexture(pe->accumTexture);
  UnloadRenderTexture(pe->pingPong[0]);
  UnloadRenderTexture(pe->pingPong[1]);
  UnloadRenderTexture(pe->outputTexture);
  RenderUtilsInitTextureHDR(&pe->accumTexture, width, height, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->pingPong[0], width, height, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->pingPong[1], width, height, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->outputTexture, width, height, LOG_PREFIX);

  SetResolutionUniforms(pe, width, height);

  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (EFFECT_DESCRIPTORS[i].resize != NULL) {
      EFFECT_DESCRIPTORS[i].resize(pe, width, height);
    }
  }

  UnloadRenderTexture(pe->halfResA);
  UnloadRenderTexture(pe->halfResB);
  RenderUtilsInitTextureHDR(&pe->halfResA, width / 2, height / 2, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->halfResB, width / 2, height / 2, LOG_PREFIX);

  UnloadRenderTexture(pe->generatorScratch);
  RenderUtilsInitTextureHDR(&pe->generatorScratch, width, height, LOG_PREFIX);

  PhysarumResize(pe->physarum, width, height);
  CurlFlowResize(pe->curlFlow, width, height);
  CurlAdvectionResize(pe->curlAdvection, width, height);
  AttractorFlowResize(pe->attractorFlow, width, height);
  ParticleLifeResize(pe->particleLife, width, height);
  BoidsResize(pe->boids, width, height);
  CymaticsResize(pe->cymatics, width, height);
}

void PostEffectClearFeedback(PostEffect *pe) {
  if (pe == NULL) {
    return;
  }

  // Clear accumulation and ping-pong buffers to black
  BeginTextureMode(pe->accumTexture);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(pe->pingPong[0]);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(pe->pingPong[1]);
  ClearBackground(BLACK);
  EndTextureMode();

  // Clear attractor lines ping-pong trail buffers
  BeginTextureMode(pe->attractorLines.pingPong[0]);
  ClearBackground(BLACK);
  EndTextureMode();
  BeginTextureMode(pe->attractorLines.pingPong[1]);
  ClearBackground(BLACK);
  EndTextureMode();
  pe->attractorLines.readIdx = 0;

  // Reset only enabled simulations to avoid expensive GPU uploads for disabled
  // effects
  if (pe->effects.physarum.enabled) {
    PhysarumReset(pe->physarum);
  }
  if (pe->effects.curlFlow.enabled) {
    CurlFlowReset(pe->curlFlow);
  }
  if (pe->effects.curlAdvection.enabled) {
    CurlAdvectionReset(pe->curlAdvection);
  }
  if (pe->effects.attractorFlow.enabled) {
    AttractorFlowReset(pe->attractorFlow);
  }
  if (pe->particleLife != NULL && pe->effects.particleLife.enabled) {
    ParticleLifeReset(pe->particleLife);
  }
  if (pe->boids != NULL && pe->effects.boids.enabled) {
    BoidsReset(pe->boids);
  }
  if (pe->cymatics != NULL && pe->effects.cymatics.enabled) {
    CymaticsReset(pe->cymatics);
  }

  TraceLog(LOG_INFO, "%s: Cleared feedback buffers and reset simulations",
           LOG_PREFIX);
}

void PostEffectBeginDrawStage(PostEffect *pe) {
  BeginTextureMode(pe->accumTexture);
}

void PostEffectEndDrawStage(void) { EndTextureMode(); }
