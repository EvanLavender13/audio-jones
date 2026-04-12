// Film Grain effect module
// Gaussian noise with signal-to-noise ratio falloff for photographic grain

#ifndef FILM_GRAIN_H
#define FILM_GRAIN_H

#include "raylib.h"
#include <stdbool.h>

struct FilmGrainConfig {
  bool enabled = false;
  float intensity = 0.35f;  // Grain visibility (0.0-1.0)
  float variance = 0.4f;    // Gaussian spread (0.0-1.0)
  float snr = 6.0f;         // Signal-to-noise ratio power (0.0-16.0)
  float colorAmount = 0.0f; // Mono vs per-channel noise (0.0-1.0)
};

#define FILM_GRAIN_CONFIG_FIELDS enabled, intensity, variance, snr, colorAmount

typedef struct FilmGrainEffect {
  Shader shader;
  int timeLoc;
  int intensityLoc;
  int varianceLoc;
  int snrLoc;
  int colorAmountLoc;
  float time; // Animation accumulator
} FilmGrainEffect;

// Returns true on success, false if shader fails to load
bool FilmGrainEffectInit(FilmGrainEffect *e);

// Accumulates time and binds all uniforms for the current frame
void FilmGrainEffectSetup(FilmGrainEffect *e, const FilmGrainConfig *cfg,
                          float deltaTime);

// Unloads shader
void FilmGrainEffectUninit(FilmGrainEffect *e);

// Registers modulatable params with the modulation engine
void FilmGrainRegisterParams(FilmGrainConfig *cfg);

#endif // FILM_GRAIN_H
