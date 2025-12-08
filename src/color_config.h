#ifndef COLOR_CONFIG_H
#define COLOR_CONFIG_H

#include "raylib.h"

typedef enum {
    COLOR_MODE_SOLID,
    COLOR_MODE_RAINBOW
} ColorMode;

struct ColorConfig {
    ColorMode mode = COLOR_MODE_SOLID;
    Color solid = WHITE;
    float rainbowHue = 0.0f;       // Starting hue offset (0-360)
    float rainbowRange = 360.0f;   // Hue degrees to span (0-360)
    float rainbowSat = 1.0f;       // Saturation (0-1)
    float rainbowVal = 1.0f;       // Value/brightness (0-1)
};

#endif // COLOR_CONFIG_H
