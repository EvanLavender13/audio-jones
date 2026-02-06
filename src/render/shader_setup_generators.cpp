#include "shader_setup_generators.h"
#include "blend_compositor.h"
#include "post_effect.h"

void SetupConstellation(PostEffect *pe) {
  ConstellationEffectSetup(&pe->constellation, &pe->effects.constellation,
                           pe->currentDeltaTime);
}

void SetupPlasma(PostEffect *pe) {
  PlasmaEffectSetup(&pe->plasma, &pe->effects.plasma, pe->currentDeltaTime);
}

void SetupInterference(PostEffect *pe) {
  InterferenceEffectSetup(&pe->interference, &pe->effects.interference,
                          pe->currentDeltaTime);
}

void SetupSolidColor(PostEffect *pe) {
  SolidColorEffectSetup(&pe->solidColor, &pe->effects.solidColor);
}

void SetupConstellationBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.constellation.blendIntensity,
                       pe->effects.constellation.blendMode);
}

void SetupPlasmaBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.plasma.blendIntensity,
                       pe->effects.plasma.blendMode);
}

void SetupInterferenceBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.interference.blendIntensity,
                       pe->effects.interference.blendMode);
}

void SetupSolidColorBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.solidColor.blendIntensity,
                       pe->effects.solidColor.blendMode);
}

GeneratorPassInfo GetGeneratorScratchPass(PostEffect *pe,
                                          TransformEffectType type) {
  switch (type) {
  case TRANSFORM_CONSTELLATION_BLEND:
    return {pe->constellation.shader, SetupConstellation};
  case TRANSFORM_PLASMA_BLEND:
    return {pe->plasma.shader, SetupPlasma};
  case TRANSFORM_INTERFERENCE_BLEND:
    return {pe->interference.shader, SetupInterference};
  case TRANSFORM_SOLID_COLOR:
    return {pe->solidColor.shader, SetupSolidColor};
  default:
    return {{0}, NULL};
  }
}
