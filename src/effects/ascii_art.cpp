// ASCII art effect module implementation

#include "ascii_art.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool AsciiArtEffectInit(AsciiArtEffect *e) {
  e->shader = LoadShader(NULL, "shaders/ascii_art.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->cellPixelsLoc = GetShaderLocation(e->shader, "cellPixels");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->foregroundLoc = GetShaderLocation(e->shader, "foreground");
  e->backgroundLoc = GetShaderLocation(e->shader, "background");
  e->invertLoc = GetShaderLocation(e->shader, "invert");

  return true;
}

void AsciiArtEffectSetup(AsciiArtEffect *e, const AsciiArtConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  int cellPixels = (int)cfg->cellSize;
  SetShaderValue(e->shader, e->cellPixelsLoc, &cellPixels, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);
  float foreground[3] = {cfg->foregroundR, cfg->foregroundG, cfg->foregroundB};
  SetShaderValue(e->shader, e->foregroundLoc, foreground, SHADER_UNIFORM_VEC3);
  float background[3] = {cfg->backgroundR, cfg->backgroundG, cfg->backgroundB};
  SetShaderValue(e->shader, e->backgroundLoc, background, SHADER_UNIFORM_VEC3);
  int invert = cfg->invert ? 1 : 0;
  SetShaderValue(e->shader, e->invertLoc, &invert, SHADER_UNIFORM_INT);
}

void AsciiArtEffectUninit(AsciiArtEffect *e) { UnloadShader(e->shader); }

AsciiArtConfig AsciiArtConfigDefault(void) { return AsciiArtConfig{}; }

void AsciiArtRegisterParams(AsciiArtConfig *cfg) {
  ModEngineRegisterParam("asciiArt.cellSize", &cfg->cellSize, 4.0f, 32.0f);
}

void SetupAsciiArt(PostEffect *pe) {
  AsciiArtEffectSetup(&pe->asciiArt, &pe->effects.asciiArt);
}
// clang-format off
REGISTER_EFFECT(TRANSFORM_ASCII_ART, AsciiArt, asciiArt, "ASCII Art", "RET", 6,
                EFFECT_FLAG_NONE, SetupAsciiArt, NULL)
// clang-format on
