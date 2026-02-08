#include "shader_setup_generators.h"
#include "blend_compositor.h"
#include "effects/filaments.h"
#include "effects/glyph_field.h"
#include "effects/moire_generator.h"
#include "effects/muons.h"
#include "effects/pitch_spiral.h"
#include "effects/slashes.h"
#include "effects/spectral_arcs.h"
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

void SetupScanBars(PostEffect *pe) {
  ScanBarsEffectSetup(&pe->scanBars, &pe->effects.scanBars,
                      pe->currentDeltaTime);
}

void SetupPitchSpiral(PostEffect *pe) {
  PitchSpiralEffectSetup(&pe->pitchSpiral, &pe->effects.pitchSpiral,
                         pe->currentDeltaTime, pe->fftTexture);
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

void SetupScanBarsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.scanBars.blendIntensity,
                       pe->effects.scanBars.blendMode);
}

void SetupPitchSpiralBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.pitchSpiral.blendIntensity,
                       pe->effects.pitchSpiral.blendMode);
}

void SetupMoireGenerator(PostEffect *pe) {
  MoireGeneratorEffectSetup(&pe->moireGenerator, &pe->effects.moireGenerator,
                            pe->currentDeltaTime);
}

void SetupMoireGeneratorBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.moireGenerator.blendIntensity,
                       pe->effects.moireGenerator.blendMode);
}

void SetupSpectralArcs(PostEffect *pe) {
  SpectralArcsEffectSetup(&pe->spectralArcs, &pe->effects.spectralArcs,
                          pe->currentDeltaTime, pe->fftTexture);
}

void SetupSpectralArcsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.spectralArcs.blendIntensity,
                       pe->effects.spectralArcs.blendMode);
}

void SetupMuons(PostEffect *pe) {
  MuonsEffectSetup(&pe->muons, &pe->effects.muons, pe->currentDeltaTime);
}

void SetupMuonsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.muons.blendIntensity,
                       pe->effects.muons.blendMode);
}

void SetupFilaments(PostEffect *pe) {
  FilamentsEffectSetup(&pe->filaments, &pe->effects.filaments,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupFilamentsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.filaments.blendIntensity,
                       pe->effects.filaments.blendMode);
}

void SetupSlashes(PostEffect *pe) {
  SlashesEffectSetup(&pe->slashes, &pe->effects.slashes, pe->currentDeltaTime,
                     pe->fftTexture);
}

void SetupSlashesBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.slashes.blendIntensity,
                       pe->effects.slashes.blendMode);
}

void SetupGlyphField(PostEffect *pe) {
  GlyphFieldEffectSetup(&pe->glyphField, &pe->effects.glyphField,
                        pe->currentDeltaTime);
}

void SetupGlyphFieldBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.glyphField.blendIntensity,
                       pe->effects.glyphField.blendMode);
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
  case TRANSFORM_SCAN_BARS_BLEND:
    return {pe->scanBars.shader, SetupScanBars};
  case TRANSFORM_PITCH_SPIRAL_BLEND:
    return {pe->pitchSpiral.shader, SetupPitchSpiral};
  case TRANSFORM_MOIRE_GENERATOR_BLEND:
    return {pe->moireGenerator.shader, SetupMoireGenerator};
  case TRANSFORM_SPECTRAL_ARCS_BLEND:
    return {pe->spectralArcs.shader, SetupSpectralArcs};
  case TRANSFORM_MUONS_BLEND:
    return {pe->muons.shader, SetupMuons};
  case TRANSFORM_FILAMENTS_BLEND:
    return {pe->filaments.shader, SetupFilaments};
  case TRANSFORM_SLASHES_BLEND:
    return {pe->slashes.shader, SetupSlashes};
  case TRANSFORM_GLYPH_FIELD_BLEND:
    return {pe->glyphField.shader, SetupGlyphField};
  default:
    return {{0}, NULL};
  }
}
