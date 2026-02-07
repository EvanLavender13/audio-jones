#include "shader_setup_retro.h"
#include "effects/ascii_art.h"
#include "effects/crt.h"
#include "effects/glitch.h"
#include "effects/matrix_rain.h"
#include "effects/pixelation.h"
#include "effects/synthwave.h"
#include "post_effect.h"

void SetupPixelation(PostEffect *pe) {
  PixelationEffectSetup(&pe->pixelation, &pe->effects.pixelation);
}

void SetupGlitch(PostEffect *pe) {
  GlitchEffectSetup(&pe->glitch, &pe->effects.glitch, pe->currentDeltaTime);
}

void SetupAsciiArt(PostEffect *pe) {
  AsciiArtEffectSetup(&pe->asciiArt, &pe->effects.asciiArt);
}

void SetupMatrixRain(PostEffect *pe) {
  MatrixRainEffectSetup(&pe->matrixRain, &pe->effects.matrixRain,
                        pe->currentDeltaTime);
}

void SetupSynthwave(PostEffect *pe) {
  SynthwaveEffectSetup(&pe->synthwave, &pe->effects.synthwave,
                       pe->currentDeltaTime);
}

void SetupCrt(PostEffect *pe) {
  CrtEffectSetup(&pe->crt, &pe->effects.crt, pe->currentDeltaTime);
}
