#include "render_pipeline.h"
#include "blend_compositor.h"
#include "post_effect.h"
#include "drawable.h"
#include "simulation/physarum.h"
#include "simulation/trail_map.h"
#include "simulation/curl_flow.h"
#include "simulation/attractor_flow.h"
#include "render_utils.h"
#include "analysis/fft.h"
#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>

static const char* ZONE_NAMES[ZONE_COUNT] = {
    "Feedback",
    "Physarum",
    "Curl Flow",
    "Attractor",
    "Drawables",
    "Output"
};

void ProfilerInit(Profiler* profiler)
{
    if (profiler == NULL) {
        return;
    }
    memset(profiler, 0, sizeof(Profiler));
    for (int i = 0; i < ZONE_COUNT; i++) {
        profiler->zones[i].name = ZONE_NAMES[i];
    }
    profiler->enabled = true;
}

void ProfilerUninit(Profiler* profiler)
{
    (void)profiler;
}

void ProfilerFrameBegin(Profiler* profiler)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    profiler->frameStartTime = GetTime();
}

void ProfilerFrameEnd(Profiler* profiler)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    for (int i = 0; i < ZONE_COUNT; i++) {
        profiler->zones[i].historyIndex = (profiler->zones[i].historyIndex + 1) % PROFILER_HISTORY_SIZE;
    }
}

void ProfilerBeginZone(Profiler* profiler, ProfileZoneId zone)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    profiler->zones[zone].startTime = GetTime();
}

void ProfilerEndZone(Profiler* profiler, ProfileZoneId zone)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    ProfileZone* z = &profiler->zones[zone];
    const double delta = GetTime() - z->startTime;
    z->lastMs = (float)(delta * 1000.0);
    z->history[z->historyIndex] = z->lastMs;
}

typedef void (*RenderPipelineShaderSetupFn)(PostEffect* pe);

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

static void SetupVoronoi(PostEffect* pe)
{
    SetShaderValue(pe->voronoiShader, pe->voronoiScaleLoc,
                   &pe->effects.voronoi.scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiIntensityLoc,
                   &pe->effects.voronoi.intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiTimeLoc,
                   &pe->voronoiTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiSpeedLoc,
                   &pe->effects.voronoi.speed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeWidthLoc,
                   &pe->effects.voronoi.edgeWidth, SHADER_UNIFORM_FLOAT);
}

static void SetupFeedback(PostEffect* pe)
{
    SetShaderValue(pe->feedbackShader, pe->feedbackDesaturateLoc,
                   &pe->effects.feedbackDesaturate, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomBaseLoc,
                   &pe->effects.flowField.zoomBase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomRadialLoc,
                   &pe->effects.flowField.zoomRadial, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotBaseLoc,
                   &pe->effects.flowField.rotBase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotRadialLoc,
                   &pe->effects.flowField.rotRadial, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDxBaseLoc,
                   &pe->effects.flowField.dxBase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDxRadialLoc,
                   &pe->effects.flowField.dxRadial, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyBaseLoc,
                   &pe->effects.flowField.dyBase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyRadialLoc,
                   &pe->effects.flowField.dyRadial, SHADER_UNIFORM_FLOAT);
}

static void SetupBlurH(PostEffect* pe)
{
    SetShaderValue(pe->blurHShader, pe->blurHScaleLoc,
                   &pe->currentBlurScale, SHADER_UNIFORM_FLOAT);
}

static void SetupBlurV(PostEffect* pe)
{
    SetShaderValue(pe->blurVShader, pe->blurVScaleLoc,
                   &pe->currentBlurScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->blurVShader, pe->halfLifeLoc,
                   &pe->effects.halfLife, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->blurVShader, pe->deltaTimeLoc,
                   &pe->currentDeltaTime, SHADER_UNIFORM_FLOAT);
}

static void SetupTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->physarum->trailMap),
                         pe->effects.physarum.boostIntensity,
                         pe->effects.physarum.blendMode);
}

static void SetupCurlFlowTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->curlFlow->trailMap),
                         pe->effects.curlFlow.boostIntensity,
                         pe->effects.curlFlow.blendMode);
}

static void SetupAttractorFlowTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->attractorFlow->trailMap),
                         pe->effects.attractorFlow.boostIntensity,
                         pe->effects.attractorFlow.blendMode);
}

static void SetupKaleido(PostEffect* pe)
{
    const KaleidoscopeConfig* k = &pe->effects.kaleidoscope;
    const int mode = (int)k->mode;
    SetShaderValue(pe->kaleidoShader, pe->kaleidoModeLoc,
                   &mode, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoSegmentsLoc,
                   &k->segments, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoRotationLoc,
                   &pe->currentKaleidoRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoTimeLoc,
                   &pe->currentKaleidoTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoTwistLoc,
                   &k->twistAmount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoFocalLoc,
                   pe->currentKaleidoFocal, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoWarpStrengthLoc,
                   &k->warpStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoWarpSpeedLoc,
                   &k->warpSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoNoiseScaleLoc,
                   &k->noiseScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsIterationsLoc,
                   &k->kifsIterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsScaleLoc,
                   &k->kifsScale, SHADER_UNIFORM_FLOAT);
    const float kifsOffset[2] = { k->kifsOffsetX, k->kifsOffsetY };
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsOffsetLoc,
                   kifsOffset, SHADER_UNIFORM_VEC2);
}

static void SetupInfiniteZoom(PostEffect* pe)
{
    const InfiniteZoomConfig* iz = &pe->effects.infiniteZoom;
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomTimeLoc,
                   &pe->infiniteZoomTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomSpeedLoc,
                   &iz->speed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomZoomDepthLoc,
                   &iz->zoomDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomFocalLoc,
                   pe->infiniteZoomFocal, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomLayersLoc,
                   &iz->layers, SHADER_UNIFORM_INT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomSpiralTurnsLoc,
                   &iz->spiralTurns, SHADER_UNIFORM_FLOAT);
}

static void SetupChromatic(PostEffect* pe)
{
    SetShaderValue(pe->chromaticShader, pe->chromaticOffsetLoc,
                   &pe->effects.chromaticOffset, SHADER_UNIFORM_FLOAT);
}

static void SetupGamma(PostEffect* pe)
{
    SetShaderValue(pe->gammaShader, pe->gammaGammaLoc,
                   &pe->effects.gamma, SHADER_UNIFORM_FLOAT);
}

static void SetupClarity(PostEffect* pe)
{
    SetShaderValue(pe->clarityShader, pe->clarityAmountLoc,
                   &pe->effects.clarity, SHADER_UNIFORM_FLOAT);
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

void RenderPipelineApplyFeedback(PostEffect* pe, float deltaTime, const float* fftMagnitude,
                                 Profiler* profiler)
{
    pe->voronoiTime += deltaTime;
    pe->infiniteZoomTime += deltaTime;
    UpdateFFTTexture(pe, fftMagnitude);

    pe->currentDeltaTime = deltaTime;
    pe->currentBlurScale = pe->effects.blurScale;

    ProfilerBeginZone(profiler, ZONE_PHYSARUM);
    ApplyPhysarumPass(pe, deltaTime);
    ProfilerEndZone(profiler, ZONE_PHYSARUM);

    ProfilerBeginZone(profiler, ZONE_CURL_FLOW);
    ApplyCurlFlowPass(pe, deltaTime);
    ProfilerEndZone(profiler, ZONE_CURL_FLOW);

    ProfilerBeginZone(profiler, ZONE_ATTRACTOR);
    ApplyAttractorFlowPass(pe, deltaTime);
    ProfilerEndZone(profiler, ZONE_ATTRACTOR);

    RenderTexture2D* src = &pe->accumTexture;
    int writeIdx = 0;

    if (pe->effects.voronoi.enabled) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->voronoiShader, SetupVoronoi);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

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
                           const float* fftMagnitude, Profiler* profiler)
{
    ProfilerFrameBegin(profiler);

    // 1. Apply feedback effects (warp, blur, decay) + simulation updates
    ProfilerBeginZone(profiler, ZONE_FEEDBACK);
    RenderPipelineApplyFeedback(pe, deltaTime, fftMagnitude, profiler);
    ProfilerEndZone(profiler, ZONE_FEEDBACK);

    // 2. Draw all drawables at configured opacity
    ProfilerBeginZone(profiler, ZONE_DRAWABLES);
    RenderPipelineDrawablesFull(pe, state, drawables, count, renderCtx);
    ProfilerEndZone(profiler, ZONE_DRAWABLES);

    // 3. Output chain
    ProfilerBeginZone(profiler, ZONE_OUTPUT);
    BeginDrawing();
    ClearBackground(BLACK);
    RenderPipelineApplyOutput(pe, DrawableGetTick(state));
    ProfilerEndZone(profiler, ZONE_OUTPUT);

    ProfilerFrameEnd(profiler);
}

void RenderPipelineApplyOutput(PostEffect* pe, uint64_t globalTick)
{
    pe->currentKaleidoRotation = pe->effects.kaleidoscope.rotationSpeed * (float)globalTick;

    // Compute Lissajous focal offset
    const float t = (float)globalTick * 0.016f;
    const KaleidoscopeConfig* k = &pe->effects.kaleidoscope;
    pe->currentKaleidoTime = t;
    pe->currentKaleidoFocal[0] = k->focalAmplitude * sinf(t * k->focalFreqX);
    pe->currentKaleidoFocal[1] = k->focalAmplitude * cosf(t * k->focalFreqY);

    // Compute infinite zoom Lissajous focal offset
    const InfiniteZoomConfig* iz = &pe->effects.infiniteZoom;
    pe->infiniteZoomFocal[0] = iz->focalAmplitude * sinf(t * iz->focalFreqX);
    pe->infiniteZoomFocal[1] = iz->focalAmplitude * cosf(t * iz->focalFreqY);

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

    if (pe->attractorFlow != NULL && pe->blendCompositor != NULL &&
        pe->effects.attractorFlow.enabled && pe->effects.attractorFlow.boostIntensity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->blendCompositor->shader, SetupAttractorFlowTrailBoost);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    if (pe->effects.kaleidoscope.segments > 1) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->kaleidoShader, SetupKaleido);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    if (pe->effects.infiniteZoom.enabled) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->infiniteZoomShader, SetupInfiniteZoom);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    // Textured shapes sample before chromatic aberration distorts UV coordinates
    BlitTexture(src->texture, &pe->outputTexture, pe->screenWidth, pe->screenHeight);

    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->chromaticShader, SetupChromatic);
    src = &pe->pingPong[writeIdx];
    writeIdx = 1 - writeIdx;

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
