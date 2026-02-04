#include "shader_setup_cellular.h"
#include "effects/lattice_fold.h"
#include "effects/phyllotaxis.h"
#include "effects/voronoi.h"
#include "post_effect.h"

void SetupVoronoi(PostEffect *pe) {
  VoronoiEffectSetup(&pe->voronoi, &pe->effects.voronoi, pe->currentDeltaTime);
}

void SetupLatticeFold(PostEffect *pe) {
  LatticeFoldEffectSetup(&pe->latticeFold, &pe->effects.latticeFold,
                         pe->currentDeltaTime, pe->transformTime);
}

void SetupPhyllotaxis(PostEffect *pe) {
  PhyllotaxisEffectSetup(&pe->phyllotaxis, &pe->effects.phyllotaxis,
                         pe->currentDeltaTime);
}
