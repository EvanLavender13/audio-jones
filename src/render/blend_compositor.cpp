#include "blend_compositor.h"
#include <stdlib.h>

BlendCompositor* BlendCompositorInit(void)
{
    BlendCompositor* bc = (BlendCompositor*)calloc(1, sizeof(BlendCompositor));
    if (bc == NULL) {
        return NULL;
    }

    bc->shader = LoadShader(0, "shaders/effect_blend.fs");
    if (bc->shader.id == 0) {
        TraceLog(LOG_ERROR, "BLEND_COMPOSITOR: Failed to load effect_blend.fs");
        free(bc);
        return NULL;
    }

    bc->effectMapLoc = GetShaderLocation(bc->shader, "effectMap");
    bc->intensityLoc = GetShaderLocation(bc->shader, "intensity");
    bc->blendModeLoc = GetShaderLocation(bc->shader, "blendMode");

    TraceLog(LOG_INFO, "BLEND_COMPOSITOR: Initialized");
    return bc;
}

void BlendCompositorUninit(BlendCompositor* bc)
{
    if (bc == NULL) {
        return;
    }

    UnloadShader(bc->shader);
    free(bc);
}

void BlendCompositorApply(const BlendCompositor* bc, Texture2D effectTexture,
                          float intensity, EffectBlendMode mode)
{
    const int blendModeInt = (int)mode;
    SetShaderValueTexture(bc->shader, bc->effectMapLoc, effectTexture);
    SetShaderValue(bc->shader, bc->intensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bc->shader, bc->blendModeLoc, &blendModeInt, SHADER_UNIFORM_INT);
}
