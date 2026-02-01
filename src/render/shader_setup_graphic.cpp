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
