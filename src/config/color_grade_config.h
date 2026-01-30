#ifndef COLOR_GRADE_CONFIG_H
#define COLOR_GRADE_CONFIG_H

// Color Grade: Full-spectrum color manipulation as a reorderable transform
// effect Applies brightness, temperature, contrast, saturation,
// lift/gamma/gain, and hue shift
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

#endif // COLOR_GRADE_CONFIG_H
