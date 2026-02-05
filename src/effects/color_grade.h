// Color grade effect module
// Full-spectrum color manipulation: hue shift, saturation, brightness,
// contrast, temperature, and lift/gamma/gain

#ifndef COLOR_GRADE_H
#define COLOR_GRADE_H

#include "raylib.h"
#include <stdbool.h>

struct ColorGradeConfig {
  bool enabled = false;
  float hueShift = 0.0f;    // Hue rotation (0-1 normalized from 0-360 degrees)
  float saturation = 1.0f;  // Color intensity (0-2, 1 = neutral)
  float brightness = 0.0f;  // Exposure in F-stops (-2 to +2)
  float contrast = 1.0f;    // Log-space contrast (0.5-2, 1 = neutral)
  float temperature = 0.0f; // Cool/warm shift (-1 to +1)
  float shadowsOffset = 0.0f;    // Lift - shadows adjustment (-0.5 to +0.5)
  float midtonesOffset = 0.0f;   // Gamma - midtones adjustment (-0.5 to +0.5)
  float highlightsOffset = 0.0f; // Gain - highlights adjustment (-0.5 to +0.5)
};

typedef struct ColorGradeEffect {
  Shader shader;
  int hueShiftLoc;
  int saturationLoc;
  int brightnessLoc;
  int contrastLoc;
  int temperatureLoc;
  int shadowsOffsetLoc;
  int midtonesOffsetLoc;
  int highlightsOffsetLoc;
} ColorGradeEffect;

// Returns true on success, false if shader fails to load
bool ColorGradeEffectInit(ColorGradeEffect *e);

// Sets all uniforms
void ColorGradeEffectSetup(ColorGradeEffect *e, const ColorGradeConfig *cfg);

// Unloads shader
void ColorGradeEffectUninit(ColorGradeEffect *e);

// Returns default config
ColorGradeConfig ColorGradeConfigDefault(void);

// Registers modulatable params with the modulation engine
void ColorGradeRegisterParams(ColorGradeConfig *cfg);

#endif // COLOR_GRADE_H
