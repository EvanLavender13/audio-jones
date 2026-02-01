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

void SetupDiscoBall(PostEffect *pe) {
  const DiscoBallConfig *db = &pe->effects.discoBall;

  // Accumulate rotation angle
  pe->discoBallAngle += db->rotationSpeed * pe->currentDeltaTime;

  SetShaderValue(pe->discoBallShader, pe->discoBallSphereRadiusLoc,
                 &db->sphereRadius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->discoBallShader, pe->discoBallTileSizeLoc, &db->tileSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->discoBallShader, pe->discoBallSphereAngleLoc,
                 &pe->discoBallAngle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->discoBallShader, pe->discoBallBumpHeightLoc,
                 &db->bumpHeight, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->discoBallShader, pe->discoBallReflectIntensityLoc,
                 &db->reflectIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->discoBallShader, pe->discoBallSpotIntensityLoc,
                 &db->spotIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->discoBallShader, pe->discoBallSpotFalloffLoc,
                 &db->spotFalloff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->discoBallShader, pe->discoBallBrightnessThresholdLoc,
                 &db->brightnessThreshold, SHADER_UNIFORM_FLOAT);
}

void SetupLegoBricks(PostEffect *pe) {
  const LegoBricksConfig *cfg = &pe->effects.legoBricks;
  SetShaderValue(pe->legoBricksShader, pe->legoBricksBrickScaleLoc,
                 &cfg->brickScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksStudHeightLoc,
                 &cfg->studHeight, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksEdgeShadowLoc,
                 &cfg->edgeShadow, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksColorThresholdLoc,
                 &cfg->colorThreshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksMaxBrickSizeLoc,
                 &cfg->maxBrickSize, SHADER_UNIFORM_INT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksLightAngleLoc,
                 &cfg->lightAngle, SHADER_UNIFORM_FLOAT);
}
