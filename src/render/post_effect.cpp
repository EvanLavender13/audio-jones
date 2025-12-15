#include "post_effect.h"
#include "physarum.h"
#include "rlgl.h"
#include "external/glad.h"
#include <cmath>
#include <stdlib.h>

// Create HDR render texture with RGBA32F format for maximum precision
static void InitRenderTextureHDR(RenderTexture2D* tex, int width, int height)
{
    tex->id = rlLoadFramebuffer();
    if (tex->id == 0) {
        TraceLog(LOG_WARNING, "POST_EFFECT: Failed to create HDR framebuffer");
        return;
    }

    rlEnableFramebuffer(tex->id);

    // Create RGBA32F texture (full 32-bit float per channel)
    tex->texture.id = rlLoadTexture(NULL, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
    tex->texture.width = width;
    tex->texture.height = height;
    tex->texture.mipmaps = 1;
    tex->texture.format = RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;

    // Attach texture to framebuffer
    rlFramebufferAttach(tex->id, tex->texture.id, RL_ATTACHMENT_COLOR_CHANNEL0,
                        RL_ATTACHMENT_TEXTURE2D, 0);

    if (!rlFramebufferComplete(tex->id)) {
        TraceLog(LOG_WARNING, "POST_EFFECT: HDR framebuffer incomplete, falling back to standard");
        rlUnloadFramebuffer(tex->id);
        rlUnloadTexture(tex->texture.id);
        *tex = LoadRenderTexture(width, height);
    }

    rlDisableFramebuffer();

    tex->depth.id = 0;

    SetTextureWrap(tex->texture, TEXTURE_WRAP_CLAMP);
    BeginTextureMode(*tex);
    ClearBackground(BLACK);
    EndTextureMode();
}

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

static void DrawFullscreenQuad(PostEffect* pe, RenderTexture2D* source)
{
    DrawTextureRec(source->texture,
        {0, 0, (float)pe->screenWidth, (float)-pe->screenHeight},
        {0, 0}, WHITE);
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
    if (pe->physarum == NULL || !pe->effects.physarum.enabled) {
        return;
    }

    PhysarumApplyConfig(pe->physarum, &pe->effects.physarum);
    PhysarumUpdate(pe->physarum, deltaTime);
    PhysarumProcessTrails(pe->physarum, deltaTime);

    BeginTextureMode(pe->accumTexture);
    PhysarumDrawDebug(pe->physarum);
    EndTextureMode();
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

    pe->feedbackShader = LoadShader(0, "shaders/feedback.fs");
    pe->blurHShader = LoadShader(0, "shaders/blur_h.fs");
    pe->blurVShader = LoadShader(0, "shaders/blur_v.fs");
    pe->chromaticShader = LoadShader(0, "shaders/chromatic.fs");
    pe->kaleidoShader = LoadShader(0, "shaders/kaleidoscope.fs");
    pe->voronoiShader = LoadShader(0, "shaders/voronoi.fs");

    if (pe->feedbackShader.id == 0 || pe->blurHShader.id == 0 ||
        pe->blurVShader.id == 0 || pe->chromaticShader.id == 0 ||
        pe->kaleidoShader.id == 0 || pe->voronoiShader.id == 0) {
        TraceLog(LOG_ERROR, "POST_EFFECT: Failed to load shaders");
        free(pe);
        return NULL;
    }

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
    pe->currentBeatIntensity = 0.0f;
    LFOStateInit(&pe->rotationLFOState);
    pe->voronoiTime = 0.0f;

    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->voronoiShader, pe->voronoiResolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    // Create HDR render textures for ping-pong blur (RGBA32F for physarum precision)
    InitRenderTextureHDR(&pe->accumTexture, screenWidth, screenHeight);
    InitRenderTextureHDR(&pe->tempTexture, screenWidth, screenHeight);

    if (pe->accumTexture.id == 0 || pe->tempTexture.id == 0) {
        TraceLog(LOG_ERROR, "POST_EFFECT: Failed to create render textures");
        UnloadShader(pe->blurHShader);
        UnloadShader(pe->blurVShader);
        UnloadShader(pe->chromaticShader);
        free(pe);
        return NULL;
    }

    pe->physarum = PhysarumInit(screenWidth, screenHeight, NULL);

    return pe;
}

void PostEffectUninit(PostEffect* pe)
{
    if (pe == NULL) {
        return;
    }

    PhysarumUninit(pe->physarum);
    UnloadRenderTexture(pe->accumTexture);
    UnloadRenderTexture(pe->tempTexture);
    UnloadShader(pe->feedbackShader);
    UnloadShader(pe->blurHShader);
    UnloadShader(pe->blurVShader);
    UnloadShader(pe->chromaticShader);
    UnloadShader(pe->kaleidoShader);
    UnloadShader(pe->voronoiShader);
    free(pe);
}

void PostEffectResize(PostEffect* pe, int width, int height)
{
    if (pe == NULL || (width == pe->screenWidth && height == pe->screenHeight)) {
        return;
    }

    pe->screenWidth = width;
    pe->screenHeight = height;

    // Recreate render textures at new size
    UnloadRenderTexture(pe->accumTexture);
    UnloadRenderTexture(pe->tempTexture);
    InitRenderTextureHDR(&pe->accumTexture, width, height);
    InitRenderTextureHDR(&pe->tempTexture, width, height);

    float resolution[2] = { (float)width, (float)height };
    SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->voronoiShader, pe->voronoiResolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    PhysarumResize(pe->physarum, width, height);
}

void PostEffectBeginAccum(PostEffect* pe, float deltaTime, float beatIntensity)
{
    pe->currentBeatIntensity = beatIntensity;
    pe->voronoiTime += deltaTime;

    const int blurScale = pe->effects.baseBlurScale + lroundf(beatIntensity * pe->effects.beatBlurScale);

    float effectiveRotation = pe->effects.feedbackRotation;
    if (pe->effects.rotationLFO.enabled) {
        const float lfoValue = LFOProcess(&pe->rotationLFOState,
                                          &pe->effects.rotationLFO,
                                          deltaTime);
        effectiveRotation *= lfoValue;
    }

    ApplyPhysarumPass(pe, deltaTime);
    ApplyVoronoiPass(pe);
    ApplyFeedbackPass(pe, effectiveRotation);
    ApplyBlurPass(pe, blurScale, deltaTime);

    BeginTextureMode(pe->accumTexture);
        DrawFullscreenQuad(pe, &pe->tempTexture);
    // Leave accumTexture open for caller to draw new content
}

void PostEffectEndAccum(PostEffect* pe, uint64_t globalTick)
{
    EndTextureMode();

    if (pe->effects.kaleidoSegments > 1) {
        const float rotation = 0.002f * (float)globalTick;

        BeginTextureMode(pe->tempTexture);
        BeginShaderMode(pe->kaleidoShader);
            SetShaderValue(pe->kaleidoShader, pe->kaleidoSegmentsLoc,
                           &pe->effects.kaleidoSegments, SHADER_UNIFORM_INT);
            SetShaderValue(pe->kaleidoShader, pe->kaleidoRotationLoc,
                           &rotation, SHADER_UNIFORM_FLOAT);
            DrawFullscreenQuad(pe, &pe->accumTexture);
        EndShaderMode();
        EndTextureMode();

        BeginTextureMode(pe->accumTexture);
            DrawFullscreenQuad(pe, &pe->tempTexture);
        EndTextureMode();
    }
}

void PostEffectToScreen(PostEffect* pe)
{
    const float chromaticOffset = pe->currentBeatIntensity * pe->effects.chromaticMaxOffset;

    if (pe->effects.chromaticMaxOffset == 0 || chromaticOffset < 0.01f) {
        DrawFullscreenQuad(pe, &pe->accumTexture);
        return;
    }

    BeginShaderMode(pe->chromaticShader);
        SetShaderValue(pe->chromaticShader, pe->chromaticOffsetLoc,
                       &chromaticOffset, SHADER_UNIFORM_FLOAT);
        DrawFullscreenQuad(pe, &pe->accumTexture);
    EndShaderMode();
}
