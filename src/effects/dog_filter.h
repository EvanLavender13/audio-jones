// DoG Filter effect module
// Difference-of-Gaussians edge detection with thresholded sharpening

#ifndef DOG_FILTER_H
#define DOG_FILTER_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

struct DogFilterConfig {
  bool enabled = false;
  float sigma = 1.5f; // Edge Gaussian sigma (0.5-5.0)
  float tau = 0.99f;  // Gaussian weighting (0.9-1.0)
  float phi = 2.0f;   // Threshold steepness (0.5-10.0)
};

#define DOG_FILTER_CONFIG_FIELDS enabled, sigma, tau, phi

typedef struct DogFilterEffect {
  Shader shader;
  int resolutionLoc;
  int sigmaLoc;
  int tauLoc;
  int phiLoc;
} DogFilterEffect;

// Returns true on success, false if shader fails to load
bool DogFilterEffectInit(DogFilterEffect *e);

// Sets all uniforms
void DogFilterEffectSetup(const DogFilterEffect *e, const DogFilterConfig *cfg);

// Unloads shader
void DogFilterEffectUninit(const DogFilterEffect *e);

// Registers modulatable params with the modulation engine
void DogFilterRegisterParams(DogFilterConfig *cfg);

DogFilterEffect *GetDogFilterEffect(PostEffect *pe);

#endif // DOG_FILTER_H
