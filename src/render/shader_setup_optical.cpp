#include "shader_setup_optical.h"
#include "post_effect.h"

void SetupBloom(PostEffect *pe) {
  const BloomConfig *b = &pe->effects.bloom;
  SetShaderValue(pe->bloomCompositeShader, pe->bloomIntensityLoc, &b->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(pe->bloomCompositeShader, pe->bloomBloomTexLoc,
                        pe->bloomMips[0].texture);
}

void SetupBokeh(PostEffect *pe) {
  const BokehConfig *b = &pe->effects.bokeh;
  SetShaderValue(pe->bokehShader, pe->bokehRadiusLoc, &b->radius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->bokehShader, pe->bokehIterationsLoc, &b->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->bokehShader, pe->bokehBrightnessPowerLoc,
                 &b->brightnessPower, SHADER_UNIFORM_FLOAT);
}

void SetupHeightfieldRelief(PostEffect *pe) {
  const HeightfieldReliefConfig *h = &pe->effects.heightfieldRelief;
  SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefIntensityLoc,
                 &h->intensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->heightfieldReliefShader,
                 pe->heightfieldReliefReliefScaleLoc, &h->reliefScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->heightfieldReliefShader,
                 pe->heightfieldReliefLightAngleLoc, &h->lightAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->heightfieldReliefShader,
                 pe->heightfieldReliefLightHeightLoc, &h->lightHeight,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefShininessLoc,
                 &h->shininess, SHADER_UNIFORM_FLOAT);
}
