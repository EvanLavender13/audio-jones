#include "visualizer.h"
#include <stdlib.h>

struct Visualizer {
    RenderTexture2D accumTexture;
    Shader fadeShader;
    int halfLifeLoc;
    int deltaTimeLoc;
    float halfLife;  // Trail persistence in seconds
    int screenWidth;
    int screenHeight;
};

Visualizer* VisualizerInit(int screenWidth, int screenHeight)
{
    Visualizer* vis = (Visualizer*)malloc(sizeof(Visualizer));
    if (vis == NULL) return NULL;

    vis->screenWidth = screenWidth;
    vis->screenHeight = screenHeight;
    vis->halfLife = 0.5f;  // 0.5 seconds trail persistence

    // Load fade shader
    vis->fadeShader = LoadShader(0, "shaders/fade.fs");
    vis->halfLifeLoc = GetShaderLocation(vis->fadeShader, "halfLife");
    vis->deltaTimeLoc = GetShaderLocation(vis->fadeShader, "deltaTime");

    // Create render texture for accumulation
    vis->accumTexture = LoadRenderTexture(screenWidth, screenHeight);

    // Clear accumulation texture
    BeginTextureMode(vis->accumTexture);
    ClearBackground(BLACK);
    EndTextureMode();

    return vis;
}

void VisualizerUninit(Visualizer* vis)
{
    if (vis == NULL) return;

    UnloadRenderTexture(vis->accumTexture);
    UnloadShader(vis->fadeShader);
    free(vis);
}

void VisualizerBeginAccum(Visualizer* vis, float deltaTime)
{
    BeginTextureMode(vis->accumTexture);

    // Apply fade to existing content
    BeginShaderMode(vis->fadeShader);
        SetShaderValue(vis->fadeShader, vis->halfLifeLoc, &vis->halfLife, SHADER_UNIFORM_FLOAT);
        SetShaderValue(vis->fadeShader, vis->deltaTimeLoc, &deltaTime, SHADER_UNIFORM_FLOAT);
        // Draw the texture onto itself with fade
        DrawTextureRec(vis->accumTexture.texture,
            (Rectangle){0, 0, (float)vis->screenWidth, (float)-vis->screenHeight},
            (Vector2){0, 0}, WHITE);
    EndShaderMode();
}

void VisualizerEndAccum(Visualizer* vis)
{
    EndTextureMode();
}

void VisualizerToScreen(Visualizer* vis)
{
    DrawTextureRec(vis->accumTexture.texture,
        (Rectangle){0, 0, (float)vis->screenWidth, (float)-vis->screenHeight},
        (Vector2){0, 0}, WHITE);
}
