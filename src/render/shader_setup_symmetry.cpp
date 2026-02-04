#include "shader_setup_symmetry.h"
#include "effects/kaleidoscope.h"
#include "effects/kifs.h"
#include "effects/mandelbox.h"
#include "effects/moire_interference.h"
#include "effects/poincare_disk.h"
#include "effects/radial_ifs.h"
#include "effects/triangle_fold.h"
#include "post_effect.h"

void SetupKaleido(PostEffect *pe) {
  KaleidoscopeEffectSetup(&pe->kaleidoscope, &pe->effects.kaleidoscope,
                          pe->currentDeltaTime);
}

void SetupKifs(PostEffect *pe) {
  KifsEffectSetup(&pe->kifs, &pe->effects.kifs, pe->currentDeltaTime);
}

void SetupPoincareDisk(PostEffect *pe) {
  PoincareDiskEffectSetup(&pe->poincareDisk, &pe->effects.poincareDisk,
                          pe->currentDeltaTime);
}

void SetupMandelbox(PostEffect *pe) {
  MandelboxEffectSetup(&pe->mandelbox, &pe->effects.mandelbox,
                       pe->currentDeltaTime);
}

void SetupTriangleFold(PostEffect *pe) {
  TriangleFoldEffectSetup(&pe->triangleFold, &pe->effects.triangleFold,
                          pe->currentDeltaTime);
}

void SetupMoireInterference(PostEffect *pe) {
  MoireInterferenceEffectSetup(&pe->moireInterference,
                               &pe->effects.moireInterference,
                               pe->currentDeltaTime);
}

void SetupRadialIfs(PostEffect *pe) {
  RadialIfsEffectSetup(&pe->radialIfs, &pe->effects.radialIfs,
                       pe->currentDeltaTime);
}
