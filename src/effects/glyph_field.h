// Glyph field effect module
// Renders scrolling character grids with layered depth and LCD sub-pixel
// overlay

#ifndef GLYPH_FIELD_H
#define GLYPH_FIELD_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct GlyphFieldConfig {
  bool enabled = false;

  // Grid layout
  float gridSize =
      24.0f; // Character density — cells per screen height (8.0-64.0)
  int layerCount = 2; // Overlapping grid planes (1-4)
  float layerScaleSpread =
      1.4f; // Scale ratio between successive layers (0.5-2.0)
  float layerSpeedSpread =
      1.3f;                  // Speed ratio between successive layers (0.5-2.0)
  float layerOpacity = 0.6f; // Opacity falloff per layer (0.1-1.0)

  // Scroll motion
  int scrollDirection = 0;  // 0=Horizontal, 1=Vertical, 2=Radial
  float scrollSpeed = 0.4f; // Base scroll velocity (0.0-2.0)

  // Stutter
  float stutterAmount = 0.0f;   // Fraction of lanes frozen (0.0-1.0)
  float stutterSpeed = 1.0f;    // Freeze/unfreeze toggle rate (0.1-5.0)
  float stutterDiscrete = 0.0f; // Smooth-to-cell-snap blend (0.0-1.0)

  // Character animation
  float flutterAmount = 0.3f; // Per-cell character cycling intensity (0.0-1.0)
  float flutterSpeed = 2.0f;  // Character cycling rate (0.1-10.0)

  // Wave distortion
  float waveAmplitude = 0.05f; // Sine distortion strength (0.0-0.5)
  float waveFreq = 6.0f;       // Sine distortion spatial frequency (1.0-20.0)
  float waveSpeed = 1.0f;      // Sine distortion animation speed (0.0-5.0)

  // Drift
  float driftAmount = 0.0f; // Per-cell position wander magnitude (0.0-0.5)
  float driftSpeed = 0.5f;  // Position wander rate (0.1-5.0)

  // Row variation
  float bandDistortion = 0.3f; // Step-based row height variation (0.0-1.0)

  // Inversion
  float inversionRate =
      0.1f; // Fraction of cells with inverted glyphs (0.0-1.0)
  float inversionSpeed = 0.1f; // Inversion state rotation speed (0.0-2.0)

  // LCD sub-pixel
  bool lcdMode = false;  // LCD sub-pixel RGB stripe overlay
  float lcdFreq = 1.77f; // LCD stripe spatial frequency (0.1-6.283)

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest mapped pitch Hz (27.5-440.0)
  int numOctaves = 5;       // Octave range across layers (1-8)
  float gain = 2.0f;        // FFT magnitude amplification (0.1-10.0)
  float curve = 0.7f;       // Contrast shaping exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color (gradient sampled across glyph field)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define GLYPH_FIELD_CONFIG_FIELDS                                              \
  enabled, gridSize, layerCount, layerScaleSpread, layerSpeedSpread,           \
      layerOpacity, scrollDirection, scrollSpeed, stutterAmount, stutterSpeed, \
      stutterDiscrete, flutterAmount, flutterSpeed, waveAmplitude, waveFreq,   \
      waveSpeed, driftAmount, driftSpeed, bandDistortion, inversionRate,       \
      inversionSpeed, lcdMode, lcdFreq, baseFreq, numOctaves, gain, curve,     \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct GlyphFieldEffect {
  Shader shader;
  Texture2D fontAtlas; // Loaded from fonts/font_atlas.png
  ColorLUT *gradientLUT;
  // CPU-accumulated time values (avoids jumps when speed changes)
  float scrollTime;
  float flutterTime;
  float waveTime;
  float driftTime;
  float inversionTime;
  float stutterTime;
  int resolutionLoc;
  int gridSizeLoc;
  int layerCountLoc;
  int layerScaleSpreadLoc;
  int layerSpeedSpreadLoc;
  int layerOpacityLoc;
  int scrollDirectionLoc;
  int scrollTimeLoc;
  int flutterAmountLoc;
  int flutterTimeLoc;
  int waveAmplitudeLoc;
  int waveFreqLoc;
  int waveTimeLoc;
  int driftAmountLoc;
  int driftTimeLoc;
  int bandDistortionLoc;
  int inversionRateLoc;
  int inversionTimeLoc;
  int lcdModeLoc;
  int lcdFreqLoc;
  int fontAtlasLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int stutterAmountLoc;
  int stutterTimeLoc;
  int stutterDiscreteLoc;
} GlyphFieldEffect;

// Returns true on success, false if shader or font atlas fails to load
bool GlyphFieldEffectInit(GlyphFieldEffect *e, const GlyphFieldConfig *cfg);

// Binds all uniforms including fftTexture, advances time accumulator, updates
// LUT texture
void GlyphFieldEffectSetup(GlyphFieldEffect *e, const GlyphFieldConfig *cfg,
                           float deltaTime, Texture2D fftTexture);

// Unloads shader, font atlas, and frees LUT
void GlyphFieldEffectUninit(GlyphFieldEffect *e);

// Returns default config
GlyphFieldConfig GlyphFieldConfigDefault(void);

// Registers modulatable params with the modulation engine
void GlyphFieldRegisterParams(GlyphFieldConfig *cfg);

#endif // GLYPH_FIELD_H
