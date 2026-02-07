#include "render_pipeline.h"
#include "analysis/analysis_pipeline.h"
#include "analysis/fft.h"
#include "blend_compositor.h"
#include "drawable.h"
#include "post_effect.h"
#include "raylib.h"
#include "render_utils.h"
#include "shader_setup.h"
#include "shader_setup_generators.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/curl_advection.h"
#include "simulation/curl_flow.h"
#include "simulation/cymatics.h"
#include "simulation/particle_life.h"
#include "simulation/physarum.h"
#include "simulation/trail_map.h"
#include <math.h>
#include <stdbool.h>

static const TransformEffectType HALF_RES_EFFECTS[] = {
    TRANSFORM_IMPRESSIONIST, TRANSFORM_RADIAL_STREAK, TRANSFORM_WATERCOLOR};
static const int HALF_RES_EFFECTS_COUNT =
    sizeof(HALF_RES_EFFECTS) / sizeof(HALF_RES_EFFECTS[0]);

static bool IsHalfResEffect(TransformEffectType type) {
  for (int i = 0; i < HALF_RES_EFFECTS_COUNT; i++) {
    if (HALF_RES_EFFECTS[i] == type) {
      return true;
    }
  }
  return false;
}

static bool IsGeneratorBlendEffect(TransformEffectType type) {
  return type == TRANSFORM_CONSTELLATION_BLEND ||
         type == TRANSFORM_PLASMA_BLEND ||
         type == TRANSFORM_INTERFERENCE_BLEND ||
         type == TRANSFORM_SOLID_COLOR || type == TRANSFORM_SCAN_BARS_BLEND ||
         type == TRANSFORM_PITCH_SPIRAL_BLEND ||
         type == TRANSFORM_MOIRE_GENERATOR_BLEND ||
         type == TRANSFORM_SPECTRAL_ARCS_BLEND ||
         type == TRANSFORM_MUONS_BLEND || type == TRANSFORM_FILAMENTS_BLEND ||
         type == TRANSFORM_SLASHES_BLEND;
}

static void BlitTexture(Texture2D srcTex, RenderTexture2D *dest, int width,
                        int height) {
  BeginTextureMode(*dest);
  ClearBackground(BLACK);
  DrawTextureRec(srcTex, {0, 0, (float)width, (float)-height}, {0, 0}, WHITE);
  EndTextureMode();
}

static void RenderPass(PostEffect *pe, RenderTexture2D *source,
                       RenderTexture2D *dest, Shader shader,
                       RenderPipelineShaderSetupFn setup) {
  BeginTextureMode(*dest);
  if (shader.id != 0) {
    BeginShaderMode(shader);
    if (setup != NULL) {
      setup(pe);
    }
  }
  RenderUtilsDrawFullscreenQuad(source->texture, pe->screenWidth,
                                pe->screenHeight);
  if (shader.id != 0) {
    EndShaderMode();
  }
  EndTextureMode();
}

static void ApplyCurlFlowPass(PostEffect *pe, float deltaTime) {
  if (pe->curlFlow == NULL) {
    return;
  }

  // Always sync config to keep internal agentCount in sync with preset
  CurlFlowApplyConfig(pe->curlFlow, &pe->effects.curlFlow);

  if (pe->effects.curlFlow.enabled) {
    CurlFlowUpdate(pe->curlFlow, deltaTime, pe->accumTexture.texture);
    CurlFlowProcessTrails(pe->curlFlow, deltaTime);
  }

  if (pe->effects.curlFlow.debugOverlay && pe->effects.curlFlow.enabled) {
    BeginTextureMode(pe->accumTexture);
    CurlFlowDrawDebug(pe->curlFlow);
    EndTextureMode();
  }
}

static void ApplyCurlAdvectionPass(PostEffect *pe, float deltaTime) {
  if (pe->curlAdvection == NULL) {
    return;
  }

  CurlAdvectionApplyConfig(pe->curlAdvection, &pe->effects.curlAdvection);

  if (pe->effects.curlAdvection.enabled) {
    CurlAdvectionUpdate(pe->curlAdvection, deltaTime, pe->accumTexture.texture);
    CurlAdvectionProcessTrails(pe->curlAdvection, deltaTime);
  }

  if (pe->effects.curlAdvection.debugOverlay &&
      pe->effects.curlAdvection.enabled) {
    BeginTextureMode(pe->accumTexture);
    CurlAdvectionDrawDebug(pe->curlAdvection);
    EndTextureMode();
  }
}

static void ApplyPhysarumPass(PostEffect *pe, float deltaTime) {
  if (pe->physarum == NULL) {
    return;
  }

  // Always sync config to keep internal agentCount in sync with preset
  PhysarumApplyConfig(pe->physarum, &pe->effects.physarum);

  if (pe->effects.physarum.enabled) {
    PhysarumUpdate(pe->physarum, deltaTime, pe->accumTexture.texture,
                   pe->fftTexture);
    PhysarumProcessTrails(pe->physarum, deltaTime);
  }

  if (pe->effects.physarum.debugOverlay && pe->effects.physarum.enabled) {
    BeginTextureMode(pe->accumTexture);
    PhysarumDrawDebug(pe->physarum);
    EndTextureMode();
  }
}

static void ApplyAttractorFlowPass(PostEffect *pe, float deltaTime) {
  if (pe->attractorFlow == NULL) {
    return;
  }

  // Always sync config to keep internal agentCount in sync with preset
  AttractorFlowApplyConfig(pe->attractorFlow, &pe->effects.attractorFlow);

  if (pe->effects.attractorFlow.enabled) {
    AttractorFlowUpdate(pe->attractorFlow, deltaTime);
    AttractorFlowProcessTrails(pe->attractorFlow, deltaTime);
  }

  if (pe->effects.attractorFlow.debugOverlay &&
      pe->effects.attractorFlow.enabled) {
    BeginTextureMode(pe->accumTexture);
    AttractorFlowDrawDebug(pe->attractorFlow);
    EndTextureMode();
  }
}

static void ApplyParticleLifePass(PostEffect *pe, float deltaTime) {
  if (pe->particleLife == NULL) {
    return;
  }

  ParticleLifeApplyConfig(pe->particleLife, &pe->effects.particleLife);

  if (pe->effects.particleLife.enabled) {
    ParticleLifeUpdate(pe->particleLife, deltaTime);
    ParticleLifeProcessTrails(pe->particleLife, deltaTime);
  }

  if (pe->effects.particleLife.debugOverlay &&
      pe->effects.particleLife.enabled) {
    BeginTextureMode(pe->accumTexture);
    ParticleLifeDrawDebug(pe->particleLife);
    EndTextureMode();
  }
}

static void ApplyBoidsPass(PostEffect *pe, float deltaTime) {
  if (pe->boids == NULL) {
    return;
  }

  // Always sync config to keep internal agentCount in sync with preset
  BoidsApplyConfig(pe->boids, &pe->effects.boids);

  if (pe->effects.boids.enabled) {
    BoidsUpdate(pe->boids, deltaTime, pe->accumTexture.texture, pe->fftTexture);
    BoidsProcessTrails(pe->boids, deltaTime);
  }

  if (pe->effects.boids.debugOverlay && pe->effects.boids.enabled) {
    BeginTextureMode(pe->accumTexture);
    BoidsDrawDebug(pe->boids);
    EndTextureMode();
  }
}

static void ApplyCymaticsPass(PostEffect *pe, float deltaTime,
                              Texture2D waveformTexture, int writeIndex) {
  if (pe->cymatics == NULL) {
    return;
  }

  CymaticsApplyConfig(pe->cymatics, &pe->effects.cymatics);

  if (pe->effects.cymatics.enabled) {
    CymaticsUpdate(pe->cymatics, waveformTexture, writeIndex, deltaTime);
    CymaticsProcessTrails(pe->cymatics, deltaTime);
  }

  if (pe->effects.cymatics.debugOverlay && pe->effects.cymatics.enabled) {
    BeginTextureMode(pe->accumTexture);
    CymaticsDrawDebug(pe->cymatics);
    EndTextureMode();
  }
}

static void UpdateWaveformTexture(PostEffect *pe,
                                  const float *waveformHistory) {
  if (waveformHistory == NULL) {
    return;
  }
  UpdateTexture(pe->waveformTexture, waveformHistory);
}

static void ApplySimulationPasses(PostEffect *pe, float deltaTime,
                                  int waveformWriteIndex) {
  ApplyPhysarumPass(pe, deltaTime);
  ApplyCurlFlowPass(pe, deltaTime);
  ApplyCurlAdvectionPass(pe, deltaTime);
  ApplyAttractorFlowPass(pe, deltaTime);
  ApplyParticleLifePass(pe, deltaTime);
  ApplyBoidsPass(pe, deltaTime);
  ApplyCymaticsPass(pe, deltaTime, pe->waveformTexture, waveformWriteIndex);
}

void RenderPipelineApplyFeedback(PostEffect *pe, float deltaTime,
                                 const float *fftMagnitude) {
  (void)fftMagnitude; // reserved for future spectral-reactive feedback
  pe->warpTime += deltaTime * pe->effects.proceduralWarp.warpSpeed *
                  pe->effects.motionScale;

  pe->currentDeltaTime = deltaTime;
  pe->currentBlurScale = pe->effects.blurScale;

  RenderTexture2D *src = &pe->accumTexture;
  int writeIdx = 0;

  RenderPass(pe, src, &pe->pingPong[writeIdx], pe->feedbackShader,
             SetupFeedback);
  src = &pe->pingPong[writeIdx];
  writeIdx = 1 - writeIdx;

  RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blurHShader, SetupBlurH);
  src = &pe->pingPong[writeIdx];

  RenderPass(pe, src, &pe->accumTexture, pe->blurVShader, SetupBlurV);
}

void RenderPipelineDrawablesFull(PostEffect *pe, DrawableState *state,
                                 Drawable *drawables, int count,
                                 RenderContext *renderCtx) {
  PostEffectBeginDrawStage(pe);
  const uint64_t tick = DrawableGetTick(state);
  DrawableRenderFull(state, renderCtx, drawables, count, tick);
  PostEffectEndDrawStage();
}

void RenderPipelineExecute(PostEffect *pe, DrawableState *state,
                           Drawable *drawables, int count,
                           RenderContext *renderCtx, float deltaTime,
                           const float *fftMagnitude,
                           const float *waveformHistory, int waveformWriteIndex,
                           Profiler *profiler) {
  ProfilerFrameBegin(profiler);

  // Upload waveform texture before simulations consume it
  UpdateWaveformTexture(pe, waveformHistory);

  // 1. Run GPU simulations (physarum, curl flow, attractor, boids, cymatics)
  ProfilerBeginZone(profiler, ZONE_SIMULATION);
  ApplySimulationPasses(pe, deltaTime, waveformWriteIndex);
  ProfilerEndZone(profiler, ZONE_SIMULATION);

  // 2. Apply feedback effects (warp, blur, decay)
  ProfilerBeginZone(profiler, ZONE_FEEDBACK);
  RenderPipelineApplyFeedback(pe, deltaTime, fftMagnitude);
  ProfilerEndZone(profiler, ZONE_FEEDBACK);

  // 2.5. Copy feedback result for textured shape sampling.
  // Shapes sample from outputTexture (via renderCtx). By updating it here
  // (after feedback, before drawables), shapes sample the feedback-processed
  // content rather than post-transform content from the previous frame.
  // This preserves the feedback loop: shapes draw their sampled content,
  // waveforms draw on top, and both contribute to the next frame's feedback.
  BlitTexture(pe->accumTexture.texture, &pe->outputTexture, pe->screenWidth,
              pe->screenHeight);

  // 3. Draw all drawables at configured opacity
  ProfilerBeginZone(profiler, ZONE_DRAWABLES);
  RenderPipelineDrawablesFull(pe, state, drawables, count, renderCtx);
  ProfilerEndZone(profiler, ZONE_DRAWABLES);

  // 4. Output chain
  ProfilerBeginZone(profiler, ZONE_OUTPUT);
  BeginDrawing();
  ClearBackground(BLACK);
  RenderPipelineApplyOutput(pe, DrawableGetTick(state), deltaTime);
  ProfilerEndZone(profiler, ZONE_OUTPUT);

  ProfilerFrameEnd(profiler);
}

void RenderPipelineApplyOutput(PostEffect *pe, uint64_t globalTick,
                               float deltaTime) {
  (void)deltaTime; // reserved for time-based output effects
  // Update trail boost active states
  pe->physarumBoostActive =
      (pe->physarum != NULL && pe->effects.physarum.enabled &&
       pe->effects.physarum.boostIntensity > 0.0f);
  pe->curlFlowBoostActive =
      (pe->curlFlow != NULL && pe->effects.curlFlow.enabled &&
       pe->effects.curlFlow.boostIntensity > 0.0f);
  pe->curlAdvectionBoostActive =
      (pe->curlAdvection != NULL && pe->effects.curlAdvection.enabled &&
       pe->effects.curlAdvection.boostIntensity > 0.0f);
  pe->attractorFlowBoostActive =
      (pe->attractorFlow != NULL && pe->effects.attractorFlow.enabled &&
       pe->effects.attractorFlow.boostIntensity > 0.0f);
  pe->particleLifeBoostActive =
      (pe->particleLife != NULL && pe->effects.particleLife.enabled &&
       pe->effects.particleLife.boostIntensity > 0.0f);
  pe->boidsBoostActive = (pe->boids != NULL && pe->effects.boids.enabled &&
                          pe->effects.boids.boostIntensity > 0.0f);
  pe->cymaticsBoostActive =
      (pe->cymatics != NULL && pe->effects.cymatics.enabled &&
       pe->effects.cymatics.boostIntensity > 0.0f);

  // Generator blend active flags — delegates to IsTransformEnabled to avoid
  // duplicating the enabled && blendIntensity > 0 check
  pe->constellationBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_CONSTELLATION_BLEND);
  pe->plasmaBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_PLASMA_BLEND);
  pe->interferenceBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_INTERFERENCE_BLEND);
  pe->solidColorBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_SOLID_COLOR);
  pe->scanBarsBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_SCAN_BARS_BLEND);
  pe->pitchSpiralBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_PITCH_SPIRAL_BLEND);
  pe->moireGeneratorBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_MOIRE_GENERATOR_BLEND);
  pe->spectralArcsBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_SPECTRAL_ARCS_BLEND);
  pe->muonsBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_MUONS_BLEND);
  pe->filamentsBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_FILAMENTS_BLEND);
  pe->slashesBlendActive =
      IsTransformEnabled(&pe->effects, TRANSFORM_SLASHES_BLEND);

  // Compute Lissajous animation time
  const float t = (float)globalTick * 0.016f;
  pe->transformTime = t;

  RenderTexture2D *src = &pe->accumTexture;
  int writeIdx = 0;

  // Chromatic aberration before transforms: the radial "bump" gets warped with
  // content
  RenderPass(pe, src, &pe->pingPong[writeIdx], pe->chromaticShader,
             SetupChromatic);
  src = &pe->pingPong[writeIdx];
  writeIdx = 1 - writeIdx;

  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    const TransformEffectType effectType = pe->effects.transformOrder[i];
    const TransformEffectEntry entry = GetTransformEffect(pe, effectType);
    if (entry.enabled != NULL && *entry.enabled) {
      if (IsHalfResEffect(effectType)) {
        ApplyHalfResEffect(pe, src, &writeIdx, *entry.shader, entry.setup);
      } else if (effectType == TRANSFORM_BLOOM) {
        ApplyBloomPasses(pe, src, &writeIdx);
        RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader,
                   entry.setup);
      } else if (effectType == TRANSFORM_ANAMORPHIC_STREAK) {
        ApplyAnamorphicStreakPasses(pe, src);
        RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader,
                   entry.setup);
      } else if (effectType == TRANSFORM_OIL_PAINT) {
        ApplyHalfResOilPaint(pe, src, &writeIdx);
      } else if (IsGeneratorBlendEffect(effectType)) {
        GeneratorPassInfo gen = GetGeneratorScratchPass(pe, effectType);
        RenderPass(pe, src, &pe->generatorScratch, gen.shader, gen.setup);
        RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader,
                   entry.setup);
      } else {
        RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader,
                   entry.setup);
      }
      src = &pe->pingPong[writeIdx];
      writeIdx = 1 - writeIdx;
    }
  }

  if (pe->effects.clarity > 0.0f) {
    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->clarityShader,
               SetupClarity);
    src = &pe->pingPong[writeIdx];
    writeIdx = 1 - writeIdx;
  }

  RenderPass(pe, src, &pe->pingPong[writeIdx], pe->fxaaShader, NULL);
  src = &pe->pingPong[writeIdx];
  writeIdx = 1 - writeIdx;

  RenderPass(pe, src, &pe->pingPong[writeIdx], pe->gammaShader, SetupGamma);
  RenderUtilsDrawFullscreenQuad(pe->pingPong[writeIdx].texture, pe->screenWidth,
                                pe->screenHeight);
}
