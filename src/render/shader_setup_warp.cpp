#include "shader_setup_warp.h"
#include "effects/chladni_warp.h"
#include "effects/circuit_board.h"
#include "effects/corridor_warp.h"
#include "effects/domain_warp.h"
#include "effects/fft_radial_warp.h"
#include "effects/gradient_flow.h"
#include "effects/interference_warp.h"
#include "effects/mobius.h"
#include "effects/radial_pulse.h"
#include "effects/sine_warp.h"
#include "effects/surface_warp.h"
#include "effects/texture_warp.h"
#include "effects/wave_ripple.h"
#include "post_effect.h"

void SetupSineWarp(PostEffect *pe) {
  SineWarpEffectSetup(&pe->sineWarp, &pe->effects.sineWarp,
                      pe->currentDeltaTime);
}

void SetupTextureWarp(PostEffect *pe) {
  TextureWarpEffectSetup(&pe->textureWarp, &pe->effects.textureWarp,
                         pe->currentDeltaTime);
}

void SetupWaveRipple(PostEffect *pe) {
  WaveRippleEffectSetup(&pe->waveRipple, &pe->effects.waveRipple,
                        pe->currentDeltaTime);
}

void SetupMobius(PostEffect *pe) {
  MobiusEffectSetup(&pe->mobius, &pe->effects.mobius, pe->currentDeltaTime);
}

void SetupGradientFlow(PostEffect *pe) {
  GradientFlowEffectSetup(&pe->gradientFlow, &pe->effects.gradientFlow,
                          pe->screenWidth, pe->screenHeight);
}

void SetupChladniWarp(PostEffect *pe) {
  ChladniWarpEffectSetup(&pe->chladniWarp, &pe->effects.chladniWarp,
                         pe->currentDeltaTime);
}

void SetupDomainWarp(PostEffect *pe) {
  DomainWarpEffectSetup(&pe->domainWarp, &pe->effects.domainWarp,
                        pe->currentDeltaTime);
}

void SetupSurfaceWarp(PostEffect *pe) {
  SurfaceWarpEffectSetup(&pe->surfaceWarp, &pe->effects.surfaceWarp,
                         pe->currentDeltaTime);
}

void SetupInterferenceWarp(PostEffect *pe) {
  InterferenceWarpEffectSetup(&pe->interferenceWarp,
                              &pe->effects.interferenceWarp,
                              pe->currentDeltaTime);
}

void SetupCorridorWarp(PostEffect *pe) {
  CorridorWarpEffectSetup(&pe->corridorWarp, &pe->effects.corridorWarp,
                          pe->currentDeltaTime, pe->screenWidth,
                          pe->screenHeight);
}

void SetupRadialPulse(PostEffect *pe) {
  RadialPulseEffectSetup(&pe->radialPulse, &pe->effects.radialPulse,
                         pe->currentDeltaTime);
}

void SetupCircuitBoard(PostEffect *pe) {
  CircuitBoardEffectSetup(&pe->circuitBoard, &pe->effects.circuitBoard,
                          pe->currentDeltaTime);
}

void SetupFftRadialWarp(PostEffect *pe) {
  FftRadialWarpEffectSetup(&pe->fftRadialWarp, &pe->effects.fftRadialWarp,
                           pe->currentDeltaTime, pe->screenWidth,
                           pe->screenHeight, pe->fftTexture);
}
