#include "shader_setup_color.h"
#include "post_effect.h"

void SetupColorGrade(PostEffect *pe) {
  ColorGradeEffectSetup(&pe->colorGrade, &pe->effects.colorGrade);
}

void SetupFalseColor(PostEffect *pe) {
  FalseColorEffectSetup(&pe->falseColor, &pe->effects.falseColor);
}

void SetupPaletteQuantization(PostEffect *pe) {
  PaletteQuantizationEffectSetup(&pe->paletteQuantization,
                                 &pe->effects.paletteQuantization);
}
