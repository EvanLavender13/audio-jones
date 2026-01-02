#include "color_lut.h"
#include "draw_utils.h"
#include <stdlib.h>

static void GenerateTexture(ColorLUT* lut, const ColorConfig* config)
{
    Image img = GenImageColor(COLOR_LUT_SIZE, 1, WHITE);
    Color* pixels = (Color*)img.data;

    for (int i = 0; i < COLOR_LUT_SIZE; i++) {
        const float t = (float)i / (float)(COLOR_LUT_SIZE - 1);
        pixels[i] = ColorFromConfig(config, t, 1.0f);
    }

    if (lut->texture.id != 0) {
        UnloadTexture(lut->texture);
    }
    lut->texture = LoadTextureFromImage(img);
    SetTextureFilter(lut->texture, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
}

ColorLUT* ColorLUTInit(const ColorConfig* config)
{
    ColorLUT* lut = (ColorLUT*)malloc(sizeof(ColorLUT));
    if (lut == NULL) {
        return NULL;
    }

    lut->texture.id = 0;
    lut->cachedConfig = *config;
    GenerateTexture(lut, config);

    return lut;
}

void ColorLUTUninit(ColorLUT* lut)
{
    if (lut == NULL) {
        return;
    }
    if (lut->texture.id != 0) {
        UnloadTexture(lut->texture);
    }
    free(lut);
}

void ColorLUTUpdate(ColorLUT* lut, const ColorConfig* config)
{
    if (lut == NULL || config == NULL) {
        return;
    }
    if (ColorConfigEquals(&lut->cachedConfig, config)) {
        return;
    }

    lut->cachedConfig = *config;
    GenerateTexture(lut, config);
}

Texture2D ColorLUTGetTexture(const ColorLUT* lut)
{
    if (lut == NULL) {
        Texture2D empty = {0};
        return empty;
    }
    return lut->texture;
}
