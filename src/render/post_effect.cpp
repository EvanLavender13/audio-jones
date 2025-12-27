#include "post_effect.h"
#include "physarum.h"
#include "render_utils.h"
#include "analysis/fft.h"
#include "rlgl.h"
#include "external/glad.h"
#include <cmath>
#include <stdlib.h>

static const char* LOG_PREFIX = "POST_EFFECT";

static void InitFFTTexture(Texture2D* tex)
{
    tex->id = rlLoadTexture(NULL, FFT_BIN_COUNT, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
    tex->width = FFT_BIN_COUNT;
    tex->height = 1;
    tex->mipmaps = 1;
    tex->format = RL_PIXELFORMAT_UNCOMPRESSED_R32;

    SetTextureFilter(*tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(*tex, TEXTURE_WRAP_CLAMP);
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

    static float normalizedFFT[FFT_BIN_COUNT];
    for (int i = 0; i < FFT_BIN_COUNT; i++) {
        normalizedFFT[i] = fftMagnitude[i] / maxMag;
    }

    UpdateTexture(pe->fftTexture, normalizedFFT);
}

static void DrawFullscreenQuad(PostEffect* pe, RenderTexture2D* source)
{
    RenderUtilsDrawFullscreenQuad(source->texture, pe->screenWidth, pe->screenHeight);
}

static void SetWarpUniforms(PostEffect* pe)
{
    SetShaderValue(pe->feedbackShader, pe->warpStrengthLoc,
                   &pe->effects.warpStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->warpScaleLoc,
                   &pe->effects.warpScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->warpOctavesLoc,
                   &pe->effects.warpOctaves, SHADER_UNIFORM_INT);
    SetShaderValue(pe->feedbackShader, pe->warpLacunarityLoc,
                   &pe->effects.warpLacunarity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->warpTimeLoc,
                   &pe->warpTime, SHADER_UNIFORM_FLOAT);
}

typedef void (*ShaderSetupFn)(PostEffect* pe);

static void RenderPass(PostEffect* pe,
                       RenderTexture2D* source,
                       RenderTexture2D* dest,
                       Shader shader,
                       ShaderSetupFn setup)
{
    BeginTextureMode(*dest);
    if (shader.id != 0) {
        BeginShaderMode(shader);
        if (setup != NULL) {
            setup(pe);
        }
    }
    DrawFullscreenQuad(pe, source);
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
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomLoc,
                   &pe->effects.feedbackZoom, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotationLoc,
                   &pe->currentRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDesaturateLoc,
                   &pe->effects.feedbackDesaturate, SHADER_UNIFORM_FLOAT);
    SetWarpUniforms(pe);
}

static void SetupBlurH(PostEffect* pe)
{
    SetShaderValue(pe->blurHShader, pe->blurHScaleLoc,
                   &pe->currentBlurScale, SHADER_UNIFORM_INT);
}

static void SetupBlurV(PostEffect* pe)
{
    SetShaderValue(pe->blurVShader, pe->blurVScaleLoc,
                   &pe->currentBlurScale, SHADER_UNIFORM_INT);
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
                   &pe->currentChromaticOffset, SHADER_UNIFORM_FLOAT);
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

// STAGE 1: Apply feedback/warp effects
// Texture flow: accumTexture -> pingPong[0] -> pingPong[1] -> ... -> accumTexture
// Result: accumTexture contains processed frame
void PostEffectApplyFeedbackStage(PostEffect* pe, float deltaTime, float beatIntensity,
                                   const float* fftMagnitude)
{
    pe->currentBeatIntensity = beatIntensity;
    pe->voronoiTime += deltaTime;
    pe->warpTime += deltaTime * pe->effects.warpSpeed;
    UpdateFFTTexture(pe, fftMagnitude);

    pe->currentDeltaTime = deltaTime;
    pe->currentBlurScale = pe->effects.baseBlurScale + lroundf(beatIntensity * pe->effects.beatBlurScale);
    pe->currentRotation = pe->effects.feedbackRotation;
    if (pe->effects.rotationLFO.enabled) {
        pe->currentRotation *= LFOProcess(&pe->rotationLFOState, &pe->effects.rotationLFO, deltaTime);
    }

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

static bool LoadPostEffectShaders(PostEffect* pe)
{
    pe->feedbackShader = LoadShader(0, "shaders/feedback.fs");
    pe->blurHShader = LoadShader(0, "shaders/blur_h.fs");
    pe->blurVShader = LoadShader(0, "shaders/blur_v.fs");
    pe->chromaticShader = LoadShader(0, "shaders/chromatic.fs");
    pe->kaleidoShader = LoadShader(0, "shaders/kaleidoscope.fs");
    pe->voronoiShader = LoadShader(0, "shaders/voronoi.fs");
    pe->trailBoostShader = LoadShader(0, "shaders/physarum_boost.fs");
    pe->fxaaShader = LoadShader(0, "shaders/fxaa.fs");
    pe->gammaShader = LoadShader(0, "shaders/gamma.fs");

    return pe->feedbackShader.id != 0 && pe->blurHShader.id != 0 &&
           pe->blurVShader.id != 0 && pe->chromaticShader.id != 0 &&
           pe->kaleidoShader.id != 0 && pe->voronoiShader.id != 0 &&
           pe->trailBoostShader.id != 0 && pe->fxaaShader.id != 0 &&
           pe->gammaShader.id != 0;
}

static void GetShaderUniformLocations(PostEffect* pe)
{
    pe->blurHResolutionLoc = GetShaderLocation(pe->blurHShader, "resolution");
    pe->blurVResolutionLoc = GetShaderLocation(pe->blurVShader, "resolution");
    pe->blurHScaleLoc = GetShaderLocation(pe->blurHShader, "blurScale");
    pe->blurVScaleLoc = GetShaderLocation(pe->blurVShader, "blurScale");
    pe->halfLifeLoc = GetShaderLocation(pe->blurVShader, "halfLife");
    pe->deltaTimeLoc = GetShaderLocation(pe->blurVShader, "deltaTime");
    pe->chromaticResolutionLoc = GetShaderLocation(pe->chromaticShader, "resolution");
    pe->chromaticOffsetLoc = GetShaderLocation(pe->chromaticShader, "chromaticOffset");
    pe->kaleidoSegmentsLoc = GetShaderLocation(pe->kaleidoShader, "segments");
    pe->kaleidoRotationLoc = GetShaderLocation(pe->kaleidoShader, "rotation");
    pe->voronoiResolutionLoc = GetShaderLocation(pe->voronoiShader, "resolution");
    pe->voronoiScaleLoc = GetShaderLocation(pe->voronoiShader, "scale");
    pe->voronoiIntensityLoc = GetShaderLocation(pe->voronoiShader, "intensity");
    pe->voronoiTimeLoc = GetShaderLocation(pe->voronoiShader, "time");
    pe->voronoiSpeedLoc = GetShaderLocation(pe->voronoiShader, "speed");
    pe->voronoiEdgeWidthLoc = GetShaderLocation(pe->voronoiShader, "edgeWidth");
    pe->feedbackZoomLoc = GetShaderLocation(pe->feedbackShader, "zoom");
    pe->feedbackRotationLoc = GetShaderLocation(pe->feedbackShader, "rotation");
    pe->feedbackDesaturateLoc = GetShaderLocation(pe->feedbackShader, "desaturate");
    pe->warpStrengthLoc = GetShaderLocation(pe->feedbackShader, "warpStrength");
    pe->warpScaleLoc = GetShaderLocation(pe->feedbackShader, "warpScale");
    pe->warpOctavesLoc = GetShaderLocation(pe->feedbackShader, "warpOctaves");
    pe->warpLacunarityLoc = GetShaderLocation(pe->feedbackShader, "warpLacunarity");
    pe->warpTimeLoc = GetShaderLocation(pe->feedbackShader, "warpTime");
    pe->trailMapLoc = GetShaderLocation(pe->trailBoostShader, "trailMap");
    pe->trailBoostIntensityLoc = GetShaderLocation(pe->trailBoostShader, "boostIntensity");
    pe->trailBlendModeLoc = GetShaderLocation(pe->trailBoostShader, "blendMode");
    pe->fxaaResolutionLoc = GetShaderLocation(pe->fxaaShader, "resolution");
    pe->gammaGammaLoc = GetShaderLocation(pe->gammaShader, "gamma");
}

static void SetResolutionUniforms(PostEffect* pe, int width, int height)
{
    float resolution[2] = { (float)width, (float)height };
    SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->voronoiShader, pe->voronoiResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->fxaaShader, pe->fxaaResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

PostEffect* PostEffectInit(int screenWidth, int screenHeight)
{
    PostEffect* pe = (PostEffect*)calloc(1, sizeof(PostEffect));
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
    pe->currentBeatIntensity = 0.0f;
    LFOStateInit(&pe->rotationLFOState);
    pe->voronoiTime = 0.0f;
    pe->warpTime = 0.0f;

    SetResolutionUniforms(pe, screenWidth, screenHeight);

    RenderUtilsInitTextureHDR(&pe->accumTexture, screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[0], screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[1], screenWidth, screenHeight, LOG_PREFIX);

    if (pe->accumTexture.id == 0 || pe->pingPong[0].id == 0 || pe->pingPong[1].id == 0) {
        TraceLog(LOG_ERROR, "POST_EFFECT: Failed to create render textures");
        UnloadShader(pe->feedbackShader);
        UnloadShader(pe->blurHShader);
        UnloadShader(pe->blurVShader);
        UnloadShader(pe->chromaticShader);
        UnloadShader(pe->kaleidoShader);
        UnloadShader(pe->voronoiShader);
        UnloadShader(pe->trailBoostShader);
        UnloadShader(pe->fxaaShader);
        free(pe);
        return NULL;
    }

    pe->physarum = PhysarumInit(screenWidth, screenHeight, NULL);

    InitFFTTexture(&pe->fftTexture);
    pe->fftMaxMagnitude = 1.0f;
    TraceLog(LOG_INFO, "POST_EFFECT: FFT texture created (%dx%d)", pe->fftTexture.width, pe->fftTexture.height);

    return pe;
}

void PostEffectUninit(PostEffect* pe)
{
    if (pe == NULL) {
        return;
    }

    PhysarumUninit(pe->physarum);
    UnloadTexture(pe->fftTexture);
    UnloadRenderTexture(pe->accumTexture);
    UnloadRenderTexture(pe->pingPong[0]);
    UnloadRenderTexture(pe->pingPong[1]);
    UnloadShader(pe->feedbackShader);
    UnloadShader(pe->blurHShader);
    UnloadShader(pe->blurVShader);
    UnloadShader(pe->chromaticShader);
    UnloadShader(pe->kaleidoShader);
    UnloadShader(pe->voronoiShader);
    UnloadShader(pe->trailBoostShader);
    UnloadShader(pe->fxaaShader);
    UnloadShader(pe->gammaShader);
    free(pe);
}

void PostEffectResize(PostEffect* pe, int width, int height)
{
    if (pe == NULL || (width == pe->screenWidth && height == pe->screenHeight)) {
        return;
    }

    pe->screenWidth = width;
    pe->screenHeight = height;

    UnloadRenderTexture(pe->accumTexture);
    UnloadRenderTexture(pe->pingPong[0]);
    UnloadRenderTexture(pe->pingPong[1]);
    RenderUtilsInitTextureHDR(&pe->accumTexture, width, height, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[0], width, height, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[1], width, height, LOG_PREFIX);

    SetResolutionUniforms(pe, width, height);

    PhysarumResize(pe->physarum, width, height);
}

// STAGE 2: Open accumulation texture for waveform drawing
void PostEffectBeginDrawStage(PostEffect* pe)
{
    BeginTextureMode(pe->accumTexture);
}

// STAGE 2: Close accumulation texture
void PostEffectEndDrawStage(void)
{
    EndTextureMode();
}

// Apply output-only effects (not fed back into accumulation)
// Texture flow: accumTexture -> [trail boost] -> [kaleido] -> chromatic -> FXAA -> gamma -> screen
static void ApplyOutputPipeline(PostEffect* pe, uint64_t globalTick)
{
    pe->currentKaleidoRotation = 0.002f * (float)globalTick;
    pe->currentChromaticOffset = pe->currentBeatIntensity * pe->effects.chromaticMaxOffset;

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
        DrawFullscreenQuad(pe, src);
    EndShaderMode();
}

void PostEffectToScreen(PostEffect* pe, uint64_t globalTick)
{
    ApplyOutputPipeline(pe, globalTick);
}
