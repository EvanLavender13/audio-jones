#ifndef SHADER_SETUP_COLOR_H
#define SHADER_SETUP_COLOR_H

#include "post_effect.h"

void SetupColorGrade(PostEffect *pe);
void SetupFalseColor(PostEffect *pe);
void SetupPaletteQuantization(PostEffect *pe);
void SetupHueRemap(PostEffect *pe);

#endif // SHADER_SETUP_COLOR_H
