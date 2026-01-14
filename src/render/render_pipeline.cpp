#include "render_pipeline.h"
#include "shader_setup.h"
#include "blend_compositor.h"
#include "post_effect.h"
#include "drawable.h"
#include "simulation/physarum.h"
#include "simulation/trail_map.h"
#include "simulation/curl_flow.h"
#include "simulation/curl_advection.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/cymatics.h"
#include "render_utils.h"
#include "analysis/fft.h"
#include "analysis/analysis_pipeline.h"
#include "raylib.h"
#include <math.h>
#include <stdbool.h>

static void BlitTexture(Texture2D srcTex, RenderTexture2D* dest, int width, int height)
{
    BeginTextureMode(*dest);
    ClearBackground(BLACK);
    DrawTextureRec(srcTex,
                   { 0, 0, (float)width, (float)-height },
                   { 0, 0 }, WHITE);
    EndTextureMode();
}

static void RenderPass(PostEffect* pe,
                       RenderTexture2D* source,
                       RenderTexture2D* dest,
                       Shader shader,
                       RenderPipelineShaderSetupFn setup)
{
    BeginTextureMode(*dest);
    if (shader.id != 0) {
        BeginShaderMode(shader);
        if (setup != NULL) {
            setup(pe);
        }
    }
    RenderUtilsDrawFullscreenQuad(source->texture, pe->screenWidth, pe->screenHeight);
    if (shader.id != 0) {
        EndShaderMode();
    }
    EndTextureMode();
}

static void ApplyCurlFlowPass(PostEffect* pe, float deltaTime)
{
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

static void ApplyCurlAdvectionPass(PostEffect* pe, float deltaTime)
{
    if (pe->curlAdvection == NULL) {
        return;
    }

    CurlAdvectionApplyConfig(pe->curlAdvection, &pe->effects.curlAdvection);

    if (pe->effects.curlAdvection.enabled) {
        CurlAdvectionUpdate(pe->curlAdvection, deltaTime);
        CurlAdvectionProcessTrails(pe->curlAdvection, deltaTime);
    }

    if (pe->effects.curlAdvection.debugOverlay && pe->effects.curlAdvection.enabled) {
        BeginTextureMode(pe->accumTexture);
        CurlAdvectionDrawDebug(pe->curlAdvection);
        EndTextureMode();
    }
}

static void ApplyPhysarumPass(PostEffect* pe, float deltaTime)
{
    if (pe->physarum == NULL) {
        return;
    }

    // Always sync config to keep internal agentCount in sync with preset
    PhysarumApplyConfig(pe->physarum, &pe->effects.physarum);

    if (pe->effects.physarum.enabled) {
        PhysarumUpdate(pe->physarum, deltaTime, pe->accumTexture.texture, pe->fftTexture);
        PhysarumProcessTrails(pe->physarum, deltaTime);
    }

    if (pe->effects.physarum.debugOverlay && pe->effects.physarum.enabled) {
        BeginTextureMode(pe->accumTexture);
        PhysarumDrawDebug(pe->physarum);
        EndTextureMode();
    }
}

static void ApplyAttractorFlowPass(PostEffect* pe, float deltaTime)
{
    if (pe->attractorFlow == NULL) {
        return;
    }

    // Always sync config to keep internal agentCount in sync with preset
    AttractorFlowApplyConfig(pe->attractorFlow, &pe->effects.attractorFlow);

    if (pe->effects.attractorFlow.enabled) {
        AttractorFlowUpdate(pe->attractorFlow, deltaTime);
        AttractorFlowProcessTrails(pe->attractorFlow, deltaTime);
    }

    if (pe->effects.attractorFlow.debugOverlay && pe->effects.attractorFlow.enabled) {
        BeginTextureMode(pe->accumTexture);
        AttractorFlowDrawDebug(pe->attractorFlow);
        EndTextureMode();
    }
}

static void ApplyBoidsPass(PostEffect* pe, float deltaTime)
{
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

static void ApplyCymaticsPass(PostEffect* pe, float deltaTime, Texture2D waveformTexture, int writeIndex)
{
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

static void UpdateWaveformTexture(PostEffect* pe, const float* waveformHistory)
{
    if (waveformHistory == NULL) {
        return;
    }
    UpdateTexture(pe->waveformTexture, waveformHistory);
}

static void UpdateFFTTexture(PostEffect* pe, const float* fftMagnitude)
{
    if (fftMagnitude == NULL) {
        return;
    }

    float currentMax = 0.0f;
    for (int i = 0; i < FFT_BIN_COUNT; i++) {
        if (fftMagnitude[i] > currentMax) {
            currentMax = fftMagnitude[i];
        }
    }

    const float decayFactor = 0.99f;
    pe->fftMaxMagnitude = pe->fftMaxMagnitude * decayFactor;
    if (currentMax > pe->fftMaxMagnitude) {
        pe->fftMaxMagnitude = currentMax;
    }

    const float maxMag = (pe->fftMaxMagnitude > 0.001f) ? pe->fftMaxMagnitude : 1.0f;

    float normalizedFFT[FFT_BIN_COUNT];
    for (int i = 0; i < FFT_BIN_COUNT; i++) {
        normalizedFFT[i] = fftMagnitude[i] / maxMag;
    }

    UpdateTexture(pe->fftTexture, normalizedFFT);
}

static void ApplySimulationPasses(PostEffect* pe, float deltaTime, int waveformWriteIndex)
{
    ApplyPhysarumPass(pe, deltaTime);
    ApplyCurlFlowPass(pe, deltaTime);
    ApplyCurlAdvectionPass(pe, deltaTime);
    ApplyAttractorFlowPass(pe, deltaTime);
    ApplyBoidsPass(pe, deltaTime);
    ApplyCymaticsPass(pe, deltaTime, pe->waveformTexture, waveformWriteIndex);
}

void RenderPipelineApplyFeedback(PostEffect* pe, float deltaTime, const float* fftMagnitude)
{
    pe->voronoiTime += deltaTime * pe->effects.voronoi.speed;
    pe->infiniteZoomTime += deltaTime * pe->effects.infiniteZoom.speed;
    pe->sineWarpTime += deltaTime * pe->effects.sineWarp.animSpeed;
    pe->waveRippleTime += deltaTime * pe->effects.waveRipple.animSpeed;
    pe->mobiusTime += deltaTime * pe->effects.mobius.animSpeed;
    pe->drosteZoomTime += deltaTime * pe->effects.drosteZoom.speed;
    pe->radialPulseTime += deltaTime * pe->effects.radialPulse.phaseSpeed;
    pe->warpTime += deltaTime * pe->effects.proceduralWarp.warpSpeed;
    UpdateFFTTexture(pe, fftMagnitude);

    pe->currentDeltaTime = deltaTime;
    pe->currentBlurScale = pe->effects.blurScale;

    RenderTexture2D* src = &pe->accumTexture;
    int writeIdx = 0;

    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->feedbackShader, SetupFeedback);
    src = &pe->pingPong[writeIdx];
    writeIdx = 1 - writeIdx;

    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blurHShader, SetupBlurH);
    src = &pe->pingPong[writeIdx];

    RenderPass(pe, src, &pe->accumTexture, pe->blurVShader, SetupBlurV);
}

void RenderPipelineDrawablesFull(PostEffect* pe, DrawableState* state,
                                 Drawable* drawables, int count,
                                 RenderContext* renderCtx)
{
    PostEffectBeginDrawStage(pe);
    const uint64_t tick = DrawableGetTick(state);
    DrawableRenderFull(state, renderCtx, drawables, count, tick);
    PostEffectEndDrawStage();
}

void RenderPipelineExecute(PostEffect* pe, DrawableState* state,
                           Drawable* drawables, int count,
                           RenderContext* renderCtx, float deltaTime,
                           const float* fftMagnitude, const float* waveformHistory,
                           int waveformWriteIndex, Profiler* profiler)
{
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

    // 3. Draw all drawables at configured opacity
    ProfilerBeginZone(profiler, ZONE_DRAWABLES);
    RenderPipelineDrawablesFull(pe, state, drawables, count, renderCtx);
    ProfilerEndZone(profiler, ZONE_DRAWABLES);

    // 4. Output chain
    ProfilerBeginZone(profiler, ZONE_OUTPUT);
    BeginDrawing();
    ClearBackground(BLACK);
    RenderPipelineApplyOutput(pe, DrawableGetTick(state));
    ProfilerEndZone(profiler, ZONE_OUTPUT);

    ProfilerFrameEnd(profiler);
}

void RenderPipelineApplyOutput(PostEffect* pe, uint64_t globalTick)
{
    pe->currentKaleidoRotation += pe->effects.kaleidoscope.rotationSpeed;
    pe->currentKifsRotation += pe->effects.kifs.rotationSpeed;
    pe->currentLatticeFoldRotation += pe->effects.latticeFold.rotationSpeed;

    // Compute Lissajous focal offset (convert Hz to angular frequency)
    const float t = (float)globalTick * 0.016f;
    const float TWO_PI = 2.0f * 3.14159265f;
    const KaleidoscopeConfig* k = &pe->effects.kaleidoscope;
    pe->transformTime = t;
    pe->currentKaleidoFocal[0] = k->focalAmplitude * sinf(t * k->focalFreqX * TWO_PI);
    pe->currentKaleidoFocal[1] = k->focalAmplitude * cosf(t * k->focalFreqY * TWO_PI);

    // Compute wave ripple Lissajous origin
    const WaveRippleConfig* wr = &pe->effects.waveRipple;
    pe->currentWaveRippleOrigin[0] = wr->originX + wr->originAmplitude * sinf(t * wr->originFreqX * TWO_PI);
    pe->currentWaveRippleOrigin[1] = wr->originY + wr->originAmplitude * cosf(t * wr->originFreqY * TWO_PI);

    // Compute mobius Lissajous fixed points
    const MobiusConfig* m = &pe->effects.mobius;
    pe->currentMobiusPoint1[0] = m->point1X + m->pointAmplitude * sinf(t * m->pointFreq1 * TWO_PI);
    pe->currentMobiusPoint1[1] = m->point1Y + m->pointAmplitude * cosf(t * m->pointFreq1 * TWO_PI);
    pe->currentMobiusPoint2[0] = m->point2X + m->pointAmplitude * sinf(t * m->pointFreq2 * TWO_PI);
    pe->currentMobiusPoint2[1] = m->point2Y + m->pointAmplitude * cosf(t * m->pointFreq2 * TWO_PI);

    // Poincare disk rotation accumulation and circular translation motion
    pe->currentPoincareRotation += pe->effects.poincareDisk.rotationSpeed;
    pe->poincareTime += pe->effects.poincareDisk.translationSpeed;
    const PoincareDiskConfig* pd = &pe->effects.poincareDisk;
    pe->currentPoincareTranslation[0] = pd->translationX + pd->translationAmplitude * sinf(pe->poincareTime);
    pe->currentPoincareTranslation[1] = pd->translationY + pd->translationAmplitude * cosf(pe->poincareTime);

    pe->currentHalftoneRotation += pe->effects.halftone.rotationSpeed;

    RenderTexture2D* src = &pe->accumTexture;
    int writeIdx = 0;

    if (pe->physarum != NULL && pe->blendCompositor != NULL &&
        pe->effects.physarum.enabled && pe->effects.physarum.boostIntensity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blendCompositor->shader, SetupTrailBoost);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    if (pe->curlFlow != NULL && pe->blendCompositor != NULL &&
        pe->effects.curlFlow.enabled && pe->effects.curlFlow.boostIntensity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blendCompositor->shader, SetupCurlFlowTrailBoost);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    if (pe->curlAdvection != NULL && pe->blendCompositor != NULL &&
        pe->effects.curlAdvection.enabled && pe->effects.curlAdvection.boostIntensity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blendCompositor->shader, SetupCurlAdvectionTrailBoost);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    if (pe->attractorFlow != NULL && pe->blendCompositor != NULL &&
        pe->effects.attractorFlow.enabled && pe->effects.attractorFlow.boostIntensity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blendCompositor->shader, SetupAttractorFlowTrailBoost);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    if (pe->boids != NULL && pe->blendCompositor != NULL &&
        pe->effects.boids.enabled && pe->effects.boids.boostIntensity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blendCompositor->shader, SetupBoidsTrailBoost);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    if (pe->cymatics != NULL && pe->blendCompositor != NULL &&
        pe->effects.cymatics.enabled && pe->effects.cymatics.boostIntensity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blendCompositor->shader, SetupCymaticsTrailBoost);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    // Chromatic aberration before transforms: the radial "bump" gets warped with content
    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->chromaticShader, SetupChromatic);
    src = &pe->pingPong[writeIdx];
    writeIdx = 1 - writeIdx;

    for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
        const TransformEffectEntry entry = GetTransformEffect(pe, pe->effects.transformOrder[i]);
        if (entry.enabled != NULL && *entry.enabled) {
            RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
            src = &pe->pingPong[writeIdx];
            writeIdx = 1 - writeIdx;
        }
    }

    // Textured shapes sample post-transform output
    BlitTexture(src->texture, &pe->outputTexture, pe->screenWidth, pe->screenHeight);

    if (pe->effects.clarity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->clarityShader, SetupClarity);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->fxaaShader, NULL);
    src = &pe->pingPong[writeIdx];
    writeIdx = 1 - writeIdx;

    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->gammaShader, SetupGamma);
    RenderUtilsDrawFullscreenQuad(pe->pingPong[writeIdx].texture, pe->screenWidth, pe->screenHeight);
}
