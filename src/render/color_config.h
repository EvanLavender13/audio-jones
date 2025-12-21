#ifndef COLOR_CONFIG_H
#define COLOR_CONFIG_H

#include "raylib.h"

#define MAX_GRADIENT_STOPS 8

typedef enum {
    COLOR_MODE_SOLID,
    COLOR_MODE_RAINBOW,
    COLOR_MODE_GRADIENT  // Future: multi-stop gradients
} ColorMode;

// Future: gradient stop for multi-color gradients
struct GradientStop {
    float position = 0.0f;  // 0.0-1.0 along waveform
    Color color = WHITE;
};

struct ColorConfig {
    ColorMode mode = COLOR_MODE_SOLID;
    Color solid = WHITE;
    float rainbowHue = 0.0f;       // Starting hue offset (0-360)
    float rainbowRange = 360.0f;   // Hue degrees to span (0-360)
    float rainbowSat = 1.0f;       // Saturation (0-1)
    float rainbowVal = 1.0f;       // Value/brightness (0-1)

    // Gradient mode (future - not serialized yet)
    GradientStop gradientStops[MAX_GRADIENT_STOPS] = {};
    int gradientStopCount = 0;
};

#endif // COLOR_CONFIG_H
