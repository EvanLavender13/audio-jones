#include "post_effect.h"
#include "physarum.h"
#include "render_utils.h"
#include "analysis/fft.h"
#include "rlgl.h"
#include "external/glad.h"
#include <cmath>
#include <stdlib.h>

static const char* LOG_PREFIX = "POST_EFFECT";

static void InitRenderTexture(RenderTexture2D* tex, int width, int height)
{
    *tex = LoadRenderTexture(width, height);
    if (tex->id == 0) {
        return;
    }
    SetTextureWrap(tex->texture, TEXTURE_WRAP_CLAMP);
    BeginTextureMode(*tex);
    ClearBackground(BLACK);
    EndTextureMode();
}

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

static void ApplyVoronoiPass(PostEffect* pe)
{
    if (pe->effects.voronoiIntensity <= 0.0f) {
        return;
    }

    BeginTextureMode(pe->tempTexture);
    BeginShaderMode(pe->voronoiShader);
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
        DrawFullscreenQuad(pe, &pe->accumTexture);
    EndShaderMode();
    EndTextureMode();

    BeginTextureMode(pe->accumTexture);
        DrawFullscreenQuad(pe, &pe->tempTexture);
    EndTextureMode();
}

static void ApplyFeedbackPass(PostEffect* pe, float rotation)
{
    BeginTextureMode(pe->tempTexture);
    BeginShaderMode(pe->feedbackShader);
        SetShaderValue(pe->feedbackShader, pe->feedbackZoomLoc,
                       &pe->effects.feedbackZoom, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pe->feedbackShader, pe->feedbackRotationLoc,
                       &rotation, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pe->feedbackShader, pe->feedbackDesaturateLoc,
                       &pe->effects.feedbackDesaturate, SHADER_UNIFORM_FLOAT);
        SetWarpUniforms(pe);
        DrawFullscreenQuad(pe, &pe->accumTexture);
    EndShaderMode();
    EndTextureMode();
}

static void ApplyBlurPass(PostEffect* pe, int blurScale, float deltaTime)
{
    BeginTextureMode(pe->accumTexture);
    BeginShaderMode(pe->blurHShader);
        SetShaderValue(pe->blurHShader, pe->blurHScaleLoc, &blurScale, SHADER_UNIFORM_INT);
        DrawFullscreenQuad(pe, &pe->tempTexture);
    EndShaderMode();
    EndTextureMode();

    BeginTextureMode(pe->tempTexture);
    BeginShaderMode(pe->blurVShader);
        SetShaderValue(pe->blurVShader, pe->blurVScaleLoc, &blurScale, SHADER_UNIFORM_INT);
        SetShaderValue(pe->blurVShader, pe->halfLifeLoc, &pe->effects.halfLife, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pe->blurVShader, pe->deltaTimeLoc, &deltaTime, SHADER_UNIFORM_FLOAT);
        DrawFullscreenQuad(pe, &pe->accumTexture);
    EndShaderMode();
    EndTextureMode();
}

static void ApplyPhysarumPass(PostEffect* pe, float deltaTime)
{
    if (pe->physarum == NULL) {
        return;
    }

    if (pe->effects.physarum.enabled) {
        PhysarumApplyConfig(pe->physarum, &pe->effects.physarum);
        PhysarumUpdate(pe->physarum, deltaTime, pe->accumTexture.texture, pe->fftTexture,
                       pe->currentBeatIntensity);
        PhysarumProcessTrails(pe->physarum, deltaTime);
    }

    if (pe->effects.physarum.debugOverlay) {
        BeginTextureMode(pe->accumTexture);
        PhysarumDrawDebug(pe->physarum);
        EndTextureMode();
    }
}

// Apply feedback effects that persist between frames
// Texture flow: accumTexture <-> tempTexture (ping-pong)
// Result: accumTexture contains processed frame, left in render mode for waveform drawing
static void ApplyAccumulationPipeline(PostEffect* pe, float deltaTime,
                                       float rotation, int blurScale)
{
    ApplyPhysarumPass(pe, deltaTime);
    ApplyVoronoiPass(pe);
    ApplyFeedbackPass(pe, rotation);
    ApplyBlurPass(pe, blurScale, deltaTime);

    BeginTextureMode(pe->accumTexture);
        DrawFullscreenQuad(pe, &pe->tempTexture);
    // Leave accumTexture open for caller (PostEffectBeginAccum)
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

    return pe->feedbackShader.id != 0 && pe->blurHShader.id != 0 &&
           pe->blurVShader.id != 0 && pe->chromaticShader.id != 0 &&
           pe->kaleidoShader.id != 0 && pe->voronoiShader.id != 0 &&
           pe->trailBoostShader.id != 0 && pe->fxaaShader.id != 0;
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
    pe->fxaaResolutionLoc = GetShaderLocation(pe->fxaaShader, "resolution");
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
    RenderUtilsInitTextureHDR(&pe->tempTexture, screenWidth, screenHeight, LOG_PREFIX);
    InitRenderTexture(&pe->kaleidoTexture, screenWidth, screenHeight);

    if (pe->accumTexture.id == 0 || pe->tempTexture.id == 0 || pe->kaleidoTexture.id == 0) {
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
    UnloadRenderTexture(pe->tempTexture);
    UnloadRenderTexture(pe->kaleidoTexture);
    UnloadShader(pe->feedbackShader);
    UnloadShader(pe->blurHShader);
    UnloadShader(pe->blurVShader);
    UnloadShader(pe->chromaticShader);
    UnloadShader(pe->kaleidoShader);
    UnloadShader(pe->voronoiShader);
    UnloadShader(pe->trailBoostShader);
    UnloadShader(pe->fxaaShader);
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
    UnloadRenderTexture(pe->tempTexture);
    UnloadRenderTexture(pe->kaleidoTexture);
    RenderUtilsInitTextureHDR(&pe->accumTexture, width, height, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->tempTexture, width, height, LOG_PREFIX);
    InitRenderTexture(&pe->kaleidoTexture, width, height);

    SetResolutionUniforms(pe, width, height);

    PhysarumResize(pe->physarum, width, height);
}

void PostEffectBeginAccum(PostEffect* pe, float deltaTime, float beatIntensity,
                          const float* fftMagnitude)
{
    pe->currentBeatIntensity = beatIntensity;
    pe->voronoiTime += deltaTime;
    pe->warpTime += deltaTime * pe->effects.warpSpeed;

    UpdateFFTTexture(pe, fftMagnitude);

    const int blurScale = pe->effects.baseBlurScale + lroundf(beatIntensity * pe->effects.beatBlurScale);

    float effectiveRotation = pe->effects.feedbackRotation;
    if (pe->effects.rotationLFO.enabled) {
        const float lfoValue = LFOProcess(&pe->rotationLFOState,
                                          &pe->effects.rotationLFO,
                                          deltaTime);
        effectiveRotation *= lfoValue;
    }

    ApplyAccumulationPipeline(pe, deltaTime, effectiveRotation, blurScale);
}

void PostEffectEndAccum(void)
{
    EndTextureMode();
}

// Apply output-only effects (not fed back into accumulation)
// Texture flow: accumTexture -> [trail boost] -> [kaleidoscope] -> chromatic -> FXAA -> screen
static void ApplyOutputPipeline(PostEffect* pe, uint64_t globalTick)
{
    RenderTexture2D* currentSource = &pe->accumTexture;

    // Trail boost (optional, writes to tempTexture)
    if (pe->physarum != NULL && pe->effects.physarum.boostIntensity > 0.0f) {
        BeginTextureMode(pe->tempTexture);
        BeginShaderMode(pe->trailBoostShader);
            SetShaderValueTexture(pe->trailBoostShader, pe->trailMapLoc,
                                  pe->physarum->trailMap.texture);
            SetShaderValue(pe->trailBoostShader, pe->trailBoostIntensityLoc,
                           &pe->effects.physarum.boostIntensity, SHADER_UNIFORM_FLOAT);
            DrawFullscreenQuad(pe, currentSource);
        EndShaderMode();
        EndTextureMode();
        currentSource = &pe->tempTexture;
    }

    // Kaleidoscope (optional, writes to kaleidoTexture)
    if (pe->effects.kaleidoSegments > 1) {
        const float rotation = 0.002f * (float)globalTick;  // radians per tick

        BeginTextureMode(pe->kaleidoTexture);
        BeginShaderMode(pe->kaleidoShader);
            SetShaderValue(pe->kaleidoShader, pe->kaleidoSegmentsLoc,
                           &pe->effects.kaleidoSegments, SHADER_UNIFORM_INT);
            SetShaderValue(pe->kaleidoShader, pe->kaleidoRotationLoc,
                           &rotation, SHADER_UNIFORM_FLOAT);
            DrawFullscreenQuad(pe, currentSource);
        EndShaderMode();
        EndTextureMode();
        currentSource = &pe->kaleidoTexture;
    }

    // Select FXAA input buffer (must differ from currentSource to avoid read/write conflict)
    RenderTexture2D* fxaaInput = (currentSource == &pe->tempTexture)
                                 ? &pe->kaleidoTexture
                                 : &pe->tempTexture;

    // Chromatic aberration (renders to fxaaInput)
    const float chromaticOffset = pe->currentBeatIntensity * pe->effects.chromaticMaxOffset;
    const bool chromaticEnabled = pe->effects.chromaticMaxOffset > 0 && chromaticOffset >= 0.01f;

    BeginTextureMode(*fxaaInput);
    if (chromaticEnabled) {
        BeginShaderMode(pe->chromaticShader);
        SetShaderValue(pe->chromaticShader, pe->chromaticOffsetLoc,
                       &chromaticOffset, SHADER_UNIFORM_FLOAT);
    }
    DrawFullscreenQuad(pe, currentSource);
    if (chromaticEnabled) {
        EndShaderMode();
    }
    EndTextureMode();

    // FXAA (final pass, renders to screen)
    BeginShaderMode(pe->fxaaShader);
        DrawFullscreenQuad(pe, fxaaInput);
    EndShaderMode();
}

void PostEffectToScreen(PostEffect* pe, uint64_t globalTick)
{
    ApplyOutputPipeline(pe, globalTick);
}
