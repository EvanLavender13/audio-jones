#include "shader_setup_optical.h"
#include "post_effect.h"

void SetupBloom(PostEffect *pe) {
  BloomEffectSetup(&pe->bloom, &pe->effects.bloom);
}

void SetupBokeh(PostEffect *pe) {
  BokehEffectSetup(&pe->bokeh, &pe->effects.bokeh);
}

void SetupHeightfieldRelief(PostEffect *pe) {
  HeightfieldReliefEffectSetup(&pe->heightfieldRelief,
                               &pe->effects.heightfieldRelief);
}

void SetupAnamorphicStreak(PostEffect *pe) {
  AnamorphicStreakEffectSetup(&pe->anamorphicStreak,
                              &pe->effects.anamorphicStreak);
}

void SetupPhiBlur(PostEffect *pe) {
  PhiBlurEffectSetup(&pe->phiBlur, &pe->effects.phiBlur);
}
