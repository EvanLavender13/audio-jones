#include "noise_texture.h"
#include "rlgl.h"
#include <stdint.h>
#include <stdlib.h>

#define NOISE_SIZE 1024

static Texture2D noiseTexture = {0};

static uint32_t pcg_hash(uint32_t input) {
  uint32_t state = input * 747796405u + 2891336453u;
  uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

void NoiseTextureInit(void) {
  uint8_t *data =
      (uint8_t *)malloc(NOISE_SIZE * NOISE_SIZE * 4 * sizeof(uint8_t));
  if (data == NULL) {
    TraceLog(LOG_ERROR, "NOISE: Failed to allocate pixel buffer");
    return;
  }

  for (int y = 0; y < NOISE_SIZE; y++) {
    for (int x = 0; x < NOISE_SIZE; x++) {
      uint32_t seed = (uint32_t)(y * NOISE_SIZE + x);
      int idx = (y * NOISE_SIZE + x) * 4;
      data[idx + 0] = (uint8_t)((pcg_hash(seed) >> 24) & 0xFF);
      data[idx + 1] = (uint8_t)((pcg_hash(seed + 1) >> 24) & 0xFF);
      data[idx + 2] = (uint8_t)((pcg_hash(seed + 2) >> 24) & 0xFF);
      data[idx + 3] = (uint8_t)((pcg_hash(seed + 3) >> 24) & 0xFF);
    }
  }

  noiseTexture.id = rlLoadTexture(data, NOISE_SIZE, NOISE_SIZE,
                                  RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
  noiseTexture.width = NOISE_SIZE;
  noiseTexture.height = NOISE_SIZE;
  noiseTexture.format = RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

  free(data);

  SetTextureWrap(noiseTexture, TEXTURE_WRAP_REPEAT);
  GenTextureMipmaps(&noiseTexture);
  SetTextureFilter(noiseTexture, TEXTURE_FILTER_TRILINEAR);

  TraceLog(LOG_INFO, "NOISE: Created %dx%d RGBA noise texture (id=%u)",
           NOISE_SIZE, NOISE_SIZE, noiseTexture.id);
}

Texture2D NoiseTextureGet(void) { return noiseTexture; }

void NoiseTextureUninit(void) { UnloadTexture(noiseTexture); }
