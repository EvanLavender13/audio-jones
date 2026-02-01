#include "shader_setup_graphic.h"
#include "post_effect.h"

void SetupToon(PostEffect *pe) {
  const ToonConfig *t = &pe->effects.toon;
  SetShaderValue(pe->toonShader, pe->toonLevelsLoc, &t->levels,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->toonShader, pe->toonEdgeThresholdLoc, &t->edgeThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->toonShader, pe->toonEdgeSoftnessLoc, &t->edgeSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->toonShader, pe->toonThicknessVariationLoc,
                 &t->thicknessVariation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->toonShader, pe->toonNoiseScaleLoc, &t->noiseScale,
                 SHADER_UNIFORM_FLOAT);
}

void SetupNeonGlow(PostEffect *pe) {
  const NeonGlowConfig *ng = &pe->effects.neonGlow;
  float glowColor[3] = {ng->glowR, ng->glowG, ng->glowB};
  SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowColorLoc, glowColor,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowEdgeThresholdLoc,
                 &ng->edgeThreshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowEdgePowerLoc, &ng->edgePower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowIntensityLoc,
                 &ng->glowIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowRadiusLoc, &ng->glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowSamplesLoc,
                 &ng->glowSamples, SHADER_UNIFORM_INT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowOriginalVisibilityLoc,
                 &ng->originalVisibility, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowColorModeLoc, &ng->colorMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowSaturationBoostLoc,
                 &ng->saturationBoost, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowBrightnessBoostLoc,
                 &ng->brightnessBoost, SHADER_UNIFORM_FLOAT);
}

void SetupKuwahara(PostEffect *pe) {
  const KuwaharaConfig *k = &pe->effects.kuwahara;
  int radius = (int)k->radius;
  SetShaderValue(pe->kuwaharaShader, pe->kuwaharaRadiusLoc, &radius,
                 SHADER_UNIFORM_INT);
}

void SetupHalftone(PostEffect *pe) {
  const HalftoneConfig *ht = &pe->effects.halftone;
  float rotation = pe->currentHalftoneRotation + ht->rotationAngle;

  SetShaderValue(pe->halftoneShader, pe->halftoneDotScaleLoc, &ht->dotScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->halftoneShader, pe->halftoneDotSizeLoc, &ht->dotSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->halftoneShader, pe->halftoneRotationLoc, &rotation,
                 SHADER_UNIFORM_FLOAT);
}

void SetupSynthwave(PostEffect *pe) {
  const SynthwaveConfig *sw = &pe->effects.synthwave;

  SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonYLoc, &sw->horizonY,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveColorMixLoc, &sw->colorMix,
                 SHADER_UNIFORM_FLOAT);

  float palettePhase[3] = {sw->palettePhaseR, sw->palettePhaseG,
                           sw->palettePhaseB};
  SetShaderValue(pe->synthwaveShader, pe->synthwavePalettePhaseLoc,
                 palettePhase, SHADER_UNIFORM_VEC3);

  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridSpacingLoc,
                 &sw->gridSpacing, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridThicknessLoc,
                 &sw->gridThickness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridOpacityLoc,
                 &sw->gridOpacity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridGlowLoc, &sw->gridGlow,
                 SHADER_UNIFORM_FLOAT);

  float gridColor[3] = {sw->gridR, sw->gridG, sw->gridB};
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridColorLoc, gridColor,
                 SHADER_UNIFORM_VEC3);

  SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeCountLoc,
                 &sw->stripeCount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeSoftnessLoc,
                 &sw->stripeSoftness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeIntensityLoc,
                 &sw->stripeIntensity, SHADER_UNIFORM_FLOAT);

  float sunColor[3] = {sw->sunR, sw->sunG, sw->sunB};
  SetShaderValue(pe->synthwaveShader, pe->synthwaveSunColorLoc, sunColor,
                 SHADER_UNIFORM_VEC3);

  SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonIntensityLoc,
                 &sw->horizonIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonFalloffLoc,
                 &sw->horizonFalloff, SHADER_UNIFORM_FLOAT);

  float horizonColor[3] = {sw->horizonR, sw->horizonG, sw->horizonB};
  SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonColorLoc,
                 horizonColor, SHADER_UNIFORM_VEC3);

  // Animation (times accumulated with speed in render_pipeline.cpp)
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridTimeLoc,
                 &pe->synthwaveGridTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeTimeLoc,
                 &pe->synthwaveStripeTime, SHADER_UNIFORM_FLOAT);
}
