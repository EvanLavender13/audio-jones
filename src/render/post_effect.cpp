#include "post_effect.h"
#include <cmath>
#include <stdlib.h>

Visualizer* VisualizerInit(int screenWidth, int screenHeight)
{
    Visualizer* vis = (Visualizer*)calloc(1, sizeof(Visualizer));
    if (vis == NULL) {
        return NULL;
    }

    vis->screenWidth = screenWidth;
    vis->screenHeight = screenHeight;
    vis->effects = EffectsConfig{};

    vis->blurHShader = LoadShader(0, "shaders/blur_h.fs");
    vis->blurVShader = LoadShader(0, "shaders/blur_v.fs");
    vis->chromaticShader = LoadShader(0, "shaders/chromatic.fs");

    if (vis->blurHShader.id == 0 || vis->blurVShader.id == 0 || vis->chromaticShader.id == 0) {
        TraceLog(LOG_ERROR, "VISUALIZER: Failed to load shaders");
        free(vis);
        return NULL;
    }

    vis->blurHResolutionLoc = GetShaderLocation(vis->blurHShader, "resolution");
    vis->blurVResolutionLoc = GetShaderLocation(vis->blurVShader, "resolution");
    vis->blurHScaleLoc = GetShaderLocation(vis->blurHShader, "blurScale");
    vis->blurVScaleLoc = GetShaderLocation(vis->blurVShader, "blurScale");
    vis->halfLifeLoc = GetShaderLocation(vis->blurVShader, "halfLife");
    vis->deltaTimeLoc = GetShaderLocation(vis->blurVShader, "deltaTime");
    vis->chromaticResolutionLoc = GetShaderLocation(vis->chromaticShader, "resolution");
    vis->chromaticOffsetLoc = GetShaderLocation(vis->chromaticShader, "chromaticOffset");
    vis->currentBeatIntensity = 0.0f;

    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(vis->blurHShader, vis->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(vis->blurVShader, vis->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(vis->chromaticShader, vis->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    // Create render textures for ping-pong blur
    vis->accumTexture = LoadRenderTexture(screenWidth, screenHeight);
    vis->tempTexture = LoadRenderTexture(screenWidth, screenHeight);

    if (vis->accumTexture.id == 0 || vis->tempTexture.id == 0) {
        TraceLog(LOG_ERROR, "VISUALIZER: Failed to create render textures");
        UnloadShader(vis->blurHShader);
        UnloadShader(vis->blurVShader);
        UnloadShader(vis->chromaticShader);
        free(vis);
        return NULL;
    }
    SetTextureWrap(vis->accumTexture.texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(vis->tempTexture.texture, TEXTURE_WRAP_CLAMP);

    // Clear both textures
    BeginTextureMode(vis->accumTexture);
    ClearBackground(BLACK);
    EndTextureMode();

    BeginTextureMode(vis->tempTexture);
    ClearBackground(BLACK);
    EndTextureMode();

    return vis;
}

void VisualizerUninit(Visualizer* vis)
{
    if (vis == NULL) {
        return;
    }

    UnloadRenderTexture(vis->accumTexture);
    UnloadRenderTexture(vis->tempTexture);
    UnloadShader(vis->blurHShader);
    UnloadShader(vis->blurVShader);
    UnloadShader(vis->chromaticShader);
    free(vis);
}

void VisualizerResize(Visualizer* vis, int width, int height)
{
    if (vis == NULL || (width == vis->screenWidth && height == vis->screenHeight)) {
        return;
    }

    vis->screenWidth = width;
    vis->screenHeight = height;

    // Recreate render textures at new size
    UnloadRenderTexture(vis->accumTexture);
    UnloadRenderTexture(vis->tempTexture);
    vis->accumTexture = LoadRenderTexture(width, height);
    vis->tempTexture = LoadRenderTexture(width, height);
    SetTextureWrap(vis->accumTexture.texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(vis->tempTexture.texture, TEXTURE_WRAP_CLAMP);

    // Clear both textures
    BeginTextureMode(vis->accumTexture);
    ClearBackground(BLACK);
    EndTextureMode();

    BeginTextureMode(vis->tempTexture);
    ClearBackground(BLACK);
    EndTextureMode();

    float resolution[2] = { (float)width, (float)height };
    SetShaderValue(vis->blurHShader, vis->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(vis->blurVShader, vis->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(vis->chromaticShader, vis->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

void VisualizerBeginAccum(Visualizer* vis, float deltaTime, float beatIntensity)
{
    vis->currentBeatIntensity = beatIntensity;

    int blurScale = vis->effects.baseBlurScale + lroundf(beatIntensity * vis->effects.beatBlurScale);

    // Horizontal blur (accumTexture -> tempTexture)
    BeginTextureMode(vis->tempTexture);
    BeginShaderMode(vis->blurHShader);
        SetShaderValue(vis->blurHShader, vis->blurHScaleLoc, &blurScale, SHADER_UNIFORM_INT);
        DrawTextureRec(vis->accumTexture.texture,
            {0, 0, (float)vis->screenWidth, (float)-vis->screenHeight},
            {0, 0}, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Vertical blur + decay (tempTexture -> accumTexture)
    BeginTextureMode(vis->accumTexture);
    BeginShaderMode(vis->blurVShader);
        SetShaderValue(vis->blurVShader, vis->blurVScaleLoc, &blurScale, SHADER_UNIFORM_INT);
        SetShaderValue(vis->blurVShader, vis->halfLifeLoc, &vis->effects.halfLife, SHADER_UNIFORM_FLOAT);
        SetShaderValue(vis->blurVShader, vis->deltaTimeLoc, &deltaTime, SHADER_UNIFORM_FLOAT);
        DrawTextureRec(vis->tempTexture.texture,
            {0, 0, (float)vis->screenWidth, (float)-vis->screenHeight},
            {0, 0}, WHITE);
    EndShaderMode();

    // Leave accumTexture open for caller to draw new content
}

void VisualizerEndAccum(Visualizer* vis)
{
    (void)vis;  // API consistency - may use in future
    EndTextureMode();
}

void VisualizerToScreen(Visualizer* vis)
{
    float chromaticOffset = vis->currentBeatIntensity * vis->effects.chromaticMaxOffset;

    if (vis->effects.chromaticMaxOffset == 0 || chromaticOffset < 0.01f) {
        DrawTextureRec(vis->accumTexture.texture,
            {0, 0, (float)vis->screenWidth, (float)-vis->screenHeight},
            {0, 0}, WHITE);
        return;
    }

    BeginShaderMode(vis->chromaticShader);
        SetShaderValue(vis->chromaticShader, vis->chromaticOffsetLoc,
                       &chromaticOffset, SHADER_UNIFORM_FLOAT);
        DrawTextureRec(vis->accumTexture.texture,
            {0, 0, (float)vis->screenWidth, (float)-vis->screenHeight},
            {0, 0}, WHITE);
    EndShaderMode();
}
