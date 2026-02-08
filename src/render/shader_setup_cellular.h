#ifndef SHADER_SETUP_CELLULAR_H
#define SHADER_SETUP_CELLULAR_H

#include "post_effect.h"

void SetupVoronoi(PostEffect *pe);
void SetupLatticeFold(PostEffect *pe);
void SetupPhyllotaxis(PostEffect *pe);
void SetupMultiScaleGrid(PostEffect *pe);
void SetupDotMatrix(PostEffect *pe);

#endif // SHADER_SETUP_CELLULAR_H
