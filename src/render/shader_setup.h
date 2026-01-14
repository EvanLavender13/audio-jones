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
void SetupCurlAdvectionTrailBoost(PostEffect* pe);
void SetupAttractorFlowTrailBoost(PostEffect* pe);
void SetupBoidsTrailBoost(PostEffect* pe);
void SetupCymaticsTrailBoost(PostEffect* pe);
void SetupKaleido(PostEffect* pe);
void SetupKifs(PostEffect* pe);
void SetupLatticeFold(PostEffect* pe);
void SetupSineWarp(PostEffect* pe);
void SetupInfiniteZoom(PostEffect* pe);
void SetupRadialStreak(PostEffect* pe);
void SetupTextureWarp(PostEffect* pe);
void SetupWaveRipple(PostEffect* pe);
void SetupMobius(PostEffect* pe);
void SetupPixelation(PostEffect* pe);
void SetupGlitch(PostEffect* pe);
void SetupPoincareDisk(PostEffect* pe);
void SetupToon(PostEffect* pe);
void SetupHeightfieldRelief(PostEffect* pe);
void SetupGradientFlow(PostEffect* pe);
void SetupDrosteZoom(PostEffect* pe);
void SetupColorGrade(PostEffect* pe);
void SetupAsciiArt(PostEffect* pe);
void SetupOilPaint(PostEffect* pe);
void SetupWatercolor(PostEffect* pe);
void SetupNeonGlow(PostEffect* pe);
void SetupRadialPulse(PostEffect* pe);
void SetupDuotone(PostEffect* pe);
void SetupHalftone(PostEffect* pe);
void SetupChladniWarp(PostEffect* pe);
void SetupChromatic(PostEffect* pe);
void SetupGamma(PostEffect* pe);
void SetupClarity(PostEffect* pe);

// Returns shader, setup callback, and enabled flag for a transform effect type
TransformEffectEntry GetTransformEffect(PostEffect* pe, TransformEffectType type);

#endif // SHADER_SETUP_H
