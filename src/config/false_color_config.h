#ifndef FALSE_COLOR_CONFIG_H
#define FALSE_COLOR_CONFIG_H

#include "render/color_config.h"

// False Color: Maps luminance to user-defined gradient via 1D LUT texture.
// Supports Solid, Rainbow, and multi-stop gradients through ColorConfig.
struct FalseColorConfig {
    bool enabled = false;
    ColorConfig gradient = {
        .mode = COLOR_MODE_GRADIENT,
        .gradientStops = {
            {0.0f, {0, 255, 255, 255}},    // Cyan at shadows
            {1.0f, {255, 0, 255, 255}}     // Magenta at highlights
        },
        .gradientStopCount = 2
    };
    float intensity = 1.0f;  // Blend: 0 = original, 1 = full false color
};

#endif // FALSE_COLOR_CONFIG_H
