#ifndef DRAW_UTILS_H
#define DRAW_UTILS_H

#include <raylib.h>
#include "color_config.h"

Color ColorWithOpacity(Color color, float opacity);

Color ColorFromConfig(const ColorConfig* color, float t, float opacity);

#endif // DRAW_UTILS_H
