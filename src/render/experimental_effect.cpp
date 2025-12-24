#include "experimental_effect.h"
#include "render_utils.h"
#include <stdlib.h>

static const char* LOG_PREFIX = "EXPERIMENTAL_EFFECT";

static bool LoadExperimentalShaders(ExperimentalEffect* exp)
{
    exp->feedbackExpShader = LoadShader(0, "shaders/experimental/feedback_exp.fs");
    exp->blendInjectShader = LoadShader(0, "shaders/experimental/blend_inject.fs");

    if (exp->feedbackExpShader.id == 0) {
        TraceLog(LOG_WARNING, "EXPERIMENTAL_EFFECT: Failed to load feedback_exp.fs");
    }
    if (exp->blendInjectShader.id == 0) {
        TraceLog(LOG_WARNING, "EXPERIMENTAL_EFFECT: Failed to load blend_inject.fs");
    }

    return exp->feedbackExpShader.id != 0 && exp->blendInjectShader.id != 0;
}

static void GetShaderUniformLocations(ExperimentalEffect* exp)
{
    exp->feedbackResolutionLoc = GetShaderLocation(exp->feedbackExpShader, "resolution");
    exp->feedbackHalfLifeLoc = GetShaderLocation(exp->feedbackExpShader, "halfLife");
    exp->feedbackDeltaTimeLoc = GetShaderLocation(exp->feedbackExpShader, "deltaTime");
    exp->feedbackZoomFactorLoc = GetShaderLocation(exp->feedbackExpShader, "zoomFactor");
    exp->blendInjectionTexLoc = GetShaderLocation(exp->blendInjectShader, "injectionTex");
    exp->blendInjectionOpacityLoc = GetShaderLocation(exp->blendInjectShader, "injectionOpacity");
}

static void SetResolutionUniforms(ExperimentalEffect* exp, int width, int height)
{
    float resolution[2] = { (float)width, (float)height };
    SetShaderValue(exp->feedbackExpShader, exp->feedbackResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

ExperimentalEffect* ExperimentalEffectInit(int screenWidth, int screenHeight)
{
    ExperimentalEffect* exp = (ExperimentalEffect*)calloc(1, sizeof(ExperimentalEffect));
    if (exp == NULL) {
        return NULL;
    }

    exp->screenWidth = screenWidth;
    exp->screenHeight = screenHeight;
    exp->config = ExperimentalConfig{};

    if (!LoadExperimentalShaders(exp)) {
        TraceLog(LOG_ERROR, "EXPERIMENTAL_EFFECT: Failed to load shaders");
        free(exp);
        return NULL;
    }

    GetShaderUniformLocations(exp);
    SetResolutionUniforms(exp, screenWidth, screenHeight);

    RenderUtilsInitTextureHDR(&exp->expAccumTexture, screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&exp->expTempTexture, screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&exp->injectionTexture, screenWidth, screenHeight, LOG_PREFIX);

    if (exp->expAccumTexture.id == 0 || exp->expTempTexture.id == 0 ||
        exp->injectionTexture.id == 0) {
        TraceLog(LOG_ERROR, "EXPERIMENTAL_EFFECT: Failed to create render textures");
        if (exp->expAccumTexture.id != 0) {
            UnloadRenderTexture(exp->expAccumTexture);
        }
        if (exp->expTempTexture.id != 0) {
            UnloadRenderTexture(exp->expTempTexture);
        }
        if (exp->injectionTexture.id != 0) {
            UnloadRenderTexture(exp->injectionTexture);
        }
        UnloadShader(exp->feedbackExpShader);
        UnloadShader(exp->blendInjectShader);
        free(exp);
        return NULL;
    }

    TraceLog(LOG_INFO, "EXPERIMENTAL_EFFECT: Initialized (%dx%d)", screenWidth, screenHeight);
    return exp;
}

void ExperimentalEffectUninit(ExperimentalEffect* exp)
{
    if (exp == NULL) {
        return;
    }

    UnloadRenderTexture(exp->expAccumTexture);
    UnloadRenderTexture(exp->expTempTexture);
    UnloadRenderTexture(exp->injectionTexture);
    UnloadShader(exp->feedbackExpShader);
    UnloadShader(exp->blendInjectShader);
    free(exp);
}

void ExperimentalEffectResize(ExperimentalEffect* exp, int width, int height)
{
    if (exp == NULL || (width == exp->screenWidth && height == exp->screenHeight)) {
        return;
    }

    exp->screenWidth = width;
    exp->screenHeight = height;

    UnloadRenderTexture(exp->expAccumTexture);
    UnloadRenderTexture(exp->expTempTexture);
    UnloadRenderTexture(exp->injectionTexture);
    RenderUtilsInitTextureHDR(&exp->expAccumTexture, width, height, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&exp->expTempTexture, width, height, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&exp->injectionTexture, width, height, LOG_PREFIX);

    SetResolutionUniforms(exp, width, height);

    TraceLog(LOG_INFO, "EXPERIMENTAL_EFFECT: Resized to %dx%d", width, height);
}

void ExperimentalEffectBeginAccum(ExperimentalEffect* exp, float deltaTime)
{
    if (exp == NULL) {
        return;
    }

    BeginTextureMode(exp->expTempTexture);
    BeginShaderMode(exp->feedbackExpShader);
        SetShaderValue(exp->feedbackExpShader, exp->feedbackHalfLifeLoc,
                       &exp->config.halfLife, SHADER_UNIFORM_FLOAT);
        SetShaderValue(exp->feedbackExpShader, exp->feedbackDeltaTimeLoc,
                       &deltaTime, SHADER_UNIFORM_FLOAT);
        SetShaderValue(exp->feedbackExpShader, exp->feedbackZoomFactorLoc,
                       &exp->config.zoomFactor, SHADER_UNIFORM_FLOAT);
        RenderUtilsDrawFullscreenQuad(exp->expAccumTexture.texture, exp->screenWidth, exp->screenHeight);
    EndShaderMode();
    EndTextureMode();

    BeginTextureMode(exp->injectionTexture);
    ClearBackground(BLACK);
}

void ExperimentalEffectEndAccum(ExperimentalEffect* exp)
{
    if (exp == NULL) {
        return;
    }

    EndTextureMode();

    BeginTextureMode(exp->expAccumTexture);
    BeginShaderMode(exp->blendInjectShader);
        SetShaderValueTexture(exp->blendInjectShader, exp->blendInjectionTexLoc,
                              exp->injectionTexture.texture);
        SetShaderValue(exp->blendInjectShader, exp->blendInjectionOpacityLoc,
                       &exp->config.injectionOpacity, SHADER_UNIFORM_FLOAT);
        RenderUtilsDrawFullscreenQuad(exp->expTempTexture.texture, exp->screenWidth, exp->screenHeight);
    EndShaderMode();
    EndTextureMode();
}

void ExperimentalEffectToScreen(ExperimentalEffect* exp)
{
    if (exp == NULL) {
        return;
    }

    RenderUtilsDrawFullscreenQuad(exp->expAccumTexture.texture, exp->screenWidth, exp->screenHeight);
}

void ExperimentalEffectClear(ExperimentalEffect* exp)
{
    if (exp == NULL) {
        return;
    }

    RenderUtilsClearTexture(&exp->expAccumTexture);
    RenderUtilsClearTexture(&exp->expTempTexture);
    RenderUtilsClearTexture(&exp->injectionTexture);
}
