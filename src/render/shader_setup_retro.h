#ifndef SHADER_SETUP_RETRO_H
#define SHADER_SETUP_RETRO_H

#include "effects/ascii_art.h"
#include "effects/glitch.h"
#include "effects/matrix_rain.h"
#include "effects/pixelation.h"
#include "effects/synthwave.h"
#include "post_effect.h"

void SetupPixelation(PostEffect *pe);
void SetupGlitch(PostEffect *pe);
void SetupAsciiArt(PostEffect *pe);
void SetupMatrixRain(PostEffect *pe);
void SetupSynthwave(PostEffect *pe);

#endif // SHADER_SETUP_RETRO_H
