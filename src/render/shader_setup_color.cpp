#include "shader_setup_color.h"
#include "color_lut.h"
#include "post_effect.h"

void SetupColorGrade(PostEffect *pe) {
  const ColorGradeConfig *cg = &pe->effects.colorGrade;
  SetShaderValue(pe->colorGradeShader, pe->colorGradeHueShiftLoc, &cg->hueShift,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->colorGradeShader, pe->colorGradeSaturationLoc,
                 &cg->saturation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->colorGradeShader, pe->colorGradeBrightnessLoc,
                 &cg->brightness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->colorGradeShader, pe->colorGradeContrastLoc, &cg->contrast,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->colorGradeShader, pe->colorGradeTemperatureLoc,
                 &cg->temperature, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->colorGradeShader, pe->colorGradeShadowsOffsetLoc,
                 &cg->shadowsOffset, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->colorGradeShader, pe->colorGradeMidtonesOffsetLoc,
                 &cg->midtonesOffset, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->colorGradeShader, pe->colorGradeHighlightsOffsetLoc,
                 &cg->highlightsOffset, SHADER_UNIFORM_FLOAT);
}

void SetupFalseColor(PostEffect *pe) {
  const FalseColorConfig *fc = &pe->effects.falseColor;

  ColorLUTUpdate(pe->falseColorLUT, &fc->gradient);

  SetShaderValue(pe->falseColorShader, pe->falseColorIntensityLoc,
                 &fc->intensity, SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(pe->falseColorShader, pe->falseColorGradientLUTLoc,
                        ColorLUTGetTexture(pe->falseColorLUT));
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
  SetShaderValue(pe->halftoneShader, pe->halftoneThresholdLoc, &ht->threshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->halftoneShader, pe->halftoneSoftnessLoc, &ht->softness,
                 SHADER_UNIFORM_FLOAT);
}

void SetupPaletteQuantization(PostEffect *pe) {
  const PaletteQuantizationConfig *pq = &pe->effects.paletteQuantization;
  SetShaderValue(pe->paletteQuantizationShader,
                 pe->paletteQuantizationColorLevelsLoc, &pq->colorLevels,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->paletteQuantizationShader,
                 pe->paletteQuantizationDitherStrengthLoc, &pq->ditherStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->paletteQuantizationShader,
                 pe->paletteQuantizationBayerSizeLoc, &pq->bayerSize,
                 SHADER_UNIFORM_INT);
}
