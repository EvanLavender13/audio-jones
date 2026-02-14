// Hue remap effect module
// Replaces source hue with user-defined gradient via 1D LUT texture

#ifndef HUE_REMAP_H
#define HUE_REMAP_H

#include "raylib.h"
#include "render/color_config.h"
#include <stdbool.h>

struct HueRemapConfig {
  bool enabled = false;
  ColorConfig gradient = {.mode = COLOR_MODE_RAINBOW}; // Custom color wheel
  float shift = 0.0f;     // Rotates through palette (0.0-1.0)
  float intensity = 1.0f; // Global blend strength (0.0-1.0)
  float radial = 0.0f; // Blend varies with distance from center (-1.0 to 1.0)
  float cx = 0.5f;     // Center X (0.0-1.0)
  float cy = 0.5f;     // Center Y (0.0-1.0)
};

typedef struct ColorLUT ColorLUT;

typedef struct HueRemapEffect {
  Shader shader;
  int shiftLoc;
  int intensityLoc;
  int radialLoc;
  int centerLoc;
  int resolutionLoc;
  int gradientLUTLoc;
  ColorLUT *gradientLUT;
} HueRemapEffect;

// Returns true on success, false if shader fails to load
bool HueRemapEffectInit(HueRemapEffect *e, const HueRemapConfig *cfg);

// Binds all uniforms and updates LUT texture
void HueRemapEffectSetup(HueRemapEffect *e, const HueRemapConfig *cfg);

// Unloads shader and frees LUT
void HueRemapEffectUninit(HueRemapEffect *e);

// Returns default config
HueRemapConfig HueRemapConfigDefault(void);

// Registers modulatable params with the modulation engine
void HueRemapRegisterParams(HueRemapConfig *cfg);

#endif // HUE_REMAP_H
