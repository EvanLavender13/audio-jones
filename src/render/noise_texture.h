#ifndef NOISE_TEXTURE_H
#define NOISE_TEXTURE_H

#include "raylib.h"

// Generate and upload noise texture (call once at startup)
void NoiseTextureInit(void);

// Return the shared noise Texture2D
Texture2D NoiseTextureGet(void);

// Unload at shutdown
void NoiseTextureUninit(void);

#endif // NOISE_TEXTURE_H
