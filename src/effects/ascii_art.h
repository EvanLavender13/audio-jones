// ASCII art effect module
// Converts frame to text characters based on luminance

#ifndef ASCII_ART_H
#define ASCII_ART_H

#include "raylib.h"
#include <stdbool.h>

struct AsciiArtConfig {
  bool enabled = false;
  float cellSize =
      8.0f; // Cell size in pixels (4-32). Larger = fewer, bigger characters.
  int colorMode = 0;        // 0 = Original colors, 1 = Mono, 2 = CRT green
  float foregroundR = 0.0f; // Mono mode foreground color
  float foregroundG = 1.0f;
  float foregroundB = 0.0f;
  float backgroundR = 0.0f; // Mono mode background color
  float backgroundG = 0.02f;
  float backgroundB = 0.0f;
  bool invert = false; // Swap light/dark character mapping
};

typedef struct AsciiArtEffect {
  Shader shader;
  int resolutionLoc;
  int cellPixelsLoc;
  int colorModeLoc;
  int foregroundLoc;
  int backgroundLoc;
  int invertLoc;
} AsciiArtEffect;

// Returns true on success, false if shader fails to load
bool AsciiArtEffectInit(AsciiArtEffect *e);

// Sets all uniforms
void AsciiArtEffectSetup(AsciiArtEffect *e, const AsciiArtConfig *cfg);

// Unloads shader
void AsciiArtEffectUninit(AsciiArtEffect *e);

// Returns default config
AsciiArtConfig AsciiArtConfigDefault(void);

#endif // ASCII_ART_H
