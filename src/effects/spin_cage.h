// SpinCage effect module
// Rotating platonic solid wireframes with per-edge FFT glow

#ifndef SPIN_CAGE_H
#define SPIN_CAGE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SpinCageConfig {
  bool enabled = false;

  // Shape
  int shape = 1; // Platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa
  int colorMode = 0; // 0=Edge Index, 1=Depth

  // Rotation
  float speedX = 0.33f;   // X-axis rotation speed rad/s (-PI..+PI)
  float speedY = 0.33f;   // Y-axis rotation speed rad/s (-PI..+PI)
  float speedZ = 0.33f;   // Z-axis rotation speed rad/s (-PI..+PI)
  float speedMult = 1.0f; // Global speed multiplier (0.1-100.0)

  // Projection
  float perspective = 4.0f; // Camera distance / projection depth (1.0-20.0)
  float scale = 1.0f;       // Wireframe size multiplier (0.1-5.0)

  // Glow
  float lineWidth = 2.0f;     // Glow falloff width in pixel units (0.5-10.0)
  float glowIntensity = 1.0f; // Glow brightness (0.1-5.0)

  // Audio
  float baseFreq = 55.0f;   // Low end FFT freq spread Hz (27.5-440.0)
  float maxFreq = 14000.0f; // High end FFT freq spread Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplification (0.1-10.0)
  float curve = 1.5f;       // FFT response curve / contrast (0.1-3.0)
  float baseBright = 0.15f; // Base edge visibility without audio (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SPIN_CAGE_CONFIG_FIELDS                                                \
  enabled, shape, colorMode, speedX, speedY, speedZ, speedMult, perspective,   \
      scale, lineWidth, glowIntensity, baseFreq, maxFreq, gain, curve,         \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SpinCageEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float angleX, angleY, angleZ; // Accumulated rotation angles
  int resolutionLoc;
  int edgesLoc;     // uniform vec4 edges[30]
  int edgeTLoc;     // uniform float edgeT[30]
  int edgeCountLoc; // uniform int edgeCount
  int lineWidthLoc;
  int glowIntensityLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} SpinCageEffect;

// Returns true on success, false if shader fails to load
bool SpinCageEffectInit(SpinCageEffect *e, const SpinCageConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SpinCageEffectSetup(SpinCageEffect *e, const SpinCageConfig *cfg,
                         float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void SpinCageEffectUninit(SpinCageEffect *e);

// Returns default config
SpinCageConfig SpinCageConfigDefault(void);

// Registers modulatable params with the modulation engine
void SpinCageRegisterParams(SpinCageConfig *cfg);

#endif // SPIN_CAGE_H
