#ifndef SHADER_SETUP_H
#define SHADER_SETUP_H

#include <stdbool.h>
#include "raylib.h"
#include "config/effect_config.h"

typedef struct PostEffect PostEffect;

// Callback for shader uniform binding
typedef void (*RenderPipelineShaderSetupFn)(PostEffect* pe);

// Entry for transform effect dispatch table
struct TransformEffectEntry {
    Shader* shader;
    RenderPipelineShaderSetupFn setup;
    bool* enabled;
};

// Shader setup functions for each effect pass
void SetupVoronoi(PostEffect* pe);
void SetupFeedback(PostEffect* pe);
void SetupBlurH(PostEffect* pe);
void SetupBlurV(PostEffect* pe);
void SetupTrailBoost(PostEffect* pe);
void SetupCurlFlowTrailBoost(PostEffect* pe);
void SetupAttractorFlowTrailBoost(PostEffect* pe);
void SetupKaleido(PostEffect* pe);
void SetupSineWarp(PostEffect* pe);
void SetupInfiniteZoom(PostEffect* pe);
void SetupRadialStreak(PostEffect* pe);
void SetupTextureWarp(PostEffect* pe);
void SetupChromatic(PostEffect* pe);
void SetupGamma(PostEffect* pe);
void SetupClarity(PostEffect* pe);

// Returns shader, setup callback, and enabled flag for a transform effect type
TransformEffectEntry GetTransformEffect(PostEffect* pe, TransformEffectType type);

#endif // SHADER_SETUP_H
