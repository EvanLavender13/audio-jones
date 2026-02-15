// False color effect module
// Maps luminance to user-defined gradient via 1D LUT texture

#ifndef FALSE_COLOR_H
#define FALSE_COLOR_H

#include "raylib.h"
#include "render/color_config.h"
#include <stdbool.h>

struct FalseColorConfig {
  bool enabled = false;
  ColorConfig gradient = {
      .mode = COLOR_MODE_GRADIENT,
      .gradientStops =
          {
              {0.0f, {0, 255, 255, 255}}, // Cyan at shadows
              {1.0f, {255, 0, 255, 255}}  // Magenta at highlights
          },
      .gradientStopCount = 2};
  float intensity = 1.0f; // Blend: 0 = original, 1 = full false color
};

#define FALSE_COLOR_CONFIG_FIELDS enabled, gradient, intensity

typedef struct ColorLUT ColorLUT;

typedef struct FalseColorEffect {
  Shader shader;
  int intensityLoc;
  int gradientLUTLoc;
  ColorLUT *lut;
} FalseColorEffect;

// Returns true on success, false if shader fails to load
bool FalseColorEffectInit(FalseColorEffect *e, const FalseColorConfig *cfg);

// Binds all uniforms and updates LUT texture
void FalseColorEffectSetup(FalseColorEffect *e, const FalseColorConfig *cfg);

// Unloads shader and frees LUT
void FalseColorEffectUninit(FalseColorEffect *e);

// Returns default config
FalseColorConfig FalseColorConfigDefault(void);

// Registers modulatable params with the modulation engine
void FalseColorRegisterParams(FalseColorConfig *cfg);

#endif // FALSE_COLOR_H
