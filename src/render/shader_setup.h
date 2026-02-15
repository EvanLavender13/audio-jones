#ifndef SHADER_SETUP_H
#define SHADER_SETUP_H

#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct PostEffect PostEffect;

// Entry for transform effect dispatch table
struct TransformEffectEntry {
  Shader *shader;
  RenderPipelineShaderSetupFn setup;
  bool *enabled;
};

// Core shader setup functions (feedback, blur, trail boost, utility)
void SetupFeedback(PostEffect *pe);
void SetupBlurH(PostEffect *pe);
void SetupBlurV(PostEffect *pe);
void SetupTrailBoost(PostEffect *pe);
void SetupCurlFlowTrailBoost(PostEffect *pe);
void SetupCurlAdvectionTrailBoost(PostEffect *pe);
void SetupAttractorFlowTrailBoost(PostEffect *pe);
void SetupParticleLifeTrailBoost(PostEffect *pe);
void SetupBoidsTrailBoost(PostEffect *pe);
void SetupCymaticsTrailBoost(PostEffect *pe);
void SetupChromatic(PostEffect *pe);
void SetupGamma(PostEffect *pe);
void SetupClarity(PostEffect *pe);

// Multi-pass and utility functions
void ApplyAnamorphicStreakPasses(PostEffect *pe, RenderTexture2D *source);
void ApplyBloomPasses(PostEffect *pe, RenderTexture2D *source, int *writeIdx);
void ApplyHalfResEffect(PostEffect *pe, RenderTexture2D *source,
                        const int *writeIdx, Shader shader,
                        RenderPipelineShaderSetupFn setup);
void ApplyHalfResOilPaint(PostEffect *pe, RenderTexture2D *source,
                          const int *writeIdx);

// Returns shader, setup callback, and enabled flag for a transform effect type
TransformEffectEntry GetTransformEffect(PostEffect *pe,
                                        TransformEffectType type);

// Resolves the generator shader and setup function for a given blend effect
// type. Returns {shader, setup} for use with RenderPass.
typedef struct GeneratorPassInfo {
  Shader shader;
  RenderPipelineShaderSetupFn setup;
} GeneratorPassInfo;

GeneratorPassInfo GetGeneratorScratchPass(PostEffect *pe,
                                          TransformEffectType type);

#endif // SHADER_SETUP_H
