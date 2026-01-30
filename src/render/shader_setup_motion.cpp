#include "shader_setup_motion.h"
#include "post_effect.h"

void SetupInfiniteZoom(PostEffect *pe) {
  const InfiniteZoomConfig *iz = &pe->effects.infiniteZoom;
  SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomTimeLoc,
                 &pe->infiniteZoomTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomZoomDepthLoc,
                 &iz->zoomDepth, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomLayersLoc, &iz->layers,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomSpiralAngleLoc,
                 &iz->spiralAngle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomSpiralTwistLoc,
                 &iz->spiralTwist, SHADER_UNIFORM_FLOAT);
}

void SetupRadialStreak(PostEffect *pe) {
  const RadialStreakConfig *rs = &pe->effects.radialStreak;
  SetShaderValue(pe->radialStreakShader, pe->radialStreakSamplesLoc,
                 &rs->samples, SHADER_UNIFORM_INT);
  SetShaderValue(pe->radialStreakShader, pe->radialStreakStreakLengthLoc,
                 &rs->streakLength, SHADER_UNIFORM_FLOAT);
}

void SetupDrosteZoom(PostEffect *pe) {
  const DrosteZoomConfig *dz = &pe->effects.drosteZoom;
  SetShaderValue(pe->drosteZoomShader, pe->drosteZoomTimeLoc,
                 &pe->drosteZoomTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->drosteZoomShader, pe->drosteZoomScaleLoc, &dz->scale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->drosteZoomShader, pe->drosteZoomSpiralAngleLoc,
                 &dz->spiralAngle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->drosteZoomShader, pe->drosteZoomShearCoeffLoc,
                 &dz->shearCoeff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->drosteZoomShader, pe->drosteZoomInnerRadiusLoc,
                 &dz->innerRadius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->drosteZoomShader, pe->drosteZoomBranchesLoc, &dz->branches,
                 SHADER_UNIFORM_INT);
}

void SetupDensityWaveSpiral(PostEffect *pe) {
  const DensityWaveSpiralConfig *dws = &pe->effects.densityWaveSpiral;
  float center[2] = {dws->centerX, dws->centerY};
  float aspect[2] = {dws->aspectX, dws->aspectY};
  SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralCenterLoc,
                 center, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralAspectLoc,
                 aspect, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralTightnessLoc,
                 &dws->tightness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->densityWaveSpiralShader,
                 pe->densityWaveSpiralRotationAccumLoc,
                 &pe->densityWaveSpiralRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->densityWaveSpiralShader,
                 pe->densityWaveSpiralGlobalRotationAccumLoc,
                 &pe->densityWaveSpiralGlobalRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralThicknessLoc,
                 &dws->thickness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralRingCountLoc,
                 &dws->ringCount, SHADER_UNIFORM_INT);
  SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralFalloffLoc,
                 &dws->falloff, SHADER_UNIFORM_FLOAT);
}
