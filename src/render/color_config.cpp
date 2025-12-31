#include "color_config.h"
#include <math.h>

void ColorConfigRGBToHSV(Color c, float* outH, float* outS, float* outV)
{
    const float r = c.r / 255.0f;
    const float g = c.g / 255.0f;
    const float b = c.b / 255.0f;
    const float maxC = fmaxf(r, fmaxf(g, b));
    const float minC = fminf(r, fminf(g, b));
    const float delta = maxC - minC;

    *outV = maxC;
    *outS = (maxC > 0.00001f) ? (delta / maxC) : 0.0f;

    if (delta < 0.00001f) {
        *outH = 0.0f;
        return;
    }

    float hue;
    if (maxC == r) {
        hue = fmodf((g - b) / delta, 6.0f);
    } else if (maxC == g) {
        hue = (b - r) / delta + 2.0f;
    } else {
        hue = (r - g) / delta + 4.0f;
    }
    hue /= 6.0f;
    if (hue < 0.0f) {
        hue += 1.0f;
    }
    *outH = hue;
}
