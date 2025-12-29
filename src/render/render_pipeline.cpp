#include "render_pipeline.h"
#include "post_effect.h"
#include "physarum.h"
#include "render_utils.h"
#include "analysis/fft.h"
#include "raylib.h"

typedef void (*RenderPipelineShaderSetupFn)(PostEffect* pe);

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
                   &pe->effects.voronoiScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiIntensityLoc,
                   &pe->effects.voronoiIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiTimeLoc,
                   &pe->voronoiTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiSpeedLoc,
                   &pe->effects.voronoiSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeWidthLoc,
                   &pe->effects.voronoiEdgeWidth, SHADER_UNIFORM_FLOAT);
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
    const int blendMode = (int)pe->effects.physarum.trailBlendMode;
    SetShaderValueTexture(pe->trailBoostShader, pe->trailMapLoc,
                          pe->physarum->trailMap.texture);
    SetShaderValue(pe->trailBoostShader, pe->trailBoostIntensityLoc,
                   &pe->effects.physarum.boostIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->trailBoostShader, pe->trailBlendModeLoc,
                   &blendMode, SHADER_UNIFORM_INT);
}

static void SetupKaleido(PostEffect* pe)
{
    SetShaderValue(pe->kaleidoShader, pe->kaleidoSegmentsLoc,
                   &pe->effects.kaleidoSegments, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoRotationLoc,
                   &pe->currentKaleidoRotation, SHADER_UNIFORM_FLOAT);
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

static void ApplyPhysarumPass(PostEffect* pe, float deltaTime)
{
    if (pe->physarum == NULL) {
        return;
    }

    if (pe->effects.physarum.enabled) {
        PhysarumApplyConfig(pe->physarum, &pe->effects.physarum);
        PhysarumUpdate(pe->physarum, deltaTime, pe->accumTexture.texture, pe->fftTexture);
        PhysarumProcessTrails(pe->physarum, deltaTime);
    }

    if (pe->effects.physarum.debugOverlay) {
        BeginTextureMode(pe->accumTexture);
        PhysarumDrawDebug(pe->physarum);
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

void RenderPipelineApplyFeedback(PostEffect* pe, float deltaTime, const float* fftMagnitude)
{
    pe->voronoiTime += deltaTime;
    UpdateFFTTexture(pe, fftMagnitude);

    pe->currentDeltaTime = deltaTime;
    pe->currentBlurScale = pe->effects.blurScale;

    ApplyPhysarumPass(pe, deltaTime);

    RenderTexture2D* src = &pe->accumTexture;
    int writeIdx = 0;

    if (pe->effects.voronoiIntensity > 0.0f) {
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

void RenderPipelineUpdateShapeSample(PostEffect* pe)
{
    // Prevents feedback loop: shapes read from this copy while drawing to accumTexture
    BeginTextureMode(pe->shapeSampleTex);
    ClearBackground(BLACK);
    DrawTextureRec(pe->accumTexture.texture,
                   (Rectangle){ 0, 0, (float)pe->screenWidth, (float)-pe->screenHeight },
                   (Vector2){ 0, 0 }, WHITE);
    EndTextureMode();
}

void RenderPipelineApplyOutput(PostEffect* pe, uint64_t globalTick)
{
    pe->currentKaleidoRotation = pe->effects.kaleidoRotationSpeed * (float)globalTick;

    RenderTexture2D* src = &pe->accumTexture;
    int writeIdx = 0;

    if (pe->physarum != NULL && pe->effects.physarum.boostIntensity > 0.0f) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->trailBoostShader, SetupTrailBoost);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    if (pe->effects.kaleidoSegments > 1) {
        RenderPass(pe, src, &pe->pingPong[writeIdx], pe->kaleidoShader, SetupKaleido);
        src = &pe->pingPong[writeIdx];
        writeIdx = 1 - writeIdx;
    }

    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->chromaticShader, SetupChromatic);
    src = &pe->pingPong[writeIdx];
    writeIdx = 1 - writeIdx;

    RenderPass(pe, src, &pe->pingPong[writeIdx], pe->fxaaShader, NULL);
    src = &pe->pingPong[writeIdx];

    BeginShaderMode(pe->gammaShader);
        SetupGamma(pe);
        RenderUtilsDrawFullscreenQuad(src->texture, pe->screenWidth, pe->screenHeight);
    EndShaderMode();
}
