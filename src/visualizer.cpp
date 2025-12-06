#include "visualizer.h"
#include <stdlib.h>

Visualizer* VisualizerInit(int screenWidth, int screenHeight)
{
    Visualizer* vis = (Visualizer*)malloc(sizeof(Visualizer));
    if (vis == NULL) {
        return NULL;
    }

    vis->screenWidth = screenWidth;
    vis->screenHeight = screenHeight;
    vis->effects = (EffectsConfig)EFFECTS_CONFIG_DEFAULT;

    // Load separable blur shaders
    vis->blurHShader = LoadShader(0, "shaders/blur_h.fs");
    vis->blurVShader = LoadShader(0, "shaders/blur_v.fs");

    // Get uniform locations
    vis->blurHResolutionLoc = GetShaderLocation(vis->blurHShader, "resolution");
    vis->blurVResolutionLoc = GetShaderLocation(vis->blurVShader, "resolution");
    vis->blurHScaleLoc = GetShaderLocation(vis->blurHShader, "blurScale");
    vis->blurVScaleLoc = GetShaderLocation(vis->blurVShader, "blurScale");
    vis->halfLifeLoc = GetShaderLocation(vis->blurVShader, "halfLife");
    vis->deltaTimeLoc = GetShaderLocation(vis->blurVShader, "deltaTime");

    // Set resolution uniform (static, only needs to be set once)
    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(vis->blurHShader, vis->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(vis->blurVShader, vis->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    // Create render textures for ping-pong blur
    vis->accumTexture = LoadRenderTexture(screenWidth, screenHeight);
    vis->tempTexture = LoadRenderTexture(screenWidth, screenHeight);

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

    // Clear both textures
    BeginTextureMode(vis->accumTexture);
    ClearBackground(BLACK);
    EndTextureMode();

    BeginTextureMode(vis->tempTexture);
    ClearBackground(BLACK);
    EndTextureMode();

    // Update shader resolution uniforms
    float resolution[2] = { (float)width, (float)height };
    SetShaderValue(vis->blurHShader, vis->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(vis->blurVShader, vis->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

void VisualizerBeginAccum(Visualizer* vis, float deltaTime, float beatIntensity)
{
    // Blur scale: base + (beat scale * intensity) for bloom pulse effect
    // Round to int for whole-pixel sampling (avoids interpolation artifacts)
    int blurScale = vis->effects.baseBlurScale + (int)(beatIntensity * vis->effects.beatBlurScale + 0.5f);

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
    DrawTextureRec(vis->accumTexture.texture,
        {0, 0, (float)vis->screenWidth, (float)-vis->screenHeight},
        {0, 0}, WHITE);
}
