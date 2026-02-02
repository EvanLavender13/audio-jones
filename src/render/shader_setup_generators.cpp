#include "shader_setup_generators.h"
#include "color_lut.h"
#include "config/interference_config.h"
#include "post_effect.h"
#include <math.h>

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

void SetupInterference(PostEffect *pe) {
  InterferenceConfig &cfg = pe->effects.interference;

  // Update color LUT from gradient config
  ColorLUTUpdate(pe->interferenceColorLUT, &cfg.color);

  // Compute animated source positions with circular distribution
  static const float TWO_PI = 6.28318530718f;
  float sources[16]; // 8 sources * 2 components (x, y)
  float phases[8];   // Per-source phase offsets
  const int count = cfg.sourceCount > 8 ? 8 : cfg.sourceCount;

  for (int i = 0; i < count; i++) {
    float angle = TWO_PI * (float)i / (float)count + cfg.patternAngle;
    float baseX = cfg.baseRadius * cosf(angle);
    float baseY = cfg.baseRadius * sinf(angle);
    float perSourceOffset = (float)i / (float)count * TWO_PI;

    float offsetX, offsetY;
    DualLissajousCompute(&cfg.lissajous, pe->interferenceSourcePhase,
                         perSourceOffset, &offsetX, &offsetY);
    sources[i * 2 + 0] = baseX + offsetX;
    sources[i * 2 + 1] = baseY + offsetY;
    phases[i] = perSourceOffset;
  }

  // Set time uniform
  SetShaderValue(pe->interferenceShader, pe->interferenceTimeLoc,
                 &pe->interferenceTime, SHADER_UNIFORM_FLOAT);

  // Set source array uniforms
  SetShaderValueV(pe->interferenceShader, pe->interferenceSourcesLoc, sources,
                  SHADER_UNIFORM_VEC2, count);
  SetShaderValueV(pe->interferenceShader, pe->interferencePhasesLoc, phases,
                  SHADER_UNIFORM_FLOAT, count);
  SetShaderValue(pe->interferenceShader, pe->interferenceSourceCountLoc, &count,
                 SHADER_UNIFORM_INT);

  // Wave parameters
  SetShaderValue(pe->interferenceShader, pe->interferenceWaveFreqLoc,
                 &cfg.waveFreq, SHADER_UNIFORM_FLOAT);

  // Falloff parameters
  SetShaderValue(pe->interferenceShader, pe->interferenceFalloffTypeLoc,
                 &cfg.falloffType, SHADER_UNIFORM_INT);
  SetShaderValue(pe->interferenceShader, pe->interferenceFalloffStrengthLoc,
                 &cfg.falloffStrength, SHADER_UNIFORM_FLOAT);

  // Boundary parameters
  int boundariesInt = cfg.boundaries ? 1 : 0;
  SetShaderValue(pe->interferenceShader, pe->interferenceBoundariesLoc,
                 &boundariesInt, SHADER_UNIFORM_INT);
  SetShaderValue(pe->interferenceShader, pe->interferenceReflectionGainLoc,
                 &cfg.reflectionGain, SHADER_UNIFORM_FLOAT);

  // Visualization parameters
  SetShaderValue(pe->interferenceShader, pe->interferenceVisualModeLoc,
                 &cfg.visualMode, SHADER_UNIFORM_INT);
  SetShaderValue(pe->interferenceShader, pe->interferenceContourCountLoc,
                 &cfg.contourCount, SHADER_UNIFORM_INT);
  SetShaderValue(pe->interferenceShader, pe->interferenceVisualGainLoc,
                 &cfg.visualGain, SHADER_UNIFORM_FLOAT);

  // Chroma spread (for chromatic visual mode)
  SetShaderValue(pe->interferenceShader, pe->interferenceChromaSpreadLoc,
                 &cfg.chromaSpread, SHADER_UNIFORM_FLOAT);

  // Color mode
  SetShaderValue(pe->interferenceShader, pe->interferenceColorModeLoc,
                 &cfg.colorMode, SHADER_UNIFORM_INT);

  // Bind color LUT texture
  SetShaderValueTexture(pe->interferenceShader, pe->interferenceColorLUTLoc,
                        ColorLUTGetTexture(pe->interferenceColorLUT));
}
