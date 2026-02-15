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
  float cx = 0.5f;        // Center X (0.0-1.0)
  float cy = 0.5f;        // Center Y (0.0-1.0)

  // Blend spatial coefficients
  float blendRadial = 0.0f;      // Distance from center (-1.0 to 1.0)
  float blendAngular = 0.0f;     // Angular sector pattern (-1.0 to 1.0)
  int blendAngularFreq = 2;      // Angular repetitions (1-8)
  float blendLinear = 0.0f;      // Linear gradient (-1.0 to 1.0)
  float blendLinearAngle = 0.0f; // Gradient direction in radians
  float blendLuminance = 0.0f;   // Brightness-based blend (-1.0 to 1.0)
  float blendNoise = 0.0f;       // Noise-based blend (-1.0 to 1.0)

  // Shift spatial coefficients
  float shiftRadial = 0.0f;      // Radial shift offset (-1.0 to 1.0)
  float shiftAngular = 0.0f;     // Angular shift offset (-1.0 to 1.0)
  int shiftAngularFreq = 2;      // Angular repetitions (1-8)
  float shiftLinear = 0.0f;      // Linear shift offset (-1.0 to 1.0)
  float shiftLinearAngle = 0.0f; // Shift direction in radians
  float shiftLuminance = 0.0f;   // Brightness-based shift (-1.0 to 1.0)
  float shiftNoise = 0.0f;       // Noise-based shift (-1.0 to 1.0)

  // Shared noise params
  float noiseScale = 5.0f; // Noise cell size (1.0-20.0)
  float noiseSpeed = 0.5f; // Noise drift speed (0.0-2.0)
};

#define HUE_REMAP_CONFIG_FIELDS                                                \
  enabled, gradient, shift, intensity, cx, cy, blendRadial, blendAngular,      \
      blendAngularFreq, blendLinear, blendLinearAngle, blendLuminance,         \
      blendNoise, shiftRadial, shiftAngular, shiftAngularFreq, shiftLinear,    \
      shiftLinearAngle, shiftLuminance, shiftNoise, noiseScale, noiseSpeed

typedef struct ColorLUT ColorLUT;

typedef struct HueRemapEffect {
  Shader shader;
  float time;
  int shiftLoc;
  int intensityLoc;
  int centerLoc;
  int resolutionLoc;
  int gradientLUTLoc;
  int blendRadialLoc;
  int blendAngularLoc;
  int blendAngularFreqLoc;
  int blendLinearLoc;
  int blendLinearAngleLoc;
  int blendLuminanceLoc;
  int blendNoiseLoc;
  int shiftRadialLoc;
  int shiftAngularLoc;
  int shiftAngularFreqLoc;
  int shiftLinearLoc;
  int shiftLinearAngleLoc;
  int shiftLuminanceLoc;
  int shiftNoiseLoc;
  int noiseScaleLoc;
  int timeLoc;
  ColorLUT *gradientLUT;
} HueRemapEffect;

// Returns true on success, false if shader fails to load
bool HueRemapEffectInit(HueRemapEffect *e, const HueRemapConfig *cfg);

// Binds all uniforms, accumulates time, and updates LUT texture
void HueRemapEffectSetup(HueRemapEffect *e, const HueRemapConfig *cfg,
                         float deltaTime);

// Unloads shader and frees LUT
void HueRemapEffectUninit(HueRemapEffect *e);

// Returns default config
HueRemapConfig HueRemapConfigDefault(void);

// Registers modulatable params with the modulation engine
void HueRemapRegisterParams(HueRemapConfig *cfg);

#endif // HUE_REMAP_H
