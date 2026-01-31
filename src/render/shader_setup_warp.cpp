#include "shader_setup_warp.h"
#include "post_effect.h"
#include <math.h>

void SetupSineWarp(PostEffect *pe) {
  const SineWarpConfig *sw = &pe->effects.sineWarp;
  SetShaderValue(pe->sineWarpShader, pe->sineWarpTimeLoc, &pe->sineWarpTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->sineWarpShader, pe->sineWarpRotationLoc, &pe->sineWarpTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->sineWarpShader, pe->sineWarpOctavesLoc, &sw->octaves,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->sineWarpShader, pe->sineWarpStrengthLoc, &sw->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->sineWarpShader, pe->sineWarpOctaveRotationLoc,
                 &sw->octaveRotation, SHADER_UNIFORM_FLOAT);
  int radialMode = sw->radialMode ? 1 : 0;
  SetShaderValue(pe->sineWarpShader, pe->sineWarpRadialModeLoc, &radialMode,
                 SHADER_UNIFORM_INT);
  int depthBlend = sw->depthBlend ? 1 : 0;
  SetShaderValue(pe->sineWarpShader, pe->sineWarpDepthBlendLoc, &depthBlend,
                 SHADER_UNIFORM_INT);
}

void SetupTextureWarp(PostEffect *pe) {
  const TextureWarpConfig *tw = &pe->effects.textureWarp;
  SetShaderValue(pe->textureWarpShader, pe->textureWarpStrengthLoc,
                 &tw->strength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->textureWarpShader, pe->textureWarpIterationsLoc,
                 &tw->iterations, SHADER_UNIFORM_INT);
  int channelMode = (int)tw->channelMode;
  SetShaderValue(pe->textureWarpShader, pe->textureWarpChannelModeLoc,
                 &channelMode, SHADER_UNIFORM_INT);
  SetShaderValue(pe->textureWarpShader, pe->textureWarpRidgeAngleLoc,
                 &tw->ridgeAngle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->textureWarpShader, pe->textureWarpAnisotropyLoc,
                 &tw->anisotropy, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->textureWarpShader, pe->textureWarpNoiseAmountLoc,
                 &tw->noiseAmount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->textureWarpShader, pe->textureWarpNoiseScaleLoc,
                 &tw->noiseScale, SHADER_UNIFORM_FLOAT);
}

void SetupWaveRipple(PostEffect *pe) {
  const WaveRippleConfig *wr = &pe->effects.waveRipple;
  SetShaderValue(pe->waveRippleShader, pe->waveRippleTimeLoc,
                 &pe->waveRippleTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->waveRippleShader, pe->waveRippleOctavesLoc, &wr->octaves,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->waveRippleShader, pe->waveRippleStrengthLoc, &wr->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->waveRippleShader, pe->waveRippleFrequencyLoc,
                 &wr->frequency, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->waveRippleShader, pe->waveRippleSteepnessLoc,
                 &wr->steepness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->waveRippleShader, pe->waveRippleDecayLoc, &wr->decay,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->waveRippleShader, pe->waveRippleCenterHoleLoc,
                 &wr->centerHole, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->waveRippleShader, pe->waveRippleOriginLoc,
                 pe->currentWaveRippleOrigin, SHADER_UNIFORM_VEC2);
  int shadeEnabled = wr->shadeEnabled ? 1 : 0;
  SetShaderValue(pe->waveRippleShader, pe->waveRippleShadeEnabledLoc,
                 &shadeEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(pe->waveRippleShader, pe->waveRippleShadeIntensityLoc,
                 &wr->shadeIntensity, SHADER_UNIFORM_FLOAT);
}

void SetupMobius(PostEffect *pe) {
  const MobiusConfig *m = &pe->effects.mobius;
  SetShaderValue(pe->mobiusShader, pe->mobiusTimeLoc, &pe->mobiusTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->mobiusShader, pe->mobiusPoint1Loc, pe->currentMobiusPoint1,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->mobiusShader, pe->mobiusPoint2Loc, pe->currentMobiusPoint2,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->mobiusShader, pe->mobiusSpiralTightnessLoc,
                 &m->spiralTightness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->mobiusShader, pe->mobiusZoomFactorLoc, &m->zoomFactor,
                 SHADER_UNIFORM_FLOAT);
}

void SetupGradientFlow(PostEffect *pe) {
  const GradientFlowConfig *gf = &pe->effects.gradientFlow;
  SetShaderValue(pe->gradientFlowShader, pe->gradientFlowStrengthLoc,
                 &gf->strength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->gradientFlowShader, pe->gradientFlowIterationsLoc,
                 &gf->iterations, SHADER_UNIFORM_INT);
  SetShaderValue(pe->gradientFlowShader, pe->gradientFlowEdgeWeightLoc,
                 &gf->edgeWeight, SHADER_UNIFORM_FLOAT);
  int randomDir = gf->randomDirection ? 1 : 0;
  SetShaderValue(pe->gradientFlowShader, pe->gradientFlowRandomDirectionLoc,
                 &randomDir, SHADER_UNIFORM_INT);
}

void SetupChladniWarp(PostEffect *pe) {
  const ChladniWarpConfig *cw = &pe->effects.chladniWarp;

  // CPU phase accumulation for smooth animation
  pe->chladniWarpPhase += pe->currentDeltaTime * cw->animRate;

  SetShaderValue(pe->chladniWarpShader, pe->chladniWarpNLoc, &cw->n,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->chladniWarpShader, pe->chladniWarpMLoc, &cw->m,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->chladniWarpShader, pe->chladniWarpPlateSizeLoc,
                 &cw->plateSize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->chladniWarpShader, pe->chladniWarpStrengthLoc,
                 &cw->strength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->chladniWarpShader, pe->chladniWarpModeLoc, &cw->warpMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->chladniWarpShader, pe->chladniWarpAnimPhaseLoc,
                 &pe->chladniWarpPhase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->chladniWarpShader, pe->chladniWarpAnimRangeLoc,
                 &cw->animRange, SHADER_UNIFORM_FLOAT);
  int preFold = cw->preFold ? 1 : 0;
  SetShaderValue(pe->chladniWarpShader, pe->chladniWarpPreFoldLoc, &preFold,
                 SHADER_UNIFORM_INT);
}

void SetupDomainWarp(PostEffect *pe) {
  const DomainWarpConfig *dw = &pe->effects.domainWarp;
  SetShaderValue(pe->domainWarpShader, pe->domainWarpWarpStrengthLoc,
                 &dw->warpStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->domainWarpShader, pe->domainWarpWarpScaleLoc,
                 &dw->warpScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->domainWarpShader, pe->domainWarpWarpIterationsLoc,
                 &dw->warpIterations, SHADER_UNIFORM_INT);
  SetShaderValue(pe->domainWarpShader, pe->domainWarpFalloffLoc, &dw->falloff,
                 SHADER_UNIFORM_FLOAT);
  float timeOffset[2] = {cosf(dw->driftAngle) * pe->domainWarpDrift,
                         sinf(dw->driftAngle) * pe->domainWarpDrift};
  SetShaderValue(pe->domainWarpShader, pe->domainWarpTimeOffsetLoc, timeOffset,
                 SHADER_UNIFORM_VEC2);
}

void SetupSurfaceWarp(PostEffect *pe) {
  const SurfaceWarpConfig *sw = &pe->effects.surfaceWarp;

  // Accumulate rotation and scroll on CPU (avoids jumps when speed changes)
  pe->surfaceWarpRotation += sw->rotationSpeed * pe->currentDeltaTime;
  pe->surfaceWarpScrollOffset += sw->scrollSpeed * pe->currentDeltaTime;

  SetShaderValue(pe->surfaceWarpShader, pe->surfaceWarpIntensityLoc,
                 &sw->intensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->surfaceWarpShader, pe->surfaceWarpAngleLoc, &sw->angle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->surfaceWarpShader, pe->surfaceWarpRotationLoc,
                 &pe->surfaceWarpRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->surfaceWarpShader, pe->surfaceWarpScrollOffsetLoc,
                 &pe->surfaceWarpScrollOffset, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->surfaceWarpShader, pe->surfaceWarpDepthShadeLoc,
                 &sw->depthShade, SHADER_UNIFORM_FLOAT);
}

void SetupInterferenceWarp(PostEffect *pe) {
  const InterferenceWarpConfig *iw = &pe->effects.interferenceWarp;

  pe->interferenceWarpTime += pe->currentDeltaTime * iw->speed;
  pe->interferenceWarpAxisRotation +=
      pe->currentDeltaTime * iw->axisRotationSpeed;

  SetShaderValue(pe->interferenceWarpShader, pe->interferenceWarpTimeLoc,
                 &pe->interferenceWarpTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->interferenceWarpShader, pe->interferenceWarpAmplitudeLoc,
                 &iw->amplitude, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->interferenceWarpShader, pe->interferenceWarpScaleLoc,
                 &iw->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->interferenceWarpShader, pe->interferenceWarpAxesLoc,
                 &iw->axes, SHADER_UNIFORM_INT);
  SetShaderValue(pe->interferenceWarpShader,
                 pe->interferenceWarpAxisRotationLoc,
                 &pe->interferenceWarpAxisRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->interferenceWarpShader, pe->interferenceWarpHarmonicsLoc,
                 &iw->harmonics, SHADER_UNIFORM_INT);
  SetShaderValue(pe->interferenceWarpShader, pe->interferenceWarpDecayLoc,
                 &iw->decay, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->interferenceWarpShader, pe->interferenceWarpDriftLoc,
                 &iw->drift, SHADER_UNIFORM_FLOAT);
}

void SetupCorridorWarp(PostEffect *pe) {
  const CorridorWarpConfig *cw = &pe->effects.corridorWarp;

  // Accumulate rotations and offsets on CPU
  pe->corridorWarpViewRotation += cw->viewRotationSpeed * pe->currentDeltaTime;
  pe->corridorWarpPlaneRotation +=
      cw->planeRotationSpeed * pe->currentDeltaTime;
  pe->corridorWarpScrollOffset += cw->scrollSpeed * pe->currentDeltaTime;
  pe->corridorWarpStrafeOffset += cw->strafeSpeed * pe->currentDeltaTime;

  float resolution[2] = {(float)pe->screenWidth, (float)pe->screenHeight};
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpResolutionLoc,
                 resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpHorizonLoc,
                 &cw->horizon, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpPerspectiveStrengthLoc,
                 &cw->perspectiveStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpModeLoc, &cw->mode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpViewRotationLoc,
                 &pe->corridorWarpViewRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpPlaneRotationLoc,
                 &pe->corridorWarpPlaneRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpScaleLoc, &cw->scale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpScrollOffsetLoc,
                 &pe->corridorWarpScrollOffset, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpStrafeOffsetLoc,
                 &pe->corridorWarpStrafeOffset, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->corridorWarpShader, pe->corridorWarpFogStrengthLoc,
                 &cw->fogStrength, SHADER_UNIFORM_FLOAT);
}

void SetupShake(PostEffect *pe) {
  const ShakeConfig *s = &pe->effects.shake;

  // Accumulate time on CPU for motionScale compatibility
  pe->shakeTime += pe->currentDeltaTime;

  SetShaderValue(pe->shakeShader, pe->shakeTimeLoc, &pe->shakeTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->shakeShader, pe->shakeIntensityLoc, &s->intensity,
                 SHADER_UNIFORM_FLOAT);
  int samples = (int)s->samples;
  SetShaderValue(pe->shakeShader, pe->shakeSamplesLoc, &samples,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->shakeShader, pe->shakeRateLoc, &s->rate,
                 SHADER_UNIFORM_FLOAT);
  int gaussian = s->gaussian ? 1 : 0;
  SetShaderValue(pe->shakeShader, pe->shakeGaussianLoc, &gaussian,
                 SHADER_UNIFORM_INT);
}

void SetupRadialPulse(PostEffect *pe) {
  const RadialPulseConfig *rp = &pe->effects.radialPulse;
  SetShaderValue(pe->radialPulseShader, pe->radialPulseRadialFreqLoc,
                 &rp->radialFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialPulseShader, pe->radialPulseRadialAmpLoc,
                 &rp->radialAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialPulseShader, pe->radialPulseSegmentsLoc,
                 &rp->segments, SHADER_UNIFORM_INT);
  SetShaderValue(pe->radialPulseShader, pe->radialPulseAngularAmpLoc,
                 &rp->angularAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialPulseShader, pe->radialPulsePetalAmpLoc,
                 &rp->petalAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialPulseShader, pe->radialPulsePhaseLoc,
                 &pe->radialPulseTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialPulseShader, pe->radialPulseSpiralTwistLoc,
                 &rp->spiralTwist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialPulseShader, pe->radialPulseOctavesLoc, &rp->octaves,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->radialPulseShader, pe->radialPulseOctaveRotationLoc,
                 &rp->octaveRotation, SHADER_UNIFORM_FLOAT);
  int depthBlend = rp->depthBlend ? 1 : 0;
  SetShaderValue(pe->radialPulseShader, pe->radialPulseDepthBlendLoc,
                 &depthBlend, SHADER_UNIFORM_INT);
}
