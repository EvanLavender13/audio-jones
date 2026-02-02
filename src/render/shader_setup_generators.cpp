#include "shader_setup_generators.h"
#include "color_lut.h"
#include "post_effect.h"

void SetupConstellation(PostEffect *pe) {
  ConstellationConfig &cfg = pe->effects.constellation;

  // Update LUT textures from gradient configs
  ColorLUTUpdate(pe->constellationPointLUT, &cfg.pointGradient);
  ColorLUTUpdate(pe->constellationLineLUT, &cfg.lineGradient);

  // Time uniform
  SetShaderValue(pe->constellationShader, pe->constellationTimeLoc,
                 &pe->constellationTime, SHADER_UNIFORM_FLOAT);

  // Float uniforms
  SetShaderValue(pe->constellationShader, pe->constellationGridScaleLoc,
                 &cfg.gridScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationAnimSpeedLoc,
                 &cfg.animSpeed, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationWanderAmpLoc,
                 &cfg.wanderAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationRadialFreqLoc,
                 &cfg.radialFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationRadialAmpLoc,
                 &cfg.radialAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationRadialSpeedLoc,
                 &cfg.radialSpeed, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->constellationShader, pe->constellationGlowScaleLoc,
                 &cfg.glowScale, SHADER_UNIFORM_FLOAT);
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

  // Bind point LUT texture (slot 1)
  int pointLUTSlot = 1;
  SetShaderValue(pe->constellationShader, pe->constellationPointLUTLoc,
                 &pointLUTSlot, SHADER_UNIFORM_INT);
  SetShaderValueTexture(pe->constellationShader, pe->constellationPointLUTLoc,
                        pe->constellationPointLUT->texture);

  // Bind line LUT texture (slot 2)
  int lineLUTSlot = 2;
  SetShaderValue(pe->constellationShader, pe->constellationLineLUTLoc,
                 &lineLUTSlot, SHADER_UNIFORM_INT);
  SetShaderValueTexture(pe->constellationShader, pe->constellationLineLUTLoc,
                        pe->constellationLineLUT->texture);
}
