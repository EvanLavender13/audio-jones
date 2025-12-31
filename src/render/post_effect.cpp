#include "post_effect.h"
#include "physarum.h"
#include "render_utils.h"
#include "analysis/fft.h"
#include "rlgl.h"
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
    pe->shapeTextureShader = LoadShader(0, "shaders/shape_texture.fs");

    return pe->feedbackShader.id != 0 && pe->blurHShader.id != 0 &&
           pe->blurVShader.id != 0 && pe->chromaticShader.id != 0 &&
           pe->kaleidoShader.id != 0 && pe->voronoiShader.id != 0 &&
           pe->trailBoostShader.id != 0 && pe->fxaaShader.id != 0 &&
           pe->gammaShader.id != 0 && pe->shapeTextureShader.id != 0;
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
    pe->kaleidoTimeLoc = GetShaderLocation(pe->kaleidoShader, "time");
    pe->kaleidoTwistLoc = GetShaderLocation(pe->kaleidoShader, "twist");
    pe->kaleidoFocalLoc = GetShaderLocation(pe->kaleidoShader, "focalOffset");
    pe->kaleidoWarpStrengthLoc = GetShaderLocation(pe->kaleidoShader, "warpStrength");
    pe->kaleidoWarpSpeedLoc = GetShaderLocation(pe->kaleidoShader, "warpSpeed");
    pe->kaleidoNoiseScaleLoc = GetShaderLocation(pe->kaleidoShader, "noiseScale");
    pe->kaleidoModeLoc = GetShaderLocation(pe->kaleidoShader, "mode");
    pe->kaleidoKifsIterationsLoc = GetShaderLocation(pe->kaleidoShader, "kifsIterations");
    pe->kaleidoKifsScaleLoc = GetShaderLocation(pe->kaleidoShader, "kifsScale");
    pe->kaleidoKifsOffsetLoc = GetShaderLocation(pe->kaleidoShader, "kifsOffset");
    pe->voronoiResolutionLoc = GetShaderLocation(pe->voronoiShader, "resolution");
    pe->voronoiScaleLoc = GetShaderLocation(pe->voronoiShader, "scale");
    pe->voronoiIntensityLoc = GetShaderLocation(pe->voronoiShader, "intensity");
    pe->voronoiTimeLoc = GetShaderLocation(pe->voronoiShader, "time");
    pe->voronoiSpeedLoc = GetShaderLocation(pe->voronoiShader, "speed");
    pe->voronoiEdgeWidthLoc = GetShaderLocation(pe->voronoiShader, "edgeWidth");
    pe->feedbackResolutionLoc = GetShaderLocation(pe->feedbackShader, "resolution");
    pe->feedbackDesaturateLoc = GetShaderLocation(pe->feedbackShader, "desaturate");
    pe->feedbackZoomBaseLoc = GetShaderLocation(pe->feedbackShader, "zoomBase");
    pe->feedbackZoomRadialLoc = GetShaderLocation(pe->feedbackShader, "zoomRadial");
    pe->feedbackRotBaseLoc = GetShaderLocation(pe->feedbackShader, "rotBase");
    pe->feedbackRotRadialLoc = GetShaderLocation(pe->feedbackShader, "rotRadial");
    pe->feedbackDxBaseLoc = GetShaderLocation(pe->feedbackShader, "dxBase");
    pe->feedbackDxRadialLoc = GetShaderLocation(pe->feedbackShader, "dxRadial");
    pe->feedbackDyBaseLoc = GetShaderLocation(pe->feedbackShader, "dyBase");
    pe->feedbackDyRadialLoc = GetShaderLocation(pe->feedbackShader, "dyRadial");
    pe->trailMapLoc = GetShaderLocation(pe->trailBoostShader, "trailMap");
    pe->trailBoostIntensityLoc = GetShaderLocation(pe->trailBoostShader, "boostIntensity");
    pe->trailBlendModeLoc = GetShaderLocation(pe->trailBoostShader, "blendMode");
    pe->fxaaResolutionLoc = GetShaderLocation(pe->fxaaShader, "resolution");
    pe->gammaGammaLoc = GetShaderLocation(pe->gammaShader, "gamma");
    pe->shapeTexZoomLoc = GetShaderLocation(pe->shapeTextureShader, "texZoom");
    pe->shapeTexAngleLoc = GetShaderLocation(pe->shapeTextureShader, "texAngle");
    pe->shapeTexBrightnessLoc = GetShaderLocation(pe->shapeTextureShader, "texBrightness");
}

static void SetResolutionUniforms(PostEffect* pe, int width, int height)
{
    float resolution[2] = { (float)width, (float)height };
    SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->voronoiShader, pe->voronoiResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->feedbackShader, pe->feedbackResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
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
    pe->voronoiTime = 0.0f;

    SetResolutionUniforms(pe, screenWidth, screenHeight);

    RenderUtilsInitTextureHDR(&pe->accumTexture, screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[0], screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[1], screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->outputTexture, screenWidth, screenHeight, LOG_PREFIX);

    if (pe->accumTexture.id == 0 || pe->pingPong[0].id == 0 || pe->pingPong[1].id == 0 ||
        pe->outputTexture.id == 0) {
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
    UnloadRenderTexture(pe->outputTexture);
    UnloadShader(pe->feedbackShader);
    UnloadShader(pe->blurHShader);
    UnloadShader(pe->blurVShader);
    UnloadShader(pe->chromaticShader);
    UnloadShader(pe->kaleidoShader);
    UnloadShader(pe->voronoiShader);
    UnloadShader(pe->trailBoostShader);
    UnloadShader(pe->fxaaShader);
    UnloadShader(pe->gammaShader);
    UnloadShader(pe->shapeTextureShader);
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
    UnloadRenderTexture(pe->outputTexture);
    RenderUtilsInitTextureHDR(&pe->accumTexture, width, height, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[0], width, height, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[1], width, height, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->outputTexture, width, height, LOG_PREFIX);

    SetResolutionUniforms(pe, width, height);

    PhysarumResize(pe->physarum, width, height);
}

void PostEffectBeginDrawStage(PostEffect* pe)
{
    BeginTextureMode(pe->accumTexture);
}

void PostEffectEndDrawStage(void)
{
    EndTextureMode();
}
