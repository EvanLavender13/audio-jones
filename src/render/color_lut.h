#ifndef COLOR_LUT_H
#define COLOR_LUT_H

#include "raylib.h"
#include "color_config.h"

#define COLOR_LUT_SIZE 256

typedef struct ColorLUT {
    Texture2D texture;
    ColorConfig cachedConfig;
} ColorLUT;

// Allocate LUT and generate texture from config
ColorLUT* ColorLUTInit(const ColorConfig* config);

// Release texture and free memory
void ColorLUTUninit(ColorLUT* lut);

// Regenerate texture if config changed
void ColorLUTUpdate(ColorLUT* lut, const ColorConfig* config);

// Get texture for shader binding
Texture2D ColorLUTGetTexture(const ColorLUT* lut);

#endif // COLOR_LUT_H
