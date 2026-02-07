#include "draw_utils.h"
#include "../config/constants.h"
#include "gradient.h"
#include <math.h>

Color ColorWithOpacity(Color color, float opacity) {
  color.a = (unsigned char)(color.a * opacity);
  return color;
}

Color ColorFromConfig(const ColorConfig *color, float t, float opacity) {
  Color result;
  const float interp = 1.0f - fabsf(2.0f * t - 1.0f);

  if (color->mode == COLOR_MODE_RAINBOW) {
    float hue = color->rainbowHue + interp * color->rainbowRange;
    hue = fmodf(hue, 360.0f);
    if (hue < 0.0f) {
      hue += 360.0f;
    }
    result = ColorFromHSV(hue, color->rainbowSat, color->rainbowVal);
  } else if (color->mode == COLOR_MODE_GRADIENT) {
    result = GradientEvaluate(color->gradientStops, color->gradientStopCount,
                              interp);
  } else if (color->mode == COLOR_MODE_PALETTE) {
    const float r =
        color->paletteAR +
        color->paletteBR *
            cosf(TWO_PI_F * (color->paletteCR * interp + color->paletteDR));
    const float g =
        color->paletteAG +
        color->paletteBG *
            cosf(TWO_PI_F * (color->paletteCG * interp + color->paletteDG));
    const float b =
        color->paletteAB +
        color->paletteBB *
            cosf(TWO_PI_F * (color->paletteCB * interp + color->paletteDB));
    result.r = (unsigned char)(fminf(fmaxf(r, 0.0f), 1.0f) * 255.0f);
    result.g = (unsigned char)(fminf(fmaxf(g, 0.0f), 1.0f) * 255.0f);
    result.b = (unsigned char)(fminf(fmaxf(b, 0.0f), 1.0f) * 255.0f);
    result.a = 255;
  } else {
    result = color->solid;
  }
  return ColorWithOpacity(result, opacity);
}
