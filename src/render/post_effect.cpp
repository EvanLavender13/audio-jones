#include "post_effect.h"
#include <cmath>
#include <stdlib.h>

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

    if (pe->feedbackShader.id == 0 || pe->blurHShader.id == 0 ||
        pe->blurVShader.id == 0 || pe->chromaticShader.id == 0) {
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
    pe->feedbackZoomLoc = GetShaderLocation(pe->feedbackShader, "zoom");
    pe->feedbackRotationLoc = GetShaderLocation(pe->feedbackShader, "rotation");
    pe->feedbackDesaturateLoc = GetShaderLocation(pe->feedbackShader, "desaturate");
    pe->currentBeatIntensity = 0.0f;
    LFOStateInit(&pe->rotationLFOState);

    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    // Create render textures for ping-pong blur
    InitRenderTexture(&pe->accumTexture, screenWidth, screenHeight);
    InitRenderTexture(&pe->tempTexture, screenWidth, screenHeight);

    if (pe->accumTexture.id == 0 || pe->tempTexture.id == 0) {
        TraceLog(LOG_ERROR, "POST_EFFECT: Failed to create render textures");
        UnloadShader(pe->blurHShader);
        UnloadShader(pe->blurVShader);
        UnloadShader(pe->chromaticShader);
        free(pe);
        return NULL;
    }

    return pe;
}

void PostEffectUninit(PostEffect* pe)
{
    if (pe == NULL) {
        return;
    }

    UnloadRenderTexture(pe->accumTexture);
    UnloadRenderTexture(pe->tempTexture);
    UnloadShader(pe->feedbackShader);
    UnloadShader(pe->blurHShader);
    UnloadShader(pe->blurVShader);
    UnloadShader(pe->chromaticShader);
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
    InitRenderTexture(&pe->accumTexture, width, height);
    InitRenderTexture(&pe->tempTexture, width, height);

    float resolution[2] = { (float)width, (float)height };
    SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

void PostEffectBeginAccum(PostEffect* pe, float deltaTime, float beatIntensity)
{
    pe->currentBeatIntensity = beatIntensity;

    int blurScale = pe->effects.baseBlurScale + lroundf(beatIntensity * pe->effects.beatBlurScale);

    // Compute effective rotation with LFO modulation
    float effectiveRotation = pe->effects.feedbackRotation;
    if (pe->effects.rotationLFO.enabled) {
        float lfoValue = LFOProcess(&pe->rotationLFOState,
                                     &pe->effects.rotationLFO,
                                     deltaTime);
        effectiveRotation *= lfoValue;  // Oscillates between -rotation and +rotation
    }

    // Feedback pass: zoom/rotate previous frame (accumTexture -> tempTexture)
    BeginTextureMode(pe->tempTexture);
    BeginShaderMode(pe->feedbackShader);
        SetShaderValue(pe->feedbackShader, pe->feedbackZoomLoc,
                       &pe->effects.feedbackZoom, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pe->feedbackShader, pe->feedbackRotationLoc,
                       &effectiveRotation, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pe->feedbackShader, pe->feedbackDesaturateLoc,
                       &pe->effects.feedbackDesaturate, SHADER_UNIFORM_FLOAT);
        DrawTextureRec(pe->accumTexture.texture,
            {0, 0, (float)pe->screenWidth, (float)-pe->screenHeight},
            {0, 0}, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Horizontal blur (tempTexture -> accumTexture)
    BeginTextureMode(pe->accumTexture);
    BeginShaderMode(pe->blurHShader);
        SetShaderValue(pe->blurHShader, pe->blurHScaleLoc, &blurScale, SHADER_UNIFORM_INT);
        DrawTextureRec(pe->tempTexture.texture,
            {0, 0, (float)pe->screenWidth, (float)-pe->screenHeight},
            {0, 0}, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Vertical blur + decay (accumTexture -> tempTexture)
    BeginTextureMode(pe->tempTexture);
    BeginShaderMode(pe->blurVShader);
        SetShaderValue(pe->blurVShader, pe->blurVScaleLoc, &blurScale, SHADER_UNIFORM_INT);
        SetShaderValue(pe->blurVShader, pe->halfLifeLoc, &pe->effects.halfLife, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pe->blurVShader, pe->deltaTimeLoc, &deltaTime, SHADER_UNIFORM_FLOAT);
        DrawTextureRec(pe->accumTexture.texture,
            {0, 0, (float)pe->screenWidth, (float)-pe->screenHeight},
            {0, 0}, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Copy result back to accumTexture for waveform drawing
    BeginTextureMode(pe->accumTexture);
        DrawTextureRec(pe->tempTexture.texture,
            {0, 0, (float)pe->screenWidth, (float)-pe->screenHeight},
            {0, 0}, WHITE);

    // Leave accumTexture open for caller to draw new content
}

void PostEffectEndAccum(PostEffect* pe)
{
    (void)pe;  // API consistency - may use in future
    EndTextureMode();
}

void PostEffectToScreen(PostEffect* pe)
{
    float chromaticOffset = pe->currentBeatIntensity * pe->effects.chromaticMaxOffset;

    if (pe->effects.chromaticMaxOffset == 0 || chromaticOffset < 0.01f) {
        DrawTextureRec(pe->accumTexture.texture,
            {0, 0, (float)pe->screenWidth, (float)-pe->screenHeight},
            {0, 0}, WHITE);
        return;
    }

    BeginShaderMode(pe->chromaticShader);
        SetShaderValue(pe->chromaticShader, pe->chromaticOffsetLoc,
                       &chromaticOffset, SHADER_UNIFORM_FLOAT);
        DrawTextureRec(pe->accumTexture.texture,
            {0, 0, (float)pe->screenWidth, (float)-pe->screenHeight},
            {0, 0}, WHITE);
    EndShaderMode();
}
