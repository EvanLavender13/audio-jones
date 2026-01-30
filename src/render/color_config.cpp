#include "color_config.h"
#include "gradient.h"
#include <math.h>
#include <string.h>

void ColorConfigRGBToHSV(Color c, float *outH, float *outS, float *outV) {
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

bool ColorConfigEquals(const ColorConfig *a, const ColorConfig *b) {
  if (a->mode != b->mode) {
    return false;
  }

  if (a->mode == COLOR_MODE_SOLID) {
    return memcmp(&a->solid, &b->solid, sizeof(Color)) == 0;
  }

  if (a->mode == COLOR_MODE_RAINBOW) {
    return a->rainbowHue == b->rainbowHue &&
           a->rainbowRange == b->rainbowRange &&
           a->rainbowSat == b->rainbowSat && a->rainbowVal == b->rainbowVal;
  }

  if (a->gradientStopCount != b->gradientStopCount) {
    return false;
  }
  for (int i = 0; i < a->gradientStopCount; i++) {
    if (a->gradientStops[i].position != b->gradientStops[i].position) {
      return false;
    }
    if (memcmp(&a->gradientStops[i].color, &b->gradientStops[i].color,
               sizeof(Color)) != 0) {
      return false;
    }
  }
  return true;
}

float ColorConfigAgentHue(const ColorConfig *color, int agentIndex,
                          int totalAgents) {
  float hue;
  const float t = (float)agentIndex / (float)totalAgents;

  if (color->mode == COLOR_MODE_SOLID) {
    float s;
    float v;
    ColorConfigRGBToHSV(color->solid, &hue, &s, &v);
    if (s < 0.1f) {
      hue = t;
    }
  } else if (color->mode == COLOR_MODE_GRADIENT) {
    const Color sampled =
        GradientEvaluate(color->gradientStops, color->gradientStopCount, t);
    float s;
    float v;
    ColorConfigRGBToHSV(sampled, &hue, &s, &v);
  } else {
    hue = (color->rainbowHue + t * color->rainbowRange) / 360.0f;
    hue = fmodf(hue, 1.0f);
    if (hue < 0.0f) {
      hue += 1.0f;
    }
  }
  return hue;
}

void ColorConfigGetSV(const ColorConfig *color, float *outS, float *outV) {
  if (color->mode == COLOR_MODE_SOLID) {
    float h;
    ColorConfigRGBToHSV(color->solid, &h, outS, outV);
  } else {
    *outS = color->rainbowSat;
    *outV = color->rainbowVal;
  }
}
