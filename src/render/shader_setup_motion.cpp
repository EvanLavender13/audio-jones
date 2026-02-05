#include "shader_setup_motion.h"
#include "effects/density_wave_spiral.h"
#include "effects/droste_zoom.h"
#include "effects/infinite_zoom.h"
#include "effects/radial_streak.h"
#include "effects/relativistic_doppler.h"
#include "effects/shake.h"
#include "post_effect.h"

void SetupInfiniteZoom(PostEffect *pe) {
  InfiniteZoomEffectSetup(&pe->infiniteZoom, &pe->effects.infiniteZoom,
                          pe->currentDeltaTime);
}

void SetupRadialStreak(PostEffect *pe) {
  RadialStreakEffectSetup(&pe->radialStreak, &pe->effects.radialStreak,
                          pe->currentDeltaTime);
}

void SetupDrosteZoom(PostEffect *pe) {
  DrosteZoomEffectSetup(&pe->drosteZoom, &pe->effects.drosteZoom,
                        pe->currentDeltaTime);
}

void SetupDensityWaveSpiral(PostEffect *pe) {
  DensityWaveSpiralEffectSetup(&pe->densityWaveSpiral,
                               &pe->effects.densityWaveSpiral,
                               pe->currentDeltaTime);
}

void SetupShake(PostEffect *pe) {
  ShakeEffectSetup(&pe->shake, &pe->effects.shake, pe->currentDeltaTime);
}

void SetupRelativisticDoppler(PostEffect *pe) {
  RelativisticDopplerEffectSetup(&pe->relativisticDoppler,
                                 &pe->effects.relativisticDoppler,
                                 pe->currentDeltaTime);
}
