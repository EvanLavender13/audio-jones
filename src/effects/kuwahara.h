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

typedef struct KuwaharaEffect {
  Shader shader;
  int resolutionLoc;
  int radiusLoc;
} KuwaharaEffect;

bool KuwaharaEffectInit(KuwaharaEffect *e);
void KuwaharaEffectSetup(KuwaharaEffect *e, const KuwaharaConfig *cfg);
void KuwaharaEffectUninit(KuwaharaEffect *e);
KuwaharaConfig KuwaharaConfigDefault(void);

#endif // KUWAHARA_H
