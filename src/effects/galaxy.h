// Galaxy generator effect
// Tilted multi-ring spiral galaxy with dust lanes, star points, and central
// bulge glow — each ring driven by FFT semitone energy

#ifndef GALAXY_H
#define GALAXY_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct GalaxyConfig {
  bool enabled = false;

  // Geometry
  int layers = 20;             // Ring count (8-25)
  float twist = 1.0f;          // Spiral winding per ring (0.0-2.0)
  float innerStretch = 2.0f;   // Inner ring X elongation (1.0-4.0)
  float ringWidth = 42.0f;     // Ring width (2.0-50.0)
  float diskThickness = 0.04f; // Ring Y perturbation (0.01-0.5)
  float tilt = 0.3f;           // Viewing angle in radians (0.0-1.57)
  float rotation = 0.0f;       // In-plane rotation in radians (-PI..PI)

  // Animation
  float orbitSpeed = 0.1f; // Orbital animation speed (-1.0-1.0)

  // Dust & Stars
  float dustContrast = 0.5f; // Dust pow() exponent (0.1-1.5)
  float starDensity = 24.0f; // Star grid resolution per ring (2.0-64.0)
  float starBright = 0.2f;   // Star point intensity (0.05-1.0)

  // Bulge
  float bulgeSize = 20.0f;  // Center glow size (5.0-50.0)
  float bulgeBright = 0.6f; // Center glow intensity (0.0-3.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Ring brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define GALAXY_CONFIG_FIELDS                                                   \
  enabled, layers, twist, innerStretch, ringWidth, diskThickness, tilt,        \
      rotation, orbitSpeed, dustContrast, starDensity, starBright, bulgeSize,  \
      bulgeBright, baseFreq, maxFreq, gain, curve, baseBright, gradient,       \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct GalaxyEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // CPU-accumulated animation time
  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int layersLoc;
  int twistLoc;
  int innerStretchLoc;
  int ringWidthLoc;
  int diskThicknessLoc;
  int tiltLoc;
  int rotationLoc;

  int dustContrastLoc;
  int starDensityLoc;
  int starBrightLoc;
  int bulgeSizeLoc;
  int bulgeBrightLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} GalaxyEffect;

// Returns true on success, false if shader fails to load
bool GalaxyEffectInit(GalaxyEffect *e, const GalaxyConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void GalaxyEffectSetup(GalaxyEffect *e, const GalaxyConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void GalaxyEffectUninit(GalaxyEffect *e);

// Registers modulatable params with the modulation engine
void GalaxyRegisterParams(GalaxyConfig *cfg);

#endif // GALAXY_H
