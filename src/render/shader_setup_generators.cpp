#include "shader_setup_generators.h"
#include "post_effect.h"

void SetupConstellation(PostEffect *pe) {
  ConstellationEffectSetup(&pe->constellation, &pe->effects.constellation,
                           pe->currentDeltaTime);
}

void SetupPlasma(PostEffect *pe) {
  PlasmaEffectSetup(&pe->plasma, &pe->effects.plasma, pe->currentDeltaTime);
}

void SetupInterference(PostEffect *pe) {
  InterferenceEffectSetup(&pe->interference, &pe->effects.interference,
                          pe->currentDeltaTime);
}
