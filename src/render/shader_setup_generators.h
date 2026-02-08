#ifndef SHADER_SETUP_GENERATORS_H
#define SHADER_SETUP_GENERATORS_H

#include "post_effect.h"
#include "shader_setup.h"

void SetupConstellation(PostEffect *pe);
void SetupPlasma(PostEffect *pe);
void SetupInterference(PostEffect *pe);
void SetupSolidColor(PostEffect *pe);
void SetupScanBars(PostEffect *pe);
void SetupPitchSpiral(PostEffect *pe);
void SetupMoireGenerator(PostEffect *pe);
void SetupSpectralArcs(PostEffect *pe);
void SetupMuons(PostEffect *pe);
void SetupFilaments(PostEffect *pe);
void SetupSlashes(PostEffect *pe);
void SetupGlyphField(PostEffect *pe);
void SetupArcStrobe(PostEffect *pe);

void SetupConstellationBlend(PostEffect *pe);
void SetupPlasmaBlend(PostEffect *pe);
void SetupInterferenceBlend(PostEffect *pe);
void SetupSolidColorBlend(PostEffect *pe);
void SetupScanBarsBlend(PostEffect *pe);
void SetupPitchSpiralBlend(PostEffect *pe);
void SetupMoireGeneratorBlend(PostEffect *pe);
void SetupSpectralArcsBlend(PostEffect *pe);
void SetupMuonsBlend(PostEffect *pe);
void SetupFilamentsBlend(PostEffect *pe);
void SetupSlashesBlend(PostEffect *pe);
void SetupGlyphFieldBlend(PostEffect *pe);
void SetupArcStrobeBlend(PostEffect *pe);

// Resolves the generator shader and setup function for a given blend effect
// type. Returns {shader, setup} for use with RenderPass. Caller renders to
// pe->generatorScratch, then composites via the BlendCompositor entry.
typedef struct GeneratorPassInfo {
  Shader shader;
  RenderPipelineShaderSetupFn setup;
} GeneratorPassInfo;

GeneratorPassInfo GetGeneratorScratchPass(PostEffect *pe,
                                          TransformEffectType type);

#endif // SHADER_SETUP_GENERATORS_H
