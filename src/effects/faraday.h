// Faraday effect module
// FFT-driven standing wave interference pattern (Faraday instability)

#ifndef FARADAY_H
#define FARADAY_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct FaradayConfig {
  bool enabled = false;

  // Wave
  int waveSource = 0; // 0=audio (FFT-driven), 1=parametric
  int waveShape =
      0; // Lattice wave shape: 0=sine, 1=triangle, 2=sawtooth, 3=square
  float waveFreq = 30.0f; // Parametric temporal frequency (5.0-100.0)
  float waveSpeed = 2.0f; // Parametric animation speed (0.0-10.0)
  int waveCount = 3;      // Lattice vectors: 1=stripes, 2=squares, 3=hexagons,
                          // 4+=quasicrystal (1-6)
  float spatialScale =
      0.1f; // Maps FFT frequency to spatial wave number density (0.01-1.0)
  float visualGain = 1.5f;    // Output intensity multiplier (0.5-5.0)
  float rotationSpeed = 0.0f; // Lattice rotation rate radians/second (-PI..PI)
  float rotationAngle =
      0.0f;       // Static lattice orientation offset radians (-PI..PI)
  int layers = 8; // Number of frequency layers (1-16)

  // Audio
  float baseFreq = 55.0f;   // Lowest driven frequency (27.5-440)
  float maxFreq = 14000.0f; // Highest driven frequency (1000-16000)
  float gain = 2.0f;        // FFT energy sensitivity (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent (0.1-3.0)

  // Trail
  float decayHalfLife = 0.3f; // Trail persistence seconds (0.05-5.0)
  int diffusionScale = 2;     // Spatial blur tap spacing (0-8)

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend compositor strength (0.0-5.0)
  ColorConfig gradient;
};

#define FARADAY_CONFIG_FIELDS                                                  \
  enabled, waveSource, waveShape, waveFreq, waveSpeed, waveCount,              \
      spatialScale, visualGain, rotationSpeed, rotationAngle, layers,          \
      baseFreq, maxFreq, gain, curve, decayHalfLife, diffusionScale,           \
      blendMode, blendIntensity, gradient

typedef struct ColorLUT ColorLUT;

typedef struct FaradayEffect {
  Shader shader;
  ColorLUT *colorLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFftTexture;
  float time;
  float rotationAccum;

  // Uniform locations
  int resolutionLoc;
  int timeLoc;
  int waveCountLoc;
  int spatialScaleLoc;
  int visualGainLoc;
  int rotationOffsetLoc;
  int layersLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int diffusionScaleLoc;
  int decayFactorLoc;
  int gradientLUTLoc;
  int waveSourceLoc;
  int waveShapeLoc;
  int waveFreqLoc;
} FaradayEffect;

// Returns true on success, false if shader fails to load
bool FaradayEffectInit(FaradayEffect *effect, const FaradayConfig *cfg,
                       int width, int height);

// Binds all uniforms, updates LUT texture
void FaradayEffectSetup(FaradayEffect *effect, const FaradayConfig *cfg,
                        float deltaTime, const Texture2D &fftTexture);

// Renders Faraday pattern into ping-pong trail buffer with decay blending
void FaradayEffectRender(FaradayEffect *effect, const FaradayConfig *cfg,
                         float deltaTime, int screenWidth, int screenHeight);

// Reallocates ping-pong render textures on resolution change
void FaradayEffectResize(FaradayEffect *effect, int width, int height);

// Unloads shader and frees LUT
void FaradayEffectUninit(FaradayEffect *effect);

// Registers modulatable params with the modulation engine
void FaradayRegisterParams(FaradayConfig *cfg);

FaradayEffect *GetFaradayEffect(PostEffect *pe);

#endif // FARADAY_H
