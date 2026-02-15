// Kuwahara painterly filter effect module
// Applies anisotropic smoothing that preserves edges

#ifndef KUWAHARA_H
#define KUWAHARA_H

#include "raylib.h"
#include <stdbool.h>

struct KuwaharaConfig {
  bool enabled = false;
  float radius = 4.0f; // Kernel radius, cast to int in shader (2-12)
};

#define KUWAHARA_CONFIG_FIELDS enabled, radius

typedef struct KuwaharaEffect {
  Shader shader;
  int resolutionLoc;
  int radiusLoc;
} KuwaharaEffect;

// Returns true on success, false if shader fails to load
bool KuwaharaEffectInit(KuwaharaEffect *e);

// Sets all uniforms
void KuwaharaEffectSetup(KuwaharaEffect *e, const KuwaharaConfig *cfg);

// Unloads shader
void KuwaharaEffectUninit(KuwaharaEffect *e);

// Returns default config
KuwaharaConfig KuwaharaConfigDefault(void);

// Registers modulatable params with the modulation engine
void KuwaharaRegisterParams(KuwaharaConfig *cfg);

#endif // KUWAHARA_H
