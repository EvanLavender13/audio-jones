#ifndef SHADER_SETUP_H
#define SHADER_SETUP_H

#include <stdbool.h>
#include "raylib.h"
#include "config/effect_config.h"
#include "shader_setup_symmetry.h"
#include "shader_setup_warp.h"
#include "shader_setup_cellular.h"
#include "shader_setup_motion.h"
#include "shader_setup_style.h"
#include "shader_setup_color.h"

typedef struct PostEffect PostEffect;

// Callback for shader uniform binding
typedef void (*RenderPipelineShaderSetupFn)(PostEffect* pe);

// Entry for transform effect dispatch table
struct TransformEffectEntry {
    Shader* shader;
    RenderPipelineShaderSetupFn setup;
    bool* enabled;
};

// Core shader setup functions (feedback, blur, trail boost, utility)
void SetupFeedback(PostEffect* pe);
void SetupBlurH(PostEffect* pe);
void SetupBlurV(PostEffect* pe);
void SetupTrailBoost(PostEffect* pe);
void SetupCurlFlowTrailBoost(PostEffect* pe);
void SetupCurlAdvectionTrailBoost(PostEffect* pe);
void SetupAttractorFlowTrailBoost(PostEffect* pe);
void SetupParticleLifeTrailBoost(PostEffect* pe);
void SetupBoidsTrailBoost(PostEffect* pe);
void SetupCymaticsTrailBoost(PostEffect* pe);
void SetupChromatic(PostEffect* pe);
void SetupGamma(PostEffect* pe);
void SetupClarity(PostEffect* pe);

// Multi-pass and utility functions
void ApplyBloomPasses(PostEffect* pe, RenderTexture2D* source, int* writeIdx);
void ApplyHalfResEffect(PostEffect* pe, RenderTexture2D* source, int* writeIdx, Shader shader, RenderPipelineShaderSetupFn setup);
void ApplyHalfResOilPaint(PostEffect* pe, RenderTexture2D* source, int* writeIdx);
void ApplyOilPaintStrokePass(PostEffect* pe, RenderTexture2D* source);

// Returns shader, setup callback, and enabled flag for a transform effect type
TransformEffectEntry GetTransformEffect(PostEffect* pe, TransformEffectType type);

#endif // SHADER_SETUP_H
