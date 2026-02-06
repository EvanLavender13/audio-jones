#ifndef SHADER_SETUP_GENERATORS_H
#define SHADER_SETUP_GENERATORS_H

#include "config/effect_config.h"
#include "post_effect.h"

void SetupConstellation(PostEffect *pe);
void SetupPlasma(PostEffect *pe);
void SetupInterference(PostEffect *pe);
void SetupSolidColor(PostEffect *pe);

void SetupConstellationBlend(PostEffect *pe);
void SetupPlasmaBlend(PostEffect *pe);
void SetupInterferenceBlend(PostEffect *pe);
void SetupSolidColorBlend(PostEffect *pe);

void RenderGeneratorToScratch(PostEffect *pe, TransformEffectType type,
                              RenderTexture2D *src);

#endif // SHADER_SETUP_GENERATORS_H
