#ifndef COLOR_CONFIG_H
#define COLOR_CONFIG_H

#include "raylib.h"

#define MAX_GRADIENT_STOPS 8

typedef enum {
  COLOR_MODE_SOLID,
  COLOR_MODE_RAINBOW,
  COLOR_MODE_GRADIENT,
  COLOR_MODE_PALETTE
} ColorMode;

struct GradientStop {
  float position = 0.0f; // 0.0-1.0 along waveform
  Color color = WHITE;
};

struct ColorConfig {
  ColorMode mode = COLOR_MODE_SOLID;
  Color solid = WHITE;
  float rainbowHue = 0.0f;     // Starting hue offset (0-360)
  float rainbowRange = 360.0f; // Hue degrees to span (0-360)
  float rainbowSat = 1.0f;     // Saturation (0-1)
  float rainbowVal = 1.0f;     // Value/brightness (0-1)

  GradientStop gradientStops[MAX_GRADIENT_STOPS] = {
      {0.0f, {0, 255, 255, 255}}, // Cyan
      {1.0f, {255, 0, 255, 255}}  // Magenta
  };
  int gradientStopCount = 2;

  // Cosine palette coefficients: color(t) = a + b * cos(2pi * (c*t + d))
  float paletteAR = 0.5f;  // Bias red
  float paletteAG = 0.5f;  // Bias green
  float paletteAB = 0.5f;  // Bias blue
  float paletteBR = 0.5f;  // Amplitude red
  float paletteBG = 0.5f;  // Amplitude green
  float paletteBB = 0.5f;  // Amplitude blue
  float paletteCR = 1.0f;  // Frequency red
  float paletteCG = 1.0f;  // Frequency green
  float paletteCB = 1.0f;  // Frequency blue
  float paletteDR = 0.0f;  // Phase red
  float paletteDG = 0.33f; // Phase green
  float paletteDB = 0.67f; // Phase blue
};

// Convert RGB color to HSV. Outputs hue (0-1), saturation (0-1), value (0-1).
void ColorConfigRGBToHSV(Color c, float *outH, float *outS, float *outV);

// Compare two ColorConfigs for equality (all relevant fields by mode).
bool ColorConfigEquals(const ColorConfig *a, const ColorConfig *b);

// Compute agent hue from color config and population index.
// Grayscale solid colors distribute hues evenly to prevent clustering.
float ColorConfigAgentHue(const ColorConfig *color, int agentIndex,
                          int totalAgents);

// Extract saturation and value from color config for deposit coloring.
void ColorConfigGetSV(const ColorConfig *color, float *outS, float *outV);

#endif // COLOR_CONFIG_H
