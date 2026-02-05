#include "shader_setup_graphic.h"
#include "effects/disco_ball.h"
#include "effects/halftone.h"
#include "effects/kuwahara.h"
#include "effects/lego_bricks.h"
#include "effects/neon_glow.h"
#include "effects/toon.h"
#include "post_effect.h"

void SetupToon(PostEffect *pe) {
  ToonEffectSetup(&pe->toon, &pe->effects.toon);
}

void SetupNeonGlow(PostEffect *pe) {
  NeonGlowEffectSetup(&pe->neonGlow, &pe->effects.neonGlow);
}

void SetupKuwahara(PostEffect *pe) {
  KuwaharaEffectSetup(&pe->kuwahara, &pe->effects.kuwahara);
}

void SetupHalftone(PostEffect *pe) {
  HalftoneEffectSetup(&pe->halftone, &pe->effects.halftone,
                      pe->currentDeltaTime);
}

void SetupDiscoBall(PostEffect *pe) {
  DiscoBallEffectSetup(&pe->discoBall, &pe->effects.discoBall,
                       pe->currentDeltaTime);
}

void SetupLegoBricks(PostEffect *pe) {
  LegoBricksEffectSetup(&pe->legoBricks, &pe->effects.legoBricks);
}
