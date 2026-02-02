#include "shader_setup_generators.h"
#include "color_lut.h"
#include "post_effect.h"

void SetupConstellation(PostEffect *pe) {
  ConstellationConfig &cfg = pe->effects.constellation;

  // Update LUT textures from gradient configs
  ColorLUTUpdate(pe->constellationPointLUT, &cfg.pointGradient);
  ColorLUTUpdate(pe->constellationLineLUT, &cfg.lineGradient);

  // Phase uniforms (accumulated on CPU with speed multipliers)
  SetShaderValue(pe->constellationShader, pe->constellationAnimPhaseLoc,
                 &pe->constellationAnimPhase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationRadialPhaseLoc,
                 &pe->constellationRadialPhase, SHADER_UNIFORM_FLOAT);

  // Float uniforms
  SetShaderValue(pe->constellationShader, pe->constellationGridScaleLoc,
                 &cfg.gridScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationWanderAmpLoc,
                 &cfg.wanderAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationRadialFreqLoc,
                 &cfg.radialFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationRadialAmpLoc,
                 &cfg.radialAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationPointSizeLoc,
                 &cfg.pointSize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationPointBrightnessLoc,
                 &cfg.pointBrightness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationLineThicknessLoc,
                 &cfg.lineThickness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationMaxLineLenLoc,
                 &cfg.maxLineLen, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationLineOpacityLoc,
                 &cfg.lineOpacity, SHADER_UNIFORM_FLOAT);

  // Boolean as int uniform
  int interp = cfg.interpolateLineColor ? 1 : 0;
  SetShaderValue(pe->constellationShader,
                 pe->constellationInterpolateLineColorLoc, &interp,
                 SHADER_UNIFORM_INT);

  // Bind LUT textures
  SetShaderValueTexture(pe->constellationShader, pe->constellationPointLUTLoc,
                        ColorLUTGetTexture(pe->constellationPointLUT));
  SetShaderValueTexture(pe->constellationShader, pe->constellationLineLUTLoc,
                        ColorLUTGetTexture(pe->constellationLineLUT));
}

void SetupPlasma(PostEffect *pe) {
  PlasmaConfig &cfg = pe->effects.plasma;

  // Update LUT texture from gradient config
  ColorLUTUpdate(pe->plasmaGradientLUT, &cfg.gradient);

  // Int uniforms
  SetShaderValue(pe->plasmaShader, pe->plasmaBoltCountLoc, &cfg.boltCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->plasmaShader, pe->plasmaLayerCountLoc, &cfg.layerCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->plasmaShader, pe->plasmaOctavesLoc, &cfg.octaves,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->plasmaShader, pe->plasmaFalloffTypeLoc, &cfg.falloffType,
                 SHADER_UNIFORM_INT);

  // Float uniforms
  SetShaderValue(pe->plasmaShader, pe->plasmaDriftAmountLoc, &cfg.driftAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->plasmaShader, pe->plasmaDisplacementLoc, &cfg.displacement,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->plasmaShader, pe->plasmaGlowRadiusLoc, &cfg.glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->plasmaShader, pe->plasmaCoreBrightnessLoc,
                 &cfg.coreBrightness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->plasmaShader, pe->plasmaFlickerAmountLoc,
                 &cfg.flickerAmount, SHADER_UNIFORM_FLOAT);

  // Phase accumulators
  SetShaderValue(pe->plasmaShader, pe->plasmaAnimPhaseLoc, &pe->plasmaAnimPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->plasmaShader, pe->plasmaDriftPhaseLoc,
                 &pe->plasmaDriftPhase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->plasmaShader, pe->plasmaFlickerTimeLoc,
                 &pe->plasmaFlickerTime, SHADER_UNIFORM_FLOAT);

  // Bind gradient LUT texture
  SetShaderValueTexture(pe->plasmaShader, pe->plasmaGradientLUTLoc,
                        ColorLUTGetTexture(pe->plasmaGradientLUT));
}
