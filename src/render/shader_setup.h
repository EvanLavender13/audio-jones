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
void ApplyOilPaintStrokePass(PostEffect* pe, RenderTexture2D* source);
void SetupWatercolor(PostEffect* pe);
void SetupNeonGlow(PostEffect* pe);
void SetupRadialPulse(PostEffect* pe);
void SetupFalseColor(PostEffect* pe);
void SetupHalftone(PostEffect* pe);
void SetupChladniWarp(PostEffect* pe);
void SetupCrossHatching(PostEffect* pe);
void SetupPaletteQuantization(PostEffect* pe);
void SetupBokeh(PostEffect* pe);
void SetupBloom(PostEffect* pe);
void SetupMandelbox(PostEffect* pe);
void SetupTriangleFold(PostEffect* pe);
void SetupDomainWarp(PostEffect* pe);
void SetupPhyllotaxis(PostEffect* pe);
void SetupDensityWaveSpiral(PostEffect* pe);
void SetupMoireInterference(PostEffect* pe);
void SetupPencilSketch(PostEffect* pe);
void SetupMatrixRain(PostEffect* pe);
void SetupImpressionist(PostEffect* pe);
void SetupKuwahara(PostEffect* pe);
void SetupInkWash(PostEffect* pe);
void ApplyBloomPasses(PostEffect* pe, RenderTexture2D* source, int* writeIdx);
void ApplyHalfResEffect(PostEffect* pe, RenderTexture2D* source, int* writeIdx, Shader shader, RenderPipelineShaderSetupFn setup);
void ApplyHalfResOilPaint(PostEffect* pe, RenderTexture2D* source, int* writeIdx);
void SetupChromatic(PostEffect* pe);
void SetupGamma(PostEffect* pe);
void SetupClarity(PostEffect* pe);

// Returns shader, setup callback, and enabled flag for a transform effect type
TransformEffectEntry GetTransformEffect(PostEffect* pe, TransformEffectType type);

#endif // SHADER_SETUP_H
