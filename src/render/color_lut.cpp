#include "color_lut.h"
#include "draw_utils.h"
#include <stdlib.h>
#include <string.h>

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

static bool ConfigEquals(const ColorConfig* a, const ColorConfig* b)
{
    if (a->mode != b->mode) {
        return false;
    }

    if (a->mode == COLOR_MODE_SOLID) {
        return memcmp(&a->solid, &b->solid, sizeof(Color)) == 0;
    }

    if (a->mode == COLOR_MODE_RAINBOW) {
        return a->rainbowHue == b->rainbowHue &&
               a->rainbowRange == b->rainbowRange &&
               a->rainbowSat == b->rainbowSat &&
               a->rainbowVal == b->rainbowVal;
    }

    // GRADIENT mode
    if (a->gradientStopCount != b->gradientStopCount) {
        return false;
    }
    for (int i = 0; i < a->gradientStopCount; i++) {
        if (a->gradientStops[i].position != b->gradientStops[i].position) {
            return false;
        }
        if (memcmp(&a->gradientStops[i].color, &b->gradientStops[i].color, sizeof(Color)) != 0) {
            return false;
        }
    }
    return true;
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
    if (ConfigEquals(&lut->cachedConfig, config)) {
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
