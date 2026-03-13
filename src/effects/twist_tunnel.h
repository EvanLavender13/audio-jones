// Twist Tunnel effect module
// Nested platonic solid wireframes with per-layer twist and FFT-reactive glow

#ifndef TWIST_TUNNEL_H
#define TWIST_TUNNEL_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct TwistTunnelConfig {
  bool enabled = false;

  // Geometry
  int shape = 1; // Platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa
  int layerCount = 12; // Number of nested wireframe layers (2-20)
  float scaleRatio =
      0.8f; // Scale multiplier between successive layers (0.5-0.99)

  // Twist
  float twistAngle = 0.5f; // Static yaw twist per layer in radians (-PI..+PI)
  float twistSpeed = 0.1f; // Yaw twist accumulation rate rad/s (-PI..+PI)
  float twistPitch = 0.0f; // Static pitch twist per layer in radians (-PI..+PI)
  float twistPitchSpeed =
      0.0f; // Pitch twist accumulation rate rad/s (-PI..+PI)

  // Projection
  float perspective = 4.0f; // Camera projection depth (1.0-20.0)
  float scale = 1.2f;       // Overall wireframe size (0.1-5.0)

  // Glow
  float lineWidth = 2.0f;     // Glow falloff width in pixel units (0.5-10.0)
  float glowIntensity = 1.0f; // Glow brightness (0.1-5.0)
  float contrast = 3.0f;      // tanh output squaring factor (0.5-10.0)

  // Camera
  DualLissajousConfig lissajous = {
      .amplitude = 0.3f,
      .motionSpeed = 1.0f,
      .freqX1 = 0.07f,
      .freqY1 = 0.11f,
  };

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

#define TWIST_TUNNEL_CONFIG_FIELDS                                             \
  enabled, shape, layerCount, scaleRatio, twistAngle, twistSpeed, twistPitch,  \
      twistPitchSpeed, perspective, scale, lineWidth, glowIntensity, contrast, \
      lissajous, baseFreq, maxFreq, gain, curve, baseBright, gradient,         \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct TwistTunnelEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float twistPhase;      // Accumulated yaw twist
  float twistPitchPhase; // Accumulated pitch twist

  int resolutionLoc;
  int layerCountLoc;
  int scaleRatioLoc;
  int twistAngleLoc;
  int twistPhaseLoc;
  int twistPitchLoc;
  int twistPitchPhaseLoc;
  int perspectiveLoc;
  int scaleLoc;
  int cameraPitchLoc;
  int cameraYawLoc;
  int lineWidthLoc;
  int glowIntensityLoc;
  int contrastLoc;
  int verticesLoc; // uniform vec3 vertices[20]
  int vertexCountLoc;
  int edgeIdxLoc; // uniform vec2 edgeIdx[30]
  int edgeCountLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} TwistTunnelEffect;

// Returns true on success, false if shader fails to load
bool TwistTunnelEffectInit(TwistTunnelEffect *e, const TwistTunnelConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void TwistTunnelEffectSetup(TwistTunnelEffect *e, TwistTunnelConfig *cfg,
                            float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void TwistTunnelEffectUninit(TwistTunnelEffect *e);

// Returns default config
TwistTunnelConfig TwistTunnelConfigDefault(void);

// Registers modulatable params with the modulation engine
void TwistTunnelRegisterParams(TwistTunnelConfig *cfg);

#endif // TWIST_TUNNEL_H
